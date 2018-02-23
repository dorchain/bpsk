
all: bpsk

bpsk: bpsk.c
	gcc $< -lm -o $@
