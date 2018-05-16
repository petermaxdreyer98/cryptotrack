all:
	cc main.c -o ./cryptotrack -lcurl
clean:
	rm ./cryptotrack
