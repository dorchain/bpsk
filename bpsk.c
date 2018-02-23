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

#define SAMPLE_RATE	192000
#define SAMPLE_PERIOD (MICROSECONDS/SAMPLE_RATE)

/* LPF Filter features */
#define CUTOFF_FREQ 55000 /* Should be derived from REF_FREQ + Signal */
#define STOPBAND_FREQ 91000
#define NTAPS 7		/* Number of filter taps */

float lpf_coeff[NTAPS];
struct lpf {
float history[NTAPS];
int last_index;
} lpf_I, lpf_Q;

float lpf(struct lpf *filter, float f){
float r;
int i, index;

filter->history[filter->last_index++] = f;
filter->last_index %= NTAPS;

r = 0;
index = filter->last_index;
for(i = 0; i< NTAPS; i++) {
  index--;
  if (index < 0) index = NTAPS - 1;
  r += filter->history[index] * lpf_coeff[i];
}
return r;
}

void setup_lpfs()
{
int i;
float d1, d2, fc;

fc = 2 * (float)CUTOFF_FREQ / (float)SAMPLE_RATE;
if (fc >= 1) {
  fprintf(stderr, "samplerate %d and cutoff freq %d violate nyquist\n", SAMPLE_RATE, CUTOFF_FREQ);
  exit(EXIT_FAILURE);
}
d1 = (NTAPS - 1) / 2;
for( i = 0; i < NTAPS; i++){
  d2 = i - d1;
  lpf_coeff[i] = (d2 == 0)? fc : sin(fc * M_PI * d2) / (M_PI * d2);
  lpf_I.history[i] = 0;
  lpf_Q.history[i] = 0;
}
/* Apply window function here */
lpf_I.last_index = 0;
lpf_Q.last_index = 0;
}

float period;    /* inverese of current frequency */
float phase;     /* phase offset */
float lock;      /* how well the signal is locked */
float ask;       /* quality of the signal */

float get_sample()
{
static float f = -SAMPLE_PERIOD * 2 * M_PI / REF_PERIOD;

f += SAMPLE_PERIOD * 2 * M_PI  / REF_PERIOD ;
if (f > 2 * M_PI) f -= 2 * M_PI;
return sin(f);
}

void get_IQ(float *I, float *Q)
{
static float f = -SAMPLE_PERIOD * 2 * M_PI / REF_PERIOD;

f += SAMPLE_PERIOD * 2 * M_PI  / period; 
if (f > 2 * M_PI) f -= 2 * M_PI;
*I = cos(f + phase);
*Q = -sin(f + phase);
}


int main(int argc, char **argv)
{
long long i;
float I,Q,S;
float S_I, S_Q, err;
float a,b; 

a = 0.05; /* phase adjustment (0..1) */
b = a * a * 0.25; /* freq adjustment) */
period = 19;
phase = .55 * 2 * M_PI;
lock = 0;
ask = 0;

setup_lpfs();

/* Main loop; to be called every SAMPLE_PERIOD */
for( i = 0; i < 1000; i++) {
S = get_sample();
get_IQ(&I, &Q);

printf("period %f, phase %f, sample %f, I %f, Q %f lock %f ask %f\n", period, phase, S, I, Q, lock, ask);

/* Multiply  & LPF */
S_I = lpf(&lpf_I, S * I);
S_Q = lpf(&lpf_Q, S * Q);

err = (S_I > 0)?  S_Q : -S_Q;
printf("S_I %f S_Q %f, err %f\n", S_I, S_Q, err);
phase += a * err;
period -= b * err;
#if 0
if (phase > 2 * M_PI) phase -= 2 * M_PI;
if (phase < -2 * M_PI) phase += 2 * M_PI;
#endif

S_I *= S_I;
S_Q *= S_Q;
lock = S_I - S_Q;
ask = S_I + S_Q;
}
}
