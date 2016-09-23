/*
 * rand4.c
 *
 *  Created on: Mar 31, 2016
 *      Author: Erica
 */

#include <msp430g2553.h>
#include <stdint.h>
#include "rand4.h"

/* This function should be initialized with a non-zero seed before used to generate random numbers.*/
int rand4(int seed){
    static uint16_t lfsr;
    static uint16_t bit;

    if (seed == 0){ // go on with the sequence determined by the previous seed
        bit  = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
        lfsr =  (lfsr >> 1) | (bit << 1);
    }
    else { // update the seed
    	lfsr = (uint16_t) seed;
    	bit  = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
    	lfsr =  (lfsr >> 1) | (bit << 1);
    }

    return (lfsr & 0x03); // output a random integer in range [0,3]
}
