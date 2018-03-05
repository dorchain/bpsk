/*
 * Copyright 2018 Joerg Dorchain
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Joerg Dorchain wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 */


/*
 * Some theorie of operation
 * This is a bpsk-phase-locked-loop receiver/demodulator based on
 * costas loop.
 *
 * Feedback value is taken from expirience and testing
 * 
 * Low Pass Filter is an FIR Filter:
 * Linear pahse, computes fast and errors are not summing up.
 *
 * In simulation, it catches the proper frequency between
 * ~48-58kHz within 10000 iterations
 */

/*
 * TODO:
 * - Check LPR FIlter cutoff: 2*Bitrate should be ok.
 * - Sample interpolation, at best triangle to get the peaks
 * - Use bandpass sampling to greatly reduce the required
 *   sampling rate
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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

/* LPF Filter features */
#define CUTOFF_FREQ (REF_FREQ + 3 * BIT_RATE) /* 55921.05263157894736842104 for some headroom */
#define STOPBAND_FREQ (REF_FREQ - 12 * BIT_RATE) /* 92105.26315789473684210526 for some headroom */
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
float phase;     /* current phase offset */
float lock;      /* how well the signal is locked */
float ask;       /* quality of the signal */
unsigned int wavecounter = 0;
char oldsign = 0;
int locked = 0;

float get_sample()
{
float S;
int r;

r = read(0, &S, sizeof(S));
if (r == 0) {
  printf("Enf of input reached\n");
  exit(EXIT_SUCCESS);
}
if (r < 0) {
  printf("Error reading input %s\n", strerror(errno));
  exit(EXIT_FAILURE);
}
if (r != sizeof(S)) {
  printf("unexspected number of input bytes: %d\n", r);
  exit(EXIT_FAILURE);
}
return S;
}

void get_IQ(float *I, float *Q)
{
static float f = -SAMPLE_PERIOD * 2 * M_PI / REF_PERIOD;

f += SAMPLE_PERIOD * 2 * M_PI  / period; 
if (f > 2 * M_PI) {
  f -= 2 * M_PI;
  wavecounter++;
}
*I = cos(f + phase);
*Q = -sin(f + phase);
}


int main(int argc, char **argv)
{
long long i;
float I,Q,S;
float S_I, S_Q, err, err_int;
float a,b,intgralf; 
char shortcounter = 0;
char sign, oldsign = 0;

printf("Call with sox <signal.wav> -t f32 - | %s\n", argv[0]);

a = 0.2; /* phase adjustment (0..1) */
b = a * a * 0.25; /* freq adjustment) */
intgralf = 1.2 * a / REF_PERIOD;
period = REF_PERIOD;
phase = .55 * 2 * M_PI;
lock = 0;
ask = 0;

setup_lpfs();

err_int = 0;
/* Main loop; to be called every SAMPLE_PERIOD */
printf(" period, phase, sample, I, Q, lock, ask, ");
printf("S_I, S_Q, err, err_int\n");
for( i = 0; i < 10000; i++) {

S = get_sample();
get_IQ(&I, &Q);
printf("%f, %f, %f, %f, %f, %f, %f, ", period, phase, S, I, Q, lock, ask);

/* Multiply  & LPF */
S_I = lpf(&lpf_I, S * I);
S_Q = lpf(&lpf_Q, S * Q);

/* discriminator function: use least computional effort, good for high S/N */
err = (S_I > 0)?  S_Q : -S_Q; /* sign(S_I) * S_Q, simpler than atan() and works as well */
printf("%f, %f, %f, %f\n", S_I, S_Q, err, err_int);
/* apply P-I filter; values from Ziegler-Nichols */
err_int += err * SAMPLE_PERIOD;
err += err_int * intgralf;
phase += a * err;
period -= b * err;
#if 0
if (phase > 2 * M_PI) phase -= 2 * M_PI;
if (phase < -2 * M_PI) phase += 2 * M_PI;
#endif

lock = (S_I * S_I) - (S_Q * S_Q);
ask = (S_I * S_I) - (S_Q * S_Q);

/* Decoder output */
if (lock > 0.15 ) {
  if (!locked) {
    locked = 1;
    printf("Locked\n");
  }
} else {
  if (locked) {
    locked = 0;
    printf("Lost Lock %f\n",lock);
  }
}
if (locked && (fabs(S_I) > 0.38)) {
  sign = (S_I > 0)? 1:-1;
  if (oldsign != sign) {
printf("wavecounter: %d\n", wavecounter);
    if (wavecounter > (PERIODS_PER_CLOCK + PERIODS_PER_BIT) / 2) {
      if (shortcounter > 0) {
        printf("single short transition!\n");
	shortcounter = 0;
      }
      printf("Received bit: 1\n");
    } else {
      shortcounter++;
      if (shortcounter == 2) {
        printf("Received bit: 0\n");
	shortcounter = 0;
      }
    }
    oldsign = sign;
    wavecounter = 0;
  }
}

}
}
