#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include "Includes.h"
#include "message.h"


// #include "utf8.h"

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <mysql/mysql.h>

#define BUFFER_SIZE 1024
#define RESPONSE_HEADER_LEN_MAX 1024
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WSPORT 8889
#define DBPORT 8890
#define DBHOST "sccc.kr"
#define DBUSER "root"
#define DBPASSWD "root"
#define DBNAME "networkproject"

#define QUERY_SIZE 8192

typedef struct _frame_head {
	unsigned char fin;
	unsigned char rev[3];
	unsigned char opcode;
	unsigned char mask;
	unsigned long long payload_length;
	unsigned char masking_key[4];
} frame_head;

typedef struct WSclient_data{
	int fd;	
	pthread_t thread_id;
	pthread_mutex_t mutex;
} client_data;


int base64_encode(unsigned char *in_str, int in_len, char *out_str);
int _readline(char* allbuf,int level,char* linebuf);
int shakehands(int cli_fd);
int recv_frame_head(int fd,frame_head* head);
void umask_setting(char *data,int len,char *mask);

int send_frame_head(int fd,frame_head* head);
// int passive_server( int port, int iDontKnowButIGuessItIsProtocol);
void *Wsconnect(void *args);
void *webSocketServerHandle();



#endif