all: clean bin

bin: main.c genesis_crc.c genesis_scrambler.c *.h
	cc -Wall -o unified-bytes-decoder-hades-sa-x86 *.c -static  -lm
clean:
	rm -f unified-bytes-decoder-hades-sa-x86

