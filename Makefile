
all: bpsk

bpsk: bpsk.c
	gcc $< -lm -o $@

signalgen: signalgen.c
	gcc $< -lm -o $@

signal.wav: signalgen
	./signalgen | sox -c 1 -r 192000 -b 32 -e floating-point -t raw - $@
