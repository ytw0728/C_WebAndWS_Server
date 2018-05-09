#include "webSocketServer.h"

/*-------------------------------------------------------------------
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-------+-+-------------+-------------------------------+
  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
  |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
  | |1|2|3|       |K|             |                               |
  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
  |     Extended payload length continued, if payload len == 127  |
  + - - - - - - - - - - - - - - - +-------------------------------+
  |                               |Masking-key, if MASK set to 1  |
  +-------------------------------+-------------------------------+
  | Masking-key (continued)       |          Payload Data         |
  +-------------------------------- - - - - - - - - - - - - - - - +
  :                     Payload Data continued ...                :
  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
  |                     Payload Data continued ...                |
  +---------------------------------------------------------------+
  --------------------------------------------------------------------*/
size_t iso8859_1_to_utf8(char *content, size_t max_size)
{
    char *src, *dst;

    //first run to see if there's enough space for the new bytes
    for (src = dst = content; *src; src++, dst++)
    {
        if (*src & 0x80)
        {
            // If the high bit is set in the ISO-8859-1 representation, then
            // the UTF-8 representation requires two bytes (one more than usual).
            ++dst;
        }
    }

    if (dst - content + 1 > max_size)
    {
        // Inform caller of the space required
        return dst - content + 1;
    }

    *(dst + 1) = '\0';
    while (dst > src)
    {
        if (*src & 0x80)
        {
            *dst-- = 0x80 | (*src & 0x3f);                     // trailing byte
            *dst-- = 0xc0 | (*((unsigned char *)src--) >> 6);  // leading byte
        }
        else
        {
            *dst-- = *src--;
        }
    }
    return 0;  // SUCCESS
}



int base64_encode(unsigned char *in_str, int in_len, char *out_str){
	BIO *b64, *bio;
	BUF_MEM *bptr = NULL;
	size_t size = 0;

	if (in_str == NULL || out_str == NULL)
		return -1;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_write(bio, in_str, in_len);
	BIO_flush(bio);

	BIO_get_mem_ptr(bio, &bptr);
	memcpy(out_str, bptr->data, bptr->length);
	out_str[bptr->length-1] = '\0';
	size = bptr->length;

	BIO_free_all(bio);
	return size;
}

/**
 * @brief _readline
 * read a line string from all buffer
 * @param allbuf
 * @param level
 * @param linebuf
 * @return
 */
int _readline(char* allbuf,int level,char* linebuf){
	int len = strlen(allbuf);
	for (;level<len;++level){
		if(allbuf[level]=='\r' && allbuf[level+1]=='\n')
			return level+2;
		else
			*(linebuf++) = allbuf[level];
	}
	return -1;
}

int shakehands(int cli_fd){
	//next line's point num
	int level = 0;
	//all request data
	char buffer[BUFFER_SIZE];
	//a line data
	char linebuf[256];
	//Sec-WebSocket-Accept
	char sec_accept[32];
	//sha1 data
	unsigned char sha1_data[SHA_DIGEST_LENGTH+1]={0};
	//reponse head buffer
	char head[BUFFER_SIZE] = {0};

	if (read(cli_fd,buffer,sizeof(buffer))<=0)
		perror("read");
	// printf("request\n"); // packet data print
	// printf("%s\n",buffer); // 

	do {
		memset(linebuf,0,sizeof(linebuf));
		level = _readline(buffer,level,linebuf);
		// printf("line:%s\n",linebuf); // line buffer print

		if (strstr(linebuf,"Sec-WebSocket-Key")!=NULL)
		{
			strcat(linebuf,GUID);
			//            printf("key:%s\nlen=%d\n",linebuf+19,strlen(linebuf+19));
			SHA1((unsigned char*)&linebuf+19,strlen(linebuf+19),(unsigned char*)&sha1_data);
			//            printf("sha1:%s\n",sha1_data);
			base64_encode(sha1_data,strlen((const char*)sha1_data),sec_accept);
			//            printf("base64:%s\n",sec_accept);
			/* write the response */
			sprintf(head, "HTTP/1.1 101 Switching Protocols\r\n" \
					"Upgrade: websocket\r\n" \
					"Connection: Upgrade\r\n" \
					"Sec-WebSocket-Accept: %s\r\n" \
					"\r\n",sec_accept);

			// printf("response\n"); // response header print
			// printf("%s",head);
			if (write(cli_fd,head,strlen(head))<0)
				perror("write");

			break;
		}
	}while((buffer[level]!='\r' || buffer[level+1]!='\n') && level!=-1);
	return 0;
}


int recv_frame_head(int fd, frame_head* head){
	char one_char;
	/*read fin and op code*/
	if (read(fd,&one_char,1)<=0){
		perror("read fin");
		return -1;
	}

	head->fin = (one_char & 0x80) == 0x80;

	head->rev[0] = (one_char & 0x40 ) == 0x40;
	head->rev[1] = (one_char & 0x20 ) == 0x20;
	head->rev[2] = (one_char & 0x10 ) == 0x10;

	head->opcode = one_char & 0x0F;

	if( head->opcode == 0x8){

	}

	if (read(fd,&one_char,1)<=0){
		perror("read mask");
		return -1;
	}

	head->mask = (one_char & 0x80) == 0X80;

	/*get payload length*/
	head->payload_length = one_char & 0x7F;

	if (head->payload_length == 126){
		char extern_len[2];
		if (read(fd,extern_len,2)<=0)
		{
			perror("read extern_len");
			return -1;
		}

		head->payload_length = (extern_len[0]&0xFF) << 8 | (extern_len[1]&0xFF);
	}
	else if (head->payload_length == 127){
		char extern_len[8],temp;
		int i;
		if (read(fd,extern_len,8)<=0)
		{
			perror("read extern_len");
			return -1;
		}

		for(i=0;i<4;i++)
		{
			temp = extern_len[i];
			extern_len[i] = extern_len[7-i];
			extern_len[7-i] = temp;
		}

		memcpy(&(head->payload_length),extern_len,8);
	}

	/*read masking-key*/
	if (read(fd,head->masking_key,4)<=0){
		perror("read masking-key");
		return -1;
	}
	


	return 0;
}

/**
 * @brief umask_setting
 * xor decode
 * @param data
 * @param len
 * @param mask
 */
void umask_setting(char *data,int len,char *mask){
	int i;
	for (i=0;i<len;++i)
		*(data+i) ^= *(mask+(i%4));
}

int send_frame_head(int fd,frame_head* head){
	unsigned char *response_head;
	int head_length = 0;
	if(head->payload_length<126){
		response_head = (char*)malloc(2);
		response_head[0] = 0x81; // fin : 1 and opcode : 0x1
		response_head[1] = head->payload_length;
		head_length = 2;
	}
	else if (head->payload_length<0xFFFF){
		response_head = (char*)malloc(4);
		response_head[0] = 0x81; // fin : 1 and opcode : 0x1
		response_head[1] = 126;
		response_head[2] = (head->payload_length >> 8 & 0xFF);
		response_head[3] = (head->payload_length & 0xFF);
		head_length = 4;
	}
	else{
		//no code
		response_head = (char*)malloc(12);
		response_head[0] = 0x81; // fin : 1 and opcode : 0x1
		response_head[1] = 127;
		response_head[2] = (head->payload_length >> 56 & 0xFF);
		response_head[3] = (head->payload_length >> 48 & 0xFF);
		response_head[4] = (head->payload_length >> 40 & 0xFF);
		response_head[5] = (head->payload_length >> 32 & 0xFF);
		response_head[6] = (head->payload_length >> 24 & 0xFF);
		response_head[7] = (head->payload_length >> 16 & 0xFF);
		response_head[8] = (head->payload_length >> 8 & 0xFF);
		response_head[9] = (head->payload_length & 0xFF);
		// head_length = 12;
		head_length = 10;
	}

	if(write(fd,response_head,head_length)<=0){
		perror("write head");
		return -1;
	}

	free(response_head);
	return 0;
}

void *WSconnect(void* args){
	int client_fd = (int)(*(int*)args);
	// need threading process

	shakehands(client_fd);	
    int exitCondition = 0;

    frame_head head;
    int rul = recv_frame_head(client_fd,&head);
    if (rul < 0){
    	printf("recv_frame_head error\n");
    	exitCondition = 1;
    }

    // printf("fin=%d\nopcode=0x%X\nmask=%d\npayload_len=%llu\n",head.fin,head.opcode,head.mask,head.payload_length);

    char payload_data[1024] = {0};
    int size = 0;
    // db 탐색해서 유저데이터 끌어오자


    //echo head
    if( !exitCondition ){
		send_frame_head(client_fd,&head);
	    

		do {
			int rul;
			rul = read(client_fd,payload_data,1024);
			if (rul<=0){
				exitCondition = 1;
				break;
			}
			size+=rul;

		   	umask_setting(payload_data,size,head.masking_key);

		    // echo data
		    if (write(client_fd,payload_data,rul)<=0){
		    	exitCondition = 1;
		    	break;
		    }
		}while(size < (int)head.payload_length);
	}


    while(!exitCondition){

		//read payload data
	    char payload_data[1024] = {0};
	    int size = 0;
	    // db 탐색해서 유저데이터 끌어오자

	    frame_head head;
	    int rul = recv_frame_head(client_fd,&head);
	    if (rul < 0){
	    	printf("recv_frame_head error\n");
	    	exitCondition = 1;
	    	continue;
	    }


	    //echo head
	    send_frame_head(client_fd,&head);
		

    	do {
			int rul;
			rul = read(client_fd,payload_data,1024);
			if (rul<=0)
				break;
			size+=rul;

	   		umask_setting(payload_data,size,head.masking_key);
	   		printf("recive:%s\n",payload_data);

		    //echo data // 만약에 문자열을 직접 보내고 싶으면 utf8로 인코딩하면됩니다.
		    if (write(client_fd,payload_data,rul)<=0){
		    	exitCondition = 1;
		    }
    	}while(!exitCondition && size< (int)head.payload_length);
	}


	printf("close ws\n");
	close(client_fd);
}

void *webSocketServerHandle(){
	serverLog(WSSERVER, LOG, "WEBSOCKET SERVER FUNCTION START", "MESSAGE");

	int sock;
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) < -1 ){
		serverLog(WSSERVER, ERROR, "system call error", "socket()");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr) );
	server_addr.sin_family 		= AF_INET;
	server_addr.sin_port 		= htons(WSPORT);
	server_addr.sin_addr.s_addr 	= htonl( INADDR_ANY);

	if( bind( sock, (struct sockaddr*)&server_addr, sizeof( server_addr)) < -1  ){
		serverLog(WSSERVER, ERROR, "system call error", "bind()");
		exit(EXIT_FAILURE);
	}
	
	if( listen(sock, 64) < 0 ){
		serverLog(WSSERVER, ERROR, "system call error", "listen()");
		exit(EXIT_FAILURE);
	}

	serverLog(WSSERVER, LOG, "WebSocketServer Start", "MESSAGE");

	int ser_fd = sock;

	struct sockaddr_in client_addr;
	socklen_t addr_length = sizeof(client_addr);
	
	
	pthread_attr_t pthread_attr;
	pthread_t pthread_id;

	pthread_attr_init(&pthread_attr);
	pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&pthread_attr, 524288);

    while (1){
    	int client_fd = accept(ser_fd,(struct sockaddr*)&client_addr, &addr_length);
		if( client_fd == -1 ) serverLog(WSSERVER,ERROR,"system call error","accept()");

		serverLog(WSSERVER,LOG,"connect","accept()");
		client_data* n;
		n = (client_data*)malloc(sizeof(client_data));


		if( (pthread_create(&(n->thread_id), &pthread_attr, WSconnect, (void *) &client_fd)) < 0 ){
			serverLog(WEBSERVER, ERROR, "Thread Error", "");
		}
    }
    close(ser_fd);

    return NULL;
}