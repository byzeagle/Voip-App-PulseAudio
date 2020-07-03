#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

/*
*   Copyright 2020
*   Author : Ugur Buyukdurak
*   Version : 1.0
*/

#include <pulse/simple.h>
#include <pulse/error.h>

#define PORT 8080
#define BUFFERSIZE 1024

static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 2
};

void control_s(pa_simple *s)
{
    if(s)
        pa_simple_free(s);
}

int create_UDP_socket_send(struct sockaddr_in * server, void * address)
{
    int sock;
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    memset(server, 0, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_port = htons(PORT);
    server->sin_addr.s_addr = inet_addr((const char *)address);

    return sock;
}

int create_UDP_socket_receive()
{
    struct sockaddr_in server;
    int sock;
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    return sock;
}

void* recorder(void * address)
{
    pa_simple *s = NULL;
    int error;
    int nsent;

    struct sockaddr_in server;
    int socket = create_UDP_socket_send(&server, address);


    /* Create the recording stream */
    if (!(s = pa_simple_new(NULL, "Voip", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        control_s(s);
        exit(EXIT_FAILURE);
    }

    uint8_t buf[BUFFERSIZE];

    for (;;) 
    {
    	if(pa_simple_read(s, buf, sizeof(buf), &error) < 0)
    	{
    		fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
    		control_s(s);
        	exit(EXIT_FAILURE);
    	}

    	if((nsent = sendto(socket, buf, sizeof(buf), 0, (struct sockaddr *)&server, sizeof(server))) < 0)
		{
			perror("sendto()");
			control_s(s);
			exit(EXIT_FAILURE);
		}
    }

    pthread_exit(NULL);
}

void* player()
{
    pa_simple *s = NULL;
    int nread;
    int error;

    int socket = create_UDP_socket_receive();

    if (!(s = pa_simple_new(NULL, "Voip", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        control_s(s);
        exit(EXIT_FAILURE);
    }

    uint8_t buf[BUFFERSIZE];

    for (;;) 
    {
    	if((nread = recvfrom(socket, buf, sizeof(buf), 0, NULL, NULL)) < 0)
		{
			perror("recvfrom()");
            control_s(s);
			exit(EXIT_FAILURE);
		}
		
		if(pa_simple_write(s, buf, sizeof(buf), &error) < 0)
		{
			fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
			control_s(s);
        	exit(EXIT_FAILURE);
		}
    }

    pthread_exit(NULL);
}

int main(int argc, char * argv[])
{
    pthread_t thread_send, thread_receive;
	int rc1, rc2;

    if(argc < 2)
    {
        fprintf(stderr, "An ip-address must be provided\n");
        exit(EXIT_FAILURE);
    }

    if( (rc1 = pthread_create(&thread_send, NULL, recorder, argv[1])))
    {
        fprintf(stderr, "Thread1 creation failed\n");
        exit(EXIT_FAILURE);
    }

    if((rc2 = pthread_create(&thread_receive, NULL, player, NULL)))
    {
        fprintf(stderr, "Thread2 creation failed\n");
        exit(EXIT_FAILURE);
    }    

    pthread_join(thread_send, NULL);
    pthread_join(thread_receive, NULL);

	pthread_exit(NULL);
}