/********************************************************************
*  Name: vcalc.h
*
*  Purpose: User created functions to handle virtual telemetry
*           for virtual telemetry calculations.
*
*  NOTE:    This file should only be included ONCE by "calc.cpp"
*           If it is included by multiple files linked together
*           there will be a name collision.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#ifndef VFUNC_H
#define VFUNC_H

#include "lib/calc/calc.h"
#include <string.h>

using namespace calc;
using namespace dls;
using namespace vcm;
using namespace convert;

// TODO these functions should really really really be broken up into different files

// macros
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

// CONVERSION FUNCTIONS //
// global VCM used by calculation functions
// set by the 'parse_vfile' function of "calc.cpp"
extern VCM* veh;

// directly copy a measurement
// 'out' must be the same size as 'in'
static RetType COPY(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    MsgLogger logger("CALC", "COPY");

    if(args.size() != 1) {
        logger.log_message("Incorrect number of args");
        return FAILURE;
    }

    measurement_info_t* in = args[0].meas;
    const uint8_t* src = args[0].addr;

    if(meas->size != in->size) {
        logger.log_message("Size mismatch");
        return FAILURE;
    }

    // need a special function for writing single measurement to shared memory!
    // ^^^ actually we're just writing to a passed in buffer, this is fine
    memcpy((void*)(dst), (void*)src, meas->size);

    return SUCCESS;
}

static RetType SUM_UINT(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    MsgLogger logger("CALC", "INTSUM");

    uint32_t sum = 0;
    uint32_t temp;

    // sum all arguments
    for(arg_t arg : args) {
        if(convert_to(veh, arg.meas, arg.addr, &temp) != SUCCESS) {
            logger.log_message("conversion failed");
            return FAILURE;
        }

        sum += temp;
    }

    // write out sum to output measurement
    if(convert_from(veh, meas, dst, sum) != SUCCESS) {
        logger.log_message("failed to convert sum to uint32");
        return FAILURE;
    }

    return SUCCESS;
}

// converts a raw ADC DAQ reading to a voltage (in millivolts)
// assumes a reference voltage of 2.442 volts
// outputs to double format
static RetType DAQ_ADC_SCALE(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    // forgo logger for speed! (gotta go fast)
    // MsgLogger logger("CALC", "DAQ_ADC_VOTLAGE");

    // assume we have the right number of args
    // TODO can we check this at parse time of the virtual file?
    // or just assume the user knows what they're doing and let it crash

    int32_t data;
    if(unlikely(convert_to(veh, args[0].meas, args[0].addr, &data) != SUCCESS)) {
        // logger.log_message("conversion failed");
        return FAILURE;
    }


    // voltage reference
    static const double vref = 2.442; // volts

    double result = (double)data * vref / (double)(1 << 23);

    if(unlikely(convert_from(veh, meas, dst, result) != SUCCESS)) {
        // logger.log_message("failed to write out result");
        return FAILURE;
    }

    return SUCCESS;
}

// converts a load cell voltage to a percent of full-scale
// assumes load cell is using current excitation of 10mA
// also assumes has a nominal arm resistance of 350 ohms and a range of 350 (+/-)10 ohms
// outputs to a double in the range [0, 1.0]
static RetType LOAD_CELL_PERCENT_CURRENT_EXCITATION(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    double vm; // measured voltage
    if(unlikely(convert_to(veh, args[0].meas, args[0].addr, &vm) != SUCCESS)) {
        return FAILURE;
    }

    // calculate ratio of measured resistance to nominal arm resistance
    // https://www.dataloggerinc.com/resource-article/strain-gauge-measurements-current/
    static double nominal_arm_resitance = 350; // ohms
    static double delta_resistance = 10; // ohms
    static double current = 0.01; // amps (current excitation of 10 mA)

    double ratio = (vm / current) / nominal_arm_resitance;

    // map to a range between nominal and max range
    double percent = ((vm / current) - 350) / delta_resistance;

    // write out percent
    if(unlikely(convert_from(veh, meas, dst, percent) != SUCCESS)) {
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

// converts a 32-bit thermocouple reading from a MAX31855K to a relative temperature in degrees Celsius
// outputs to double format
static RetType KTYPE_THERMOCOUPLE_TEMP(measurement_info_t* meas, uint8_t* dst, std::vector<arg_t>& args) {
    uint32_t data;
    // convert functions shouldn't fail if the config is correct, hint that to the branch predictor
    if(unlikely(convert_to(veh, args[0].meas, args[0].addr, &data) != SUCCESS)) {
        return FAILURE;
    }

    if(data & (1 << 15)) {
        // fault bit is set, TC not connected (most likely)
        return FAILURE;
    }

    // remote temp, 14 highest bits
    uint16_t tr = data >> 18;
    int16_t TR = tr;
    if(tr & 0b0010000000000000) { // sign bit is set
        TR *= -1;
    }

    // ambient temp
    uint16_t tamb = (data >> 4) & 0xF0;
    int16_t TAMB = tamb;
    if(tamb & 0b0000100000000000) { // sign bit is set
        TAMB *= -1;
    }

    // Vout = (41.276 uV / C) * (TR - TAMB)
    // Vout = (41.276 uV / C) * (degree_ratio)(TR_reading - TAMB_reading)

    // degree ratio is how many degrees C per magnitude of reading (assuming linear scale)
    // comes from manual page 10, 0b01100100000000 equates to 1600 C
    // this yields a ratio of 4 C per magnitude

    // Vout = (4 * 41.276 uV / C) * (degree_ratio)(TR_reading - TAMB_reading)

    // vout is the voltage output of the thermocouple in mV
    // derived from the termperatures reported by the MAX31855K which assumed a linear scale
    // we can use the temperature readings to calculate vout and find the termperature using a non-linear method

    // the multiplication of constants should get optimized out
    double vout = (4 * 0.041276) * (TR - TAMB);

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

    // write out relative temperature
    if(unlikely(convert_from(veh, meas, dst, t) != SUCCESS)) {
        // logger.log_message("failed to write out result");
        return FAILURE;
    }

    return SUCCESS;
}

// functions mapped to names //
typedef struct {
    const char* name;
    RetType (*func)(measurement_info_t*, uint8_t*, std::vector<arg_t>&);
} vfunc_t;

const vfunc_t vfunc_list[] =
{
    {"COPY", &COPY},
    {"SUM_UINT", &SUM_UINT},
    {"DAQ_ADC_SCALE", &DAQ_ADC_SCALE},
    {"KTYPE_THERMOCOUPLE_TEMP", &KTYPE_THERMOCOUPLE_TEMP},
    {"LOAD_CELL_PERCENT_CURRENT_EXCITATION", LOAD_CELL_PERCENT_CURRENT_EXCITATION}
    {NULL, NULL}
};

#endif
