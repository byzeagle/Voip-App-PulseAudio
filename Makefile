CC=gcc

all: VoipClient

VoipClient: VoipClient.c
	$(CC) VoipClient.c -o VoipClient -O3 -lpulse-simple -lpulse -pthread -lopus

new: new.c
	$(CC) -g new.c -o new -lpulse-simple -lpulse -lopus

clean:
	rm VoipClient new