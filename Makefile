CC=gcc

all: VoipClient

VoipClient: VoipClient.c
	$(CC) VoipClient.c -o VoipClient -O3 -lpulse-simple -lpulse -pthread -lopus

clean:
	rm VoipClient
