#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <opus/opus.h>

#define PORT 11000

/*
*   Simple Voip Program
*   Author: Ugur Buyukdurak
*   Copyright: 2020
*/

#define BUFFERSIZE 1024
#define FRAME_SIZE 960
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define MAX_FRAME_SIZE 6 * FRAME_SIZE
#define MAX_PACKET_SIZE (BUFFERSIZE * 4)
#define APPLICATION OPUS_APPLICATION_VOIP

struct Data
{
    int payload_length;
    uint8_t cbits[MAX_PACKET_SIZE];
} __attribute__((__packed__));

typedef struct Data Data;

static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 48000,
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

    int err;
    OpusEncoder *enc;

    enc = opus_encoder_create(SAMPLE_RATE, CHANNELS, APPLICATION, &err);
    if (err < 0)
    {
        fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
        exit(EXIT_FAILURE);
    }

    opus_int16 in[FRAME_SIZE * CHANNELS];
    uint8_t cbits[MAX_PACKET_SIZE];
    uint8_t buf[FRAME_SIZE * CHANNELS * 2];

    int nBytes;
    Data data;

    for (;;) 
    {
     if(pa_simple_read(s, buf, sizeof(buf), &error) < 0)
     {
        fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
        control_s(s);
        exit(EXIT_FAILURE);
    }

    memcpy(in, buf, sizeof(buf));
    nBytes = opus_encode(enc, in, FRAME_SIZE, cbits, MAX_PACKET_SIZE);
    if(nBytes < 0)
    {
        fprintf(stderr, "Error opus_encode\n");
        exit(EXIT_FAILURE);
    }

    data.payload_length = nBytes;
    memcpy(data.cbits, cbits, sizeof(uint8_t) * nBytes);

    if((nsent = sendto(socket, &data, sizeof(data.payload_length) + nBytes, 0, (struct sockaddr *)&server, sizeof(server))) < 0)
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

    int err;
    OpusDecoder *dec;
    dec = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (err < 0)
    {
        fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
        exit(EXIT_FAILURE);
    }

    opus_int16 out[FRAME_SIZE * CHANNELS];
    uint8_t cbits[MAX_PACKET_SIZE];
    uint8_t buf[FRAME_SIZE * CHANNELS * 2];

    int nBytes;
    Data data;

    for (;;) 
    {
        if((nread = recvfrom(socket, &data, sizeof(data), 0, NULL, NULL)) < 0)
        {
            perror("recvfrom()");
            control_s(s);
            exit(EXIT_FAILURE);
        }

        int payload = data.payload_length;
        if(nread != sizeof(data.payload_length) + payload){
            fprintf(stderr, "Error receiving the packet\n");
            exit(EXIT_FAILURE);
        }

        nBytes = opus_decode(dec, data.cbits, data.payload_length, out, MAX_FRAME_SIZE, 0);
        if(nBytes < 0)
        {
            fprintf(stderr, "Error opus_decode\n");
            exit(EXIT_FAILURE);
        }

        memcpy(buf, out, sizeof(out));

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

    printf("Port used is: %d\n", PORT);

    pthread_join(thread_send, NULL);
    pthread_join(thread_receive, NULL);

    pthread_exit(NULL);
}