all:
	c99 main.c -o ./cryptotrack -lcurl
clean:
	rm ./cryptotrack
