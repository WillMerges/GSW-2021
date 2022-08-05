#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "lib/dev/xbee/net.h"
#include "lib/dev/xbee/xbee.h"

#define START_DELIMETER   0x7E
#define TX_FRAME_TYPE     0x10
#define RX_FRAME_TYPE     0x90
#define REMOTE_AT_CMD_FRAME_TYPE 0x17
#define LOCAL_AT_CMD_FRAME_TYPE 0x08
#define RESERVED_VALUE    0xFEFF // in network order

// static buffer sizes
#define TX_BUFF_SIZE 1024 // bytes
#define RX_BUFF_SIZE 2048 // bytes

typedef struct {
    uint8_t start_delimiter;
    uint16_t length;
} __attribute__((packed)) xb_header_t;

typedef struct {
    xb_header_t header;
    uint8_t frame_type;
    uint8_t frame_id;
    uint64_t dst_addr;
    uint16_t reserved;
    uint8_t radius;
    uint8_t options;
} __attribute__((packed)) xb_tx_frame_t;

typedef struct {
    xb_header_t header;
    uint8_t frame_type;
    uint64_t src_addr;
    uint16_t reserved;
    uint8_t options;
} __attribute__((packed)) xb_rx_frame_t;

typedef struct {
    xb_header_t header;
    uint8_t frame_type;
    uint8_t frame_id;
    uint64_t dst_addr;
    uint16_t reserved;
    uint8_t options;
    uint8_t at_command[2];
} __attribute__((packed)) xb_remote_at_frame_t;

typedef struct {
    xb_header_t header;
    uint8_t frame_type;
    uint8_t frame_id;
    uint8_t at_command[2];
} __attribute__((packed)) xb_at_frame_t;

int (*xb_write)(uint8_t* buf, size_t len) = NULL;
void (*xb_delay)(uint32_t ms) = NULL;

static uint64_t default_dst = 0xFFFF000000000000; // broadcast address in network order
static void (*rx_callback)(uint8_t* buff, size_t len, uint64_t src_addr);

static uint8_t tx_buff[TX_BUFF_SIZE];

xb_ret_t xb_send(uint8_t* data, size_t len) {
    return xb_sendto(default_dst, data, len);
}

xb_ret_t xb_sendto(uint64_t addr, uint8_t* data, size_t len) {
    xb_tx_frame_t* frame = (xb_tx_frame_t*)tx_buff;

    frame->header.start_delimiter = START_DELIMETER;
    frame->header.length = hton16(len + sizeof(xb_tx_frame_t) - sizeof(xb_header_t));
    frame->frame_type = TX_FRAME_TYPE;
    frame->frame_id = 0;
    frame->dst_addr = addr;
    frame->reserved = RESERVED_VALUE;
    frame->radius = 0;
    frame->options = 0x80;

    memcpy(tx_buff + sizeof(xb_tx_frame_t), data, len);

    uint8_t check = 0;
    for(size_t i = sizeof(xb_header_t); i < len + sizeof(xb_tx_frame_t); i++) {
        check += tx_buff[i];
    }

    check = 0xFF - check;
    memcpy(tx_buff + len + sizeof(xb_tx_frame_t), &check, 1);

    int write_len = len + sizeof(xb_tx_frame_t) + 1;
    if(xb_write(tx_buff, write_len) != write_len) {
        // write error
        return XB_ERR;
    }

    return XB_OK;
}

void xb_attach_rx_callback(void (*rx)(uint8_t* buff, size_t len, uint64_t src_addr)) {
    rx_callback = rx;
}

#define RX_BUFF_SIZE 2048 // bytes
static uint8_t rx_buff[RX_BUFF_SIZE];

void xb_rx_complete(xb_rx_request* req) {
    typedef enum {
        WAITING_FOR_FRAME,
        WAITING_FOR_LENGTH,
        READING_PAYLOAD,
        WAIT_FOR_FRAME_END
    } state_t;

    static size_t rx_index;
    static size_t to_read; // bytes left of payload to read
    static size_t payload_size;

    static state_t state = WAITING_FOR_FRAME;
    static uint8_t check = 0;

    uint8_t len_buff[2];
    size_t i = 0;


    start_switch:
    switch(state) {
        case WAITING_FOR_FRAME:
            for(; i < req->len; i++) {
                if(req->buff[i] == START_DELIMETER) {
                    state = WAITING_FOR_LENGTH;
                    rx_index = 0;
                    i++;
                    break; // don't need to keep checking for delimeters
                }
            }

            if(state == WAITING_FOR_FRAME) {
                // we didn't get a delimter, don't save anything from our buffer
                req->len = 3;
                req->buff = rx_buff;
                break;
            } // otherwise start parsing header

        case WAITING_FOR_LENGTH:
            for(; i < req->len; i++) {
                len_buff[rx_index] = req->buff[i];
                rx_index++;
                if(rx_index == 2) {
                    payload_size = ntoh16(*((uint16_t*)len_buff));
                    to_read = payload_size + 1; // read checksum too

                    // start writing over the buffer again, already stored length
                    rx_index = 0;

                    state = READING_PAYLOAD;
                    i++;
                    break; // don't need to keep reading header
                }
            }

            if(state == WAITING_FOR_LENGTH) {
                req->len = 2 - rx_index;
                req->buff = rx_buff;
                break;
            }

        case READING_PAYLOAD:
            for(; i < req->len; i++) {
                rx_index++;
                to_read--;

                if(to_read == 0) {
                    uint8_t checksum = 0xFF - check;
                    if(checksum == rx_buff[rx_index - 1]) {
                        // pick a callback based on the frame type
                        switch(rx_buff[0]) {
                            case RX_FRAME_TYPE:
                                // printf("payload size: %li\n", payload_size - sizeof(xb_rx_frame_t) - sizeof(xb_header_t));
                                if(rx_callback) {
                                    rx_callback(rx_buff + sizeof(xb_rx_frame_t) - sizeof(xb_header_t),
                                        payload_size - (sizeof(xb_rx_frame_t) - sizeof(xb_header_t)),
                                        ntoh64(*((uint64_t*)(rx_buff + 1))));
                                }
                                break;
                            default:
                                // no callback
                                break;
                        }
                    } // else bad check, throwaway

                    // read a full frame, reset from the top
                    check = 0;
                    state = WAITING_FOR_FRAME;
                    goto start_switch;
                } else {
                    check += req->buff[i];
                }

                if(rx_index == RX_BUFF_SIZE) {
                    // can't read anymore, throw away packet
                    check = 0;
                    state = WAIT_FOR_FRAME_END;
                    goto start_switch;
                }
            }

            req->len = to_read;
            req->buff = rx_buff + rx_index;
            break; // should immediately fail next loop anyways
        case WAIT_FOR_FRAME_END:
            for(; i < req->len; i++) {
                // do nothing
                to_read--;

                if(to_read == 0) {
                    // restart now
                    state = WAITING_FOR_FRAME;
                    goto start_switch;
                }
            }
    }
}

void xb_set_default_dst(uint64_t addr) {
    default_dst = hton64(addr);
}

static xb_ret_t xb_remote_at_cmd(const char cmd[2], uint8_t* param, size_t param_size) {
    xb_remote_at_frame_t* frame = (xb_remote_at_frame_t*)tx_buff;

    frame->header.start_delimiter = START_DELIMETER;
    frame->header.length = hton16(sizeof(xb_remote_at_frame_t) - sizeof(xb_header_t) + param_size);
    frame->frame_type = REMOTE_AT_CMD_FRAME_TYPE;
    frame->frame_id = 0; // NOTE: this means we won't get a response frame!
    frame->dst_addr = default_dst;
    frame->reserved = hton16(0xFFFE);
    frame->options = 0x02; // apply changes immediately on remote
    frame->at_command[0] = cmd[0];
    frame->at_command[1] = cmd[1];

    // copy parameter
    memcpy(tx_buff + sizeof(xb_remote_at_frame_t), param, param_size);

    uint8_t check = 0;
    size_t i;
    for(i = sizeof(xb_header_t); i < sizeof(xb_remote_at_frame_t) + param_size; i++) {
        check += tx_buff[i];
    }

    tx_buff[i] = 0xFF - check;

    if(xb_write(tx_buff, i + 1) < ((int)(i + 1))) {
        // write error
        return XB_ERR;
    }

    return XB_OK;
}

xb_ret_t xb_at_cmd(const char cmd[2], uint8_t* param, size_t param_size) {
    xb_at_frame_t* frame = (xb_at_frame_t*)tx_buff;

    frame->header.start_delimiter = START_DELIMETER;
    frame->header.length = hton16(sizeof(xb_at_frame_t) - sizeof(xb_header_t) + param_size);
    frame->frame_type = LOCAL_AT_CMD_FRAME_TYPE;
    frame->frame_id = 0;
    frame->at_command[0] = cmd[0];
    frame->at_command[1] = cmd[1];

    // copy parameter
    memcpy(tx_buff + sizeof(xb_remote_at_frame_t), param, param_size);

    uint8_t check = 0;
    size_t i;
    for(i = sizeof(xb_header_t); i < sizeof(xb_remote_at_frame_t) + param_size; i++) {
        check += tx_buff[i];
    }

    tx_buff[i] = 0xFF - check;

    if(xb_write(tx_buff, i + 1) < ((int)(i + 1))) {
        // write error
        return XB_ERR;
    }

    return XB_OK;
}

xb_ret_t xb_cmd_remote_dio(xb_dio_t dio, xb_dio_output_t output) {
    char cmd[2];

    switch(dio) {
        case XB_DIO12:
            // P2
            cmd[0] = 'P';
            cmd[1] = '2';
            break;
        default:
            return XB_ERR;
    }

    // parameter passed as binary
    uint8_t param = 0x04;
    if(output == XB_DIO_HIGH) {
        param = 0x05;
    } // else low (4)

    return xb_remote_at_cmd(cmd, &param, 1);
}


xb_ret_t xb_cmd_dio(xb_dio_t dio, xb_dio_output_t output) {
    char cmd[2];

    switch(dio) {
        case XB_DIO12:
            // P2
            cmd[0] = 'P';
            cmd[1] = '2';
            break;
        default:
            return XB_ERR;
    }

    // parameter passed as binary
    uint8_t param = 0x04;
    if(output == XB_DIO_HIGH) {
        param = 0x05;
    } // else low (4)

    return xb_at_cmd(cmd, &param, 1);
}

xb_ret_t xb_set_net_id(uint16_t id) {
    char cmd[2] = {'I', 'D'};

    if(id > 0x7FFF) {
        // invalid ID
        return XB_ERR;
    }

    uint16_t param = hton16(id);
    return xb_at_cmd(cmd, (uint8_t*)&param, sizeof(uint16_t));
}

void xb_set_handler(int (*write) (uint8_t* buff, size_t len)) {
    xb_write = write;
}

xb_ret_t xb_init(int (*write)(uint8_t* buf, size_t len), void (*delay)(uint32_t ms)) {
    xb_write = write;
    xb_delay = delay;

    // the line should be silent for 1s
    // wait for 1.1s to be safe
    delay(1100);

    // enter command mode
    if(xb_write((uint8_t*)"+++", 3) < 3) {
        // write failure
        return XB_ERR;
    }

    // the line should be silent for 1s
    // wait for 1.1s to be safe
    delay(1100);

    // send command to put into API mode
    const char* at_cmd = "ATAP1\r"; // API mode without escapes

    if(xb_write((uint8_t*)at_cmd, 6) < 6) {
        // write failure
        return XB_ERR;
    }

    // at this point we're in API mode

    return XB_OK;
}
