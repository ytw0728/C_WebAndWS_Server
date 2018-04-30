#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "Includes.h"

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define BUFFER_SIZE 1024
#define RESPONSE_HEADER_LEN_MAX 1024
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WSPORT 12345

typedef struct _frame_head {
	char fin;
	char opcode;
	char mask;
	unsigned long long payload_length;
	char masking_key[4];
} frame_head;



int base64_encode(unsigned char *in_str, int in_len, char *out_str);
int _readline(char* allbuf,int level,char* linebuf);
int shakehands(int cli_fd);
int recv_frame_head(int fd,frame_head* head);
void umask_setting(char *data,int len,char *mask);

int send_frame_head(int fd,frame_head* head);
// int passive_server( int port, int iDontKnowButIGuessItIsProtocol);
void *webSocketServerHandle();



#endif