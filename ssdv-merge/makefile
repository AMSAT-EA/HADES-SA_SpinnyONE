all: clean bin

bin: main.c *.h
	cc -Wall -o ssdv-merge-linux-x86 *.c -static  -lm
clean:
	rm -f ssdv-merge-linux-x86

# -fno-omit-frame-pointer -lm
