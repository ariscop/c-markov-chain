
markov: markov.c markov.h Makefile
	clang -g -O0 markov.c -static -o markov
