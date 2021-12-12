#ifndef GSW_NET_H
#define GSW_NET_H

#include "common/types.h"
#include <netdb.h>
#include <sys/socket.h>

// Get an IPv4 address from a hostname
static inline RetType get_addr(const char* host, struct in_addr* ret) {
    struct hostent* h = gethostbyname(host);

    if(h->h_addrtype != AF_INET) {
        return FAILURE;
    }

    if(!(h->h_addr_list[0])) {
        return FAILURE;
    }

    *ret = *((struct in_addr*)h->h_addr_list[0]);

    return SUCCESS;
}

#endif
