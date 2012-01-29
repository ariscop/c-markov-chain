CC=gcc
#CC=clang

markov: markov.c markov.h Makefile
	$(CC) --std=c99 -O4 markov.c -static -o markov -pg
