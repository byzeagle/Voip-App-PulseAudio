all: VoipClient

VoipClient: VoipClient.c
	gcc VoipClient.c -o VoipClient -lpulse-simple -lpulse -pthread

clean:
	rm VoipClient