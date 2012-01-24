#CC=gcc
CC=clang

markov: markov.c markov.h Makefile
	$(CC) -g --std=c99 -O4 markov.c -static -o markov
