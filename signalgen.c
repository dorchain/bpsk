/*
 * Copyright 2018 Joerg Dorchain
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Joerg Dorchain wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MICROSECONDS 1000000

#define REF_FREQ (4000000/76) /* 52631.57894736842105263157 Hz */
#define REF_PERIOD (MICROSECONDS/REF_FREQ) /* 19 us for a full wave */

#define PERIODS_PER_CLOCK 24
#define CLOCK_RATE (REF_FREQ/PERIODS_PER_CLOCK)
#define PERIODS_PER_BIT (2*PERIODS_PER_CLOCK)
#define BIT_RATE (REF_FREQ/PERIODS_PER_BIT) /* 1096.49122807017543859649 */

#define SAMPLE_RATE	192000
#define SAMPLE_PERIOD (MICROSECONDS/SAMPLE_RATE)

char modulation[] = {1,-1,1,1,-1,1,-1,-1}; /* 1,-1 bit sequence repeats endlessly */

float get_sample()
{
static float f = -SAMPLE_PERIOD * 2 * M_PI / REF_PERIOD;
static int fullwaves = 0;
static int bitcounter = 0;

f += SAMPLE_PERIOD * 2 * M_PI  / REF_PERIOD ;
if (f > 2 * M_PI) { /* nyquist makes sure we have at least 2 sample per period */
  f -= 2 * M_PI;
  fullwaves++;
  fullwaves %= PERIODS_PER_CLOCK;
  if (fullwaves == 0) {
  	bitcounter++;
	bitcounter %= sizeof(modulation);
  }
}
return modulation[bitcounter]*sin(f);
}


int main(int argc, char **argv)
{
long long i;
float S;

for( i = 0; i < 1000000; i++) {

S = get_sample();
write(1, &S, sizeof(S));
}
}
