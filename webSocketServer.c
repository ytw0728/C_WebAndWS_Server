#include "webSocketServer.h"

MYSQL * mysql_handle;
pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER; // 쓰레드 초기화

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
	client_data * client = (client_data*)args;
	int client_fd = client->fd;

	// need threading process
	shakehands(client_fd);	
	int exitCondition = 0;

	mysql_thread_init();
	// printf("fin=%d\nopcode=0x%X\nmask=%d\npayload_len=%llu\n",head.fin,head.opcode,head.mask,head.payload_length);

	//read payload data
	char payload_data_buffer[1024];
	char payload_data[8192];
	while(!exitCondition){


		// db 탐색해서 유저데이터 끌어오자
		memset(payload_data_buffer, 0 , sizeof(payload_data_buffer));
		memset(payload_data, 0, sizeof(payload_data));

		frame_head recvHead;
		int rul = recv_frame_head(client_fd,&recvHead);
		if (rul < 0){
			serverLog(WSSERVER,ERROR, "recv_frame_head error","wsconnect()");
			exitCondition = 1;
			continue;
		}
		int size = 0;
		int length = (int)recvHead.payload_length;
		do {
			int rul;
			rul = read(client_fd,payload_data_buffer,length);
			if (rul<=0)
				break;

			length -= rul;
			size+=rul;

			umask_setting(payload_data_buffer, size, recvHead.masking_key);
			serverLog(WSSERVER,FILELOG,"receive message",payload_data_buffer);


			strcat(payload_data, payload_data_buffer);


			// printf("\n\n\n %d \n\n\n", strcmp(payload_data, "\x41\x42\x43") ); // utf-8 이랑 비교 가능

			// if (write(client_fd,"\x47\x45\x54",3)<=0){ // utf-8 로 보내기 GET // 숫자 파싱은 그냥 0x30 더하면 댐
			// 	exitCondition = 1;
			// 	break;
			// }
		}while(!exitCondition && size < (int)recvHead.payload_length);    	
		if( recvHead.opcode == 0x8){
			serverLog(WSSERVER, LOG, "client requests to close", "");
			exitCondition = 1;
			continue;
		}

		frame_head sendHead = recvHead;

		struct packet p;
		json_to_packet(payload_data, &p);

		if( p.major_code == 0 ){
			switch( p.minor_code){
				case 0 : // 00
					exitCondition = drawingPoint(client, &sendHead, &p);
					break;
				case 1 : // 01
					exitCondition = validateChatMsg(client, &sendHead, &p);
					break;
					/*case 2 :
					  break;
					  case 3 : 
					  break;
					  case 4 :
					  break;*/
			}
		}
		else if( p.major_code == 1){
			switch( p.minor_code){
				case 0 : // 10
					exitCondition = waitingList(client, &sendHead, &p);
					break;
				case 1 : // 11
					exitCondition = requestEnterRoom(client, &sendHead, &p);
					break;
				case 4 : // 14
					exitCondition = userAdd(client, &sendHead, &p);
					break;
				case 5 : //15
					exitCondition = makeRoom(client, &sendHead, &p);
					break;
				case 6 : // 16
					exitCondition = exitRoom(client, &sendHead, &p);
					break;
				case 7 : // 17
					exitCondition = setScore(client, &sendHead, &p);
					break;
			}
		}

		if( p.ptr ) free(p.ptr);
		p.ptr = NULL;
	}

	deleteUser(client);

	serverLog(WSSERVER,FILELOG,"ws closed","\n");
	mysql_thread_end();
	close(client_fd);
	pthread_detach(client->thread_id);
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


	if( mysql_library_init(0, NULL, NULL) ){
		serverLog(WSSERVER, ERROR, "Can't load mysql library", "mysql_library_init()");
		goto EXIT;
	}


	if( (mysql_handle = mysql_init(NULL)) == 0 ){
		char logbuffer[1024];
		sprintf(logbuffer, "%u (%s): %s", mysql_errno(mysql_handle), mysql_sqlstate(mysql_handle), mysql_error(mysql_handle));
		serverLog(WSSERVER, ERROR, logbuffer, "mysql_init()");
		goto EXIT;
	}

	if( !mysql_thread_safe() ) { 
		serverLog(WSSERVER, ERROR, "mssql lib isn't safe for thread", "mysql_thread_safe()");
	} 


	if (!mysql_real_connect(mysql_handle,       /* MYSQL structure to use */
				DBHOST,         /* server hostname or IP address */ 
				DBUSER,         /* mysql user */
				DBPASSWD,          /* password */
				NULL,           /* default database to use, NULL for none */
				8890,           /* port number, 0 for default */
				NULL,        /* socket file or named pipe name */
				0 /* connection flags */ 
			       )){
		serverLog(WSSERVER, ERROR, "mysql error", "mysql_real_connect()");
		goto EXIT;
	}
	if( mysql_query(mysql_handle, "USE networkproject")){
		serverLog(WSSERVER, ERROR, "use networkproject", "mysql_query()");
		return -1;
	}

#ifndef DEV
	if( truncateDB() == -1){
		serverLog(WSSERVER, ERROR, "mysql error", "truncateDB()");
		goto EXIT;
	}
	printf("truncateDB success\n");
#endif


	pthread_mutex_init(&mutex, NULL);

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
		n->fd = client_fd;
		n->mutex = mutex;
		if( (pthread_create(&(n->thread_id), &pthread_attr, WSconnect, (void *) n)) < 0 ){
			serverLog(WSSERVER, ERROR, "Thread Error", "clientSocketThread");
		}
	}

	mysql_close(mysql_handle);
	mysql_library_end();
EXIT:
	pthread_mutex_destroy(&mutex);
	close(ser_fd);
	serverLog(WSSERVER, LOG, "SERVER END","");
	return NULL;
}




// db connection
MYSQL_RES * db_query(char *query, client_data * client , int type ){
	if( client == NULL ) pthread_mutex_lock(&mutex);
	else pthread_mutex_lock(&(client->mutex));
	serverLog(WSSERVER, LOG, query, "trying");
	if( mysql_query(mysql_handle, query) ){
		serverLog(WSSERVER, ERROR, query, "mysql_query()");
		if( client == NULL ) pthread_mutex_unlock(&mutex);
		else pthread_mutex_unlock(&(client->mutex));
		return -1;
	}

	if( type == SELECT ){
		MYSQL_RES * res = mysql_store_result(mysql_handle);
		if( client == NULL ) pthread_mutex_unlock(&mutex);
		else pthread_mutex_unlock(&(client->mutex));
		return res;
	}
	else{
		if( client == NULL ) pthread_mutex_unlock(&mutex);
		else pthread_mutex_unlock(&(client->mutex));

		return 0;
	}
}


// db truncateDB
int truncateDB(){
	MYSQL_RES * res;
	if( ( res = db_query((char*)"delete from `playerlist` where true", NULL, NONSELECT) ) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "DELETE PLAYERLIST" );
		return -1;
	}
	if( res ) mysql_free_result(res);

	if( ( res = db_query((char*)"delete from `gameroom` where true", NULL, NONSELECT ) ) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "DELETE GAMEROOM" );
		return -1;
	}
	if( res ) mysql_free_result(res);

	if( ( res = db_query((char*)"delete from `users` where true", NULL, NONSELECT ) ) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "DELETE USERS" );
		return -1;
	}
	if( res ) mysql_free_result(res);

	if( ( res = db_query((char*)"delete from `questions` where true", NULL, NONSELECT ) ) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "DELETE QUESTIONS" );
		return -1;
	}
	if( res ) mysql_free_result(res);

	if( (res = db_query((char*)"alter table `users` auto_increment = 1", NULL, NONSELECT)) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "alter users" );
		return -1;
	}
	if( res ) mysql_free_result(res);
	if( (res = db_query((char*)"alter table `questions` auto_increment = 1", NULL, NONSELECT)) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "alter questions" );
		return -1;
	}
	if( res ) mysql_free_result(res);
	if( (res = db_query((char*)"alter table `gameroom` auto_increment = 1", NULL, NONSELECT)) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "alter gameroom" );
		return -1;
	}
	if( res ) mysql_free_result(res);
	if( (res = db_query((char*)"alter table `playerlist` auto_increment = 1", NULL, NONSELECT)) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "alter playerlist" );
		return -1;
	}
	if( res ) mysql_free_result(res);

	int i = 0;
	for( ; i < 20 ; i++){
		if( (res = db_query((char*)"insert into `gameroom` values(0,0,NULL,NULL)", NULL, NONSELECT)) == -1 ){
			serverLog(WSSERVER,ERROR, "truncateDB error", "insert gameroom data" );
			return -1;
		}
	}

	return 0;
}



















// below lines are for actual procedure of web app.
void deleteUser(client_data * client){
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select * from `users` where fd = %d", client->fd);
	MYSQL_RES * result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "delete user failed","after db query(select)");
		return;
	}

	MYSQL_ROW row;
	row = mysql_fetch_row(result);
	
	if( row[0] == NULL ) return;

	int uid = atoi(row[0]);
	char nickname[NICKNAME_SIZE + 1];
	strcpy(nickname, row[1]);
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "select *, count(*) from `playerlist` where user_id = %d", uid );
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "delete user failed","after db query(delete)");
		return;
	}
	row = mysql_fetch_row(result);
	if( row[0] != NULL ){
		int room_id;
		room_id = atoi(row[1]);

		frame_head sendHead;
		memset(&sendHead, 0 , sizeof(frame_head));
		sendHead.fin = 1;
		sendHead.opcode = 0x1;

		struct packet p;
		p.major_code = 1;
		p.minor_code = 6;
		p.ptr = (REQUEST_EXIT_ROOM*)malloc(sizeof(REQUEST_EXIT_ROOM));

		((REQUEST_EXIT_ROOM*)(p.ptr))->room_id = room_id;
		((REQUEST_EXIT_ROOM*)(p.ptr))->from.uid = uid;
		strcpy(((REQUEST_EXIT_ROOM*)(p.ptr))->from.nickname, nickname);	

		if( result ){
			mysql_free_result(result);
			result = NULL;
		}
		client->fd = -1;
		if( exitRoom( client, &sendHead, &p) != 0){
			serverLog(WSSERVER,ERROR, "delete User error ","exitroom()");
			return;
		}
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}


	sprintf(queryBuffer, "delete from `users` where fd = %d", client->fd );
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "delete user failed","after db query(delete)");
		return;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}
}
// nickname
int userAdd(client_data * client, frame_head * sendHead, struct packet * p){
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "insert into `users` values(0, \'%s\',0,%d)", ((REQUEST_NICKNAME_REGISTER *)(p->ptr))->nickname, client->fd );
	MYSQL_RES * result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "user Add failed","after db query(insert)");
		goto USERADDFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "select * from `users` where fd = %d", client->fd);
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "user Add failed","after db query(select)");
		goto USERADDFAIL;
	}

	MYSQL_ROW row;
	row = mysql_fetch_row( result );

	struct packet sendPacket;
	sendPacket.major_code = 3;
	sendPacket.minor_code = 0;
	sendPacket.ptr = (RESPONSE_REGISTER*)malloc(sizeof(RESPONSE_REGISTER));

	((RESPONSE_REGISTER*)(sendPacket.ptr))->success = 1;
	((RESPONSE_REGISTER*)(sendPacket.ptr))->user.uid = atoi(row[0]);
	strcpy(((RESPONSE_REGISTER*)(sendPacket.ptr))->user.nickname, row[1]);
	((RESPONSE_REGISTER*)(sendPacket.ptr))->user.score = atoi(row[2]);

	if( result ){
		mysql_free_result(result);	
	} 

	const char * contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "user Add failed","packet sending error");
		goto USERADDFAIL;
	}


	free(sendPacket.ptr);
	free((char*)contents);	
	sendPacket.ptr = NULL;
	contents = NULL;

	return 0;

USERADDFAIL:
	sendPacket;
	sendPacket.major_code = 3;
	sendPacket.minor_code = 0;
	if( sendPacket.ptr ) free(sendPacket.ptr);
	sendPacket.ptr = (RESPONSE_REGISTER*)malloc(sizeof(RESPONSE_REGISTER));

	((RESPONSE_REGISTER*)(sendPacket.ptr))->success = 0;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "user Add failed","packet with failure sending error");
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	if( result ){
		mysql_free_result(result);	
	} 
	return 1;
}

int setScore(client_data * client, frame_head * sendHead, struct packet * p){ 
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "update `users` set `score`= %d where fd = %d", ((REQUEST_SET_SCORE *)(p->ptr))->score, client->fd );
	MYSQL_RES * result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "score set failed","after db query(update)");
		goto SETSCOREFAIL;
	}
	if( result ){
		mysql_free_result(result);	
	} 

	struct packet sendPacket;
	sendPacket.major_code = 3;
	sendPacket.minor_code = 1;
	sendPacket.ptr = (RESPONSE_SET_SCORE*)malloc(sizeof(RESPONSE_SET_SCORE));
	((RESPONSE_SET_SCORE*)(sendPacket.ptr))->success = 1;

	const char * contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "score set failed","packet sending error");
		goto SETSCOREFAIL;
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;

	return 0;


SETSCOREFAIL:
	sendPacket;
	sendPacket.major_code = 3;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr ) free(sendPacket.ptr);
	sendPacket.ptr = (RESPONSE_SET_SCORE*)malloc(sizeof(RESPONSE_SET_SCORE));
	((RESPONSE_SET_SCORE*)(sendPacket.ptr))->success = 0;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "score set failed","packet with failure sending error");
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	if( result ){
		mysql_free_result(result);	
	} 
	return 1;
}


// waiting list
int waitingList(client_data * client, frame_head * sendHead, struct packet * p){

	char queryBuffer[QUERY_SIZE] = 	"select gameroom.id, gameroom.status, gameroom.leader_id, count(playerlist.user_id) from gameroom "
									"left join playerlist on gameroom.id = playerlist.room_id "
									"where gameroom.status <> 0 and (gameroom.status/10) < 1";



	MYSQL_RES * result =  db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "get waiting list failed","after db query(select)");
		goto WAITINGLISTFAIL;
	}

	struct packet sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 0;
	sendPacket.ptr = (ROOM_LIST_DATA*)malloc(sizeof(ROOM_LIST_DATA));

	MYSQL_ROW row;
	int idx = 0;

	while( (row = mysql_fetch_row(result)) && idx < MAX_ROOM){
		if( row[0] == NULL ){
			break;
		}
		((ROOM_LIST_DATA*)(sendPacket.ptr))->rlist[idx].id = atoi(row[0]);
		((ROOM_LIST_DATA*)(sendPacket.ptr))->rlist[idx].status = atoi(row[1]);
		((ROOM_LIST_DATA*)(sendPacket.ptr))->rlist[idx].num = atoi(row[3]);
		idx++;
	}
	if( result ){
		mysql_free_result(result);
	}

	((ROOM_LIST_DATA*)(sendPacket.ptr))->idx = idx;
	((ROOM_LIST_DATA*)(sendPacket.ptr))->success = 1;
	const char * contents = packet_to_json(sendPacket);	
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "get waiting list failed","packet sending error");
		goto WAITINGLISTFAIL;
	}


	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	return 0;



WAITINGLISTFAIL:
	sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 0;
	if( sendPacket.ptr ) free( sendPacket.ptr);
	sendPacket.ptr = (ROOM_LIST_DATA*)malloc(sizeof(ROOM_LIST_DATA));
	((ROOM_LIST_DATA*)(sendPacket.ptr))->success = 0;
	((ROOM_LIST_DATA*)(sendPacket.ptr))->idx = 0;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "get waiting list failed","fail to send error packet");
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	if( result ){
		mysql_free_result(result);	
	} 
	return 1;
}

int requestEnterRoom(client_data * client, frame_head * sendHead, struct packet * p){
	/*미완성
	  todo
	  room data 갱신 해야함
	  user data 갱신 해야함
	 */
	char logbuffer[QUERY_SIZE];//tmp
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select gameroom.id, gameroom.status, gameroom.leader_id, count(playerlist.user_id)"
			"from gameroom "
			"left join playerlist on gameroom.id = playerlist.room_id "
			"where gameroom.id = %d && (gameroom.status/10) = 0;", ((REQUEST_ENTER_ROOM *)(p->ptr))->room_id);

	MYSQL_RES * result =  db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","after db query(select)");
		goto ENTERROOMFAIL;
	}

	struct packet sendPacket;
	sendPacket.major_code = 2; 
	sendPacket.minor_code = 1; 
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));

	((ROOM_DATA*)(sendPacket.ptr))->success = 1; 

	MYSQL_ROW row; 
	int idx = 0; 

	while( (row = mysql_fetch_row(result)) ){
		if( row[0] == NULL ){
			break;
		}
		((ROOM_DATA*)(sendPacket.ptr))->room.id = ((REQUEST_ENTER_ROOM *)(p->ptr))->room_id;
		((ROOM_DATA*)(sendPacket.ptr))->room.status = atoi(row[1]);
		((ROOM_DATA*)(sendPacket.ptr))->room.num = atoi(row[3]);
	}

	sprintf(logbuffer, "21.success%d room(#%d, %d, %d)", ((ROOM_DATA*)(sendPacket.ptr))->success, ((ROOM_DATA*)(sendPacket.ptr))->room.id, ((ROOM_DATA*)(sendPacket.ptr))->room.status, ((ROOM_DATA*)(sendPacket.ptr))->room.num );//debug
	serverLog(WSSERVER, LOG, logbuffer,"");

	sprintf(queryBuffer, "select users.id, users.nickname "
			"from users "
			"left join playerlist on playerlist.room_id = %d "
			"where playerlist.user_id = users.id;", ((REQUEST_ENTER_ROOM *)(p->ptr))->room_id);

	if( result ){
		mysql_free_result(result);
	}
	result =  db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","after db query for members(select)");
		goto ENTERROOMFAIL;
	}
	while( (row = mysql_fetch_row(result)) ){
		if( row[0] == NULL ){
			break;
		}
		((ROOM_DATA*)(sendPacket.ptr))->members[idx].uid = atoi(row[0]);
		strcpy(((ROOM_DATA*)(sendPacket.ptr))->members[idx].nickname, row[1]);
		idx++;
	}
	//((ROOM_DATA*)(sendPacket.ptr))->idx = idx;

	const char * contents = packet_to_json(sendPacket);

	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){

		serverLog(WSSERVER, ERROR, "entering waiting room failed","packet sending error");
		goto ENTERROOMFAIL;
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	if( result ){
		mysql_free_result(result);
	}
	return 0;

ENTERROOMFAIL:
	sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 1;
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));
	((ROOM_DATA*)(sendPacket.ptr))->success = 0;
	((ROOM_DATA*)(sendPacket.ptr))->idx = 0;


	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","packet sending error");
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	if( result ){
		mysql_free_result(result);
	}

	return 1;
}

int makeRoom(client_data * client, frame_head * sendHead, struct packet * p){
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select * from `gameroom` where status = 0");
	MYSQL_RES * result;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(select)");
		goto MAKEROOMFAIL;
	}
	MYSQL_ROW row;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto MAKEROOMFAIL; // all rooms are occupied;

	struct packet sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr) free( sendPacket.ptr);
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));

	((ROOM_DATA*)(sendPacket.ptr))->room.id = atoi(row[0]);
	((ROOM_DATA*)(sendPacket.ptr))->room.status = 1;
	if( result ){
		mysql_free_result(result);	
		result = NULL;
	}




	sprintf(queryBuffer, "update `gameroom` set status = 1, leader_id = %d where id = %d ",((MAKE_ROOM*)(p->ptr))->from.uid, ((ROOM_DATA*)(sendPacket.ptr))->room.id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(update)");
		goto MAKEROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "insert into playerlist values(0,%d,%d)",((ROOM_DATA*)(sendPacket.ptr))->room.id ,((MAKE_ROOM*)(p->ptr))->from.uid);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(insert)");
		goto MAKEROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where room_id = %d", ((ROOM_DATA*)(sendPacket.ptr))->room.id );
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(select)");
		goto MAKEROOMFAIL;
	}

	printf("11111111111111\n\n\n\n\n\n\n\n");
	int idx = 0;
	while( (row = mysql_fetch_row(result)) && idx < MAX_USER){
		if( row[0] == NULL ) break;
		((ROOM_DATA*)(sendPacket.ptr))->members[idx].uid = atoi(row[3]);
		strcpy(((ROOM_DATA*)(sendPacket.ptr))->members[idx].nickname, row[4]);
		((ROOM_DATA*)(sendPacket.ptr))->members[idx].score, atoi(row[5]);
		idx ++;
	}
	if( result ){
		mysql_free_result(result);
	}

	printf("22222222222222222\n\n\n\n\n\n\n\n");
	((ROOM_DATA*)(sendPacket.ptr))->idx = idx;
	((ROOM_DATA*)(sendPacket.ptr))->room.num = idx;
	((ROOM_DATA*)(sendPacket.ptr))->success = 1;

	const char * contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		goto MAKEROOMFAIL;
	}


	free((char*)contents);
	free(sendPacket.ptr);
	return 0;

MAKEROOMFAIL:
	sendPacket.major_code = 2;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr) free( sendPacket.ptr);
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));
	((ROOM_DATA*)(sendPacket.ptr))->idx = 0;
	((ROOM_DATA*)(sendPacket.ptr))->success = 0;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","fail to send error packet");
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;
	if( result ){
		mysql_free_result(result);	
	} 
	return 1;
}



// waiting room

int exitRoom(client_data * client, frame_head * sendHead, struct packet * p){
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "delete from playerlist where room_id = %d and user_id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id, ((REQUEST_EXIT_ROOM*)(p->ptr))->from.uid);
	MYSQL_RES * result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "exit room fail", "after db query(delete)");
		goto EXITROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "select *, count(*) from playerlist where room_id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
		goto EXITROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	int number = 0;
	MYSQL_ROW row;
	const char * contents;
	int size;

	/*
	   while( ( row = mysql_fetch_row(result) ) ){
	   if( row[0] == NULL ) break;

	   int fd = atoi(row[6]);
	   struct packet tmp;
	   tmp.major_code = 3;
	   tmp.minor_code = 2;
	   tmp.ptr = (POP_MEMBER_DATA*)malloc(sizeof(POP_MEMBER_DATA));
	   ((POP_MEMBER_DATA*)(tmp.ptr))->room.id = atoi(row[1]);
	   number = ((POP_MEMBER_DATA*)(tmp.ptr))->room.num = atoi(row[7]);
	   ((POP_MEMBER_DATA*)(tmp.ptr))->user.uid = ((REQUEST_EXIT_ROOM*)(p->ptr))->from.uid;
	   strcpy( ((POP_MEMBER_DATA*)(tmp.ptr))->user.nickname, ((REQUEST_EXIT_ROOM*)(p->ptr))->from.nickname );

	   contents = packet_to_json(tmp);
	   iso8859_1_to_utf8(contents, strlen(contents));
	   size = sendHead->payload_length = strlen(contents);
	   send_frame_head(fd, sendHead);
	   if( write( fd, contents, size) <= 0 ){
	   serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
	   goto EXITROOMFAIL;
	   }

	   free(tmp.ptr);
	   tmp.ptr = NULL;
	   }
	 */
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	if( number < 1 ){
		sprintf(queryBuffer, "delete from playerlist where room_id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(delete)");
			goto EXITROOMFAIL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}

		sprintf(queryBuffer, "delete from gameroom where id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(delete)");
			goto EXITROOMFAIL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}
	}
	else{
		int room_id = ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id;
		sprintf(queryBuffer, "select user_id from playerlist where room_id = %d", room_id);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
			goto EXITROOMFAIL;
		}
		row = mysql_fetch_row(result);
		int uid = atoi(row[0]);
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}

		sprintf(queryBuffer, "update from gameroom set leader_id = %d", uid);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
			goto EXITROOMFAIL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}

		sprintf(queryBuffer, "select * from gameroom left join users on gameroom.leader_id = users.id where gameroom.id = %d", room_id);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
			goto EXITROOMFAIL;
		}
		row = mysql_fetch_row(result);
		if( row[0] == NULL ) goto EXITROOMFAIL;

		int status = atoi(row[1]);
		int leader_id = atoi(row[2]);
		int answer_id = atoi(row[3]);
		char leader_nickname[NICKNAME_SIZE + 1];
		strcpy(leader_nickname, row[5]);

		if( result ){
			mysql_free_result(result);
			result = NULL;
		}


		sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where playerlist.room_id = %d", room_id);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
			goto EXITROOMFAIL;
		}


		/*
		   while( ( row = mysql_fetch_row(result)) ){
		   if( row[0] == NULL ) break;
		   int fd = atoi(row[6]);
		   struct packet tmp;
		   tmp.major_code = 3;
		   tmp.minor_code = 3;
		   tmp.ptr = (STATUS_CHANGED*)malloc(sizeof(STATUS_CHANGED));
		   ((STATUS_CHANGED*)(tmp.ptr))->room.id = atoi(row[1]);
		   ((STATUS_CHANGED*)(tmp.ptr))->room.num = atoi(number);
		   ((STATUS_CHANGED*)(tmp.ptr))->room.status = atoi(status);
		   ((STATUS_CHANGED*)(tmp.ptr))->leader.uid = leader_id;
		   strcpy(((STATUS_CHANGED*)(tmp.ptr))->leader.nickname, leader_nickname);


		   contents = packet_to_json(tmp);
		   iso8859_1_to_utf8(contents, strlen(contents));
		   size = sendHead->payload_length = strlen(contents);
		   send_frame_head(fd, sendHead);
		   if( write( fd, contents, size) <= 0 ){
		   serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		   goto EXITROOMFAIL;
		   }

		   free(tmp.ptr);
		   tmp.ptr = NULL;
		   }
		 */
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}
	}
	if( result ){
		mysql_free_result(result);
	} 


	if( client->fd == -1 ){	 // when triggered by deleteUser
		if( result ){
			mysql_free_result(result);
		} 
		return 0;
	}

	struct packet sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 8;
	sendPacket.ptr = (RESPONSE_EXIT*)malloc(sizeof(RESPONSE_EXIT));
	((RESPONSE_EXIT*)(sendPacket.ptr))->success = 1;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);
	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		goto EXITROOMFAIL;
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;

	return 0;
EXITROOMFAIL:
	sendPacket.major_code = 2;
	sendPacket.minor_code = 8;
	if( sendPacket.ptr ) free(sendPacket.ptr);
	sendPacket.ptr = (RESPONSE_EXIT*)malloc(sizeof(RESPONSE_EXIT));
	((RESPONSE_EXIT*)(sendPacket.ptr))->success = 0;


	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);
	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		goto EXITROOMFAIL;
	}

	free(sendPacket.ptr);
	free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;

	if( result ){
		mysql_free_result(result);	
	} 
	return 1;
}

// gameroom
int drawingPoint(client_data * client, frame_head * sendHead, struct packet * p){
	// struct packet sendPacket = *p;
	const char * contents = packet_to_json(*p);

	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size ) <= 0){
		free((char*)contents);
		return 1;
	}	
	free((char*)contents);
	return 0;
}
int validateChatMsg(client_data * client, frame_head * sendHead, struct packet * p){
	// struct packet sendPacket = *p;
	int isCorrect = 0;

	const char * contents = packet_to_json(*p);

	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size ) <= 0){
		free((char*)contents);
		return 1;
	}
	// some statement for validate the msg which it equals answer.
	if( isCorrect ){

	}
	else{

	}
	free((char*)contents);
	return 0;
}
