all: clean bin

bin: main.c 
	cc -Wall -o codec2-merge-linux-x86 *.c -static  -lm
clean:
	rm -f codec2-merge-linux-x86

# -fno-omit-frame-pointer -lm
