/********************************************************************
*  Name: sensors.cpp
*
*  Purpose: Functions relating to data acquisition sensors
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include "lib/daq/sensors.h"
#include "lib/telemetry/TelemetryViewer.h"
#include "lib/telemetry/TelemetryWriter.h"
#include "lib/trigger/trigger.h"
#include "common/types.h"

using namespace trigger;

// macros
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


RetType DAQ_ADC_SCALE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    // NOTE: we don't do an arg check!
    // if it segfaults, that's the fault of whoever made the triggers file wrong

    int32_t data;

    if(unlikely(SUCCESS != tv->get_int(args->args[0], &data))) {
        return FAILURE;
    }

    static const double vref = 2.442; // volts

    double result = (double)data * vref / (double)(1 << 23);

    if(unlikely(tw->write(args->args[1], (uint8_t*)&result, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}

// K inverse polynomial approximations from NIST
// https://srdata.nist.gov/its90/type_k/kcoefficients_inverse.html
// given in 3 temperature ranges

// polynomial coefficients for range 0, -5.891mV to 0mV
#define K_INVERSE_R0_NUM 9 // number of coefficients
static double dr0[K_INVERSE_R0_NUM] =
{
    0.0E0,          // d0
    2.5173462E1,    // d1
    -1.1662878E0,   // d2
    -1.0833638E0,   // d3
    -8.9773540E-1,  // d4
    -3.7342377E-1,  // d5
    -8.6632643E-2,  // d6
    -1.0450598E-2,  // d7
    -5.1920577E-4   // d8
};

// polynomial coefficients for range 1, 0mV to 20.644mV
#define K_INVERSE_R1_NUM 10 // number of coefficients
static double dr1[K_INVERSE_R1_NUM] =
{
    0.0E0,          // d0
    2.508355E1,     // d1
    7.860106E-2,    // d2
    -2.503131E-1,   // d3
    8.315270E-2,    // d4
    -1.228034E-2,   // d5
    9.804036E-4,    // d6
    -4.413030E-5,   // d7
    1.057734E-6,    // d8
    -1.052755E-8    // d9
};

// polynomial coefficients for range 2, 20.644mV to 54.886mV
#define K_INVERSE_R2_NUM 7 // number of coefficients
static double dr2[K_INVERSE_R2_NUM] =
{
    -1.318058E-2,   // d0
    4.830222E1,     // d1
    -1.646031E0,    // d2
    5.464731E2,     // d3
    -9.650715E-4,   // d4
    8.802193E-6,    // d5
    -3.110810E-8,   // d6
};

// @arg1 raw input (32 bit int)
// @arg2 connected status
// @arg3 remote temperature in Celsius (double)
// @arg4 ambient temperature in Celsius (double)
// @arg5 corrected absolute temperature (Celsius) (double)
RetType MAX31855K_THERMOCOUPLE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    // NOTE: we don't do an arg check
    // if it segfaults, that's the fault of whoever made the triggers file wrong

    uint32_t data;

    if(unlikely(SUCCESS != tv->get_uint(args->args[0], &data))) {
        return FAILURE;
    }

    uint8_t connected = 1;

    if(data & (1 << 16)) {
        connected = 0;
    }

    if(unlikely(SUCCESS != tw->write(args->args[1], (uint8_t*)&connected, sizeof(uint8_t)))) {
        return FAILURE;
    }

    if(!connected) {
        return SUCCESS;
    }

    // remote temp, 14 highest bits
    int16_t tr = (data >> 18);
    // if(tr & 0b0010000000000000) { // sign bit is set
    //     TR *= -1;
    // }

    // ratio from manual
    double remote = tr * 0.25;

    if(unlikely(SUCCESS != tw->write(args->args[2], (uint8_t*)&remote, sizeof(double)))) {
        return FAILURE;
    }

    // ambient temp, 12 bits starting at bit 4
    int16_t tamb = ((data >> 4) & 0x0FFF);
    // int16_t TAMB = *(&tamb(int16_t*)&tamb);
    // if(tamb & 0b0000100000000000) { // sign bit is set
    //     TAMB *= -1;
    // }

    // ratio from manual
    double ambient = tamb * 0.0625;

    if(unlikely(SUCCESS != tw->write(args->args[3], (uint8_t*)&ambient, sizeof(double)))) {
        return FAILURE;
    }

    // MAX31855 manual page 8
    // Vout = (41.276 uV / C) * (Tremote - Tambient)
    double vout = (41.276 / 1000) * (remote - ambient); // millivolts

    // find which range vout is in
    if(vout < -5.891) {
        // out of range for NIST polynomials
        return FAILURE;
    }

    double t = 0.0;
    double E = 1.0;

    if(vout < 0) {
        // use polynomial 1
        for(size_t i = 0; i < K_INVERSE_R0_NUM; i++) {
            t += (dr0[i] * E);
            E *= vout;
        }
    } else if(vout < 20.664) {
        // use polynomial 2
        for(size_t i = 0; i < K_INVERSE_R1_NUM; i++) {
            t += (dr1[i] * E);
            E *= vout;
        }
    } else if(vout < 54.886) {
        // use polynnomial 3
        for(size_t i = 0; i < K_INVERSE_R2_NUM; i++) {
            t += (dr2[i] * E);
            E *= vout;
        }
    } else {
        // too high
        // this is readings over 1372 C, which we can't read
        return FAILURE;
    }

    t += ambient;

    // t is corrected absolute temperature
    if(unlikely(SUCCESS != tw->write(args->args[4], (uint8_t*)&t, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}


// @arg1 input measured voltage (double)
// @arg2 measured force in lbF (double)
RetType PCB1403_CURRENT_EXCITE(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    double vmeas;

    if(unlikely(tv->get_double(args->args[0], &vmeas))) {
        return FAILURE;
    }

    // vmeas / 10 mA = Rmeas
    // Rnominal = 350 ohm
    // Ratio of Rmeas:Rnominal is proportional to force measured
    // full scale force is 2500 lbF
    // we negate so that pushing is positive
    double f = -((vmeas / 0.01) / 350) * 2500;

    if(unlikely(tw->write(args->args[1], (uint8_t*)&f, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}

// @arg1 input measured voltage (double)
// @arg2 measured pressure (PSI)
RetType PRESSURE_TRANSDUCER_8252(TelemetryViewer* tv, TelemetryWriter* tw, arg_t* args) {
    double vmeas;

    if(unlikely(tv->get_double(args->args[0], &vmeas))) {
        return FAILURE;
    }

    // measured current is vmeas / resistance of current sense resistor (121 ohms)
    // measured current is in the linear scale from 4-20mA
    // 1500 psi is max pressure
    double p = (((vmeas / 121) - 0.004) / 0.016) * 1500;

    if(unlikely(tw->write(args->args[1], (uint8_t*)&p, sizeof(double)))) {
        return FAILURE;
    }

    return SUCCESS;
}
