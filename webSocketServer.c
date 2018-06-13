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
	unsigned char *response_head = NULL;
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

	if(response_head){
		free(response_head);
		response_head = NULL;
	}
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
	// const char * payload_data = (const char *)malloc(sizeof(char) * 8192);

	while(!exitCondition){


		// db 탐색해서 유저데이터 끌어오자
		memset(payload_data_buffer, 0 , sizeof(payload_data_buffer));
		memset(payload_data, 0, sizeof(char) * 8192);

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
		p.ptr = NULL;
		memset(&p, 0x00, sizeof(struct packet));
		json_to_packet(payload_data, &p);

		if( p.major_code == 0 ){
			switch( p.minor_code){
				case 0 : // 00
					exitCondition = drawingPoint(client, &sendHead, &p);
					break;
				case 1 : // 01
					exitCondition = echoChatData(client, &sendHead, &p);
					break;
				case 2 : // 02
					exitCondition = startDrawing(client, &sendHead, &p);
					break;
				case 3 : // 03
					exitCondition = endDrawing(client, &sendHead, &p);
					break;
				case 4 : // 04
					exitCondition = shareTime(client, &sendHead, &p);
					break;
			}
		}
		else if( p.major_code == 1){
			switch( p.minor_code){
				case 0 : // 10
					exitCondition = waitingList(client, &sendHead, &p);
					break;
				case 2 : // 12
					exitCondition = enterGameRoom(client, &sendHead, &p);
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

		if( p.ptr ){
			free(p.ptr);
			p.ptr = NULL;
		}
	}

	int fd = client->fd;
	deleteUser(client);

	serverLog(WSSERVER,FILELOG,"ws closed","\n");
	mysql_thread_end();
	close(fd);
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


	// mysql_options(mysql_handle, MYSQL_SET_CHARSET_NAME, "utf81");
	mysql_options(mysql_handle, MYSQL_INIT_COMMAND, "SET NAMES utf8");


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
	MYSQL_RES * res = NULL;
	if( ( res = db_query((char*)"delete from `playerlist` where true", NULL, NONSELECT) ) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "DELETE PLAYERLIST" );
		return -1;
	}
	if( res ) mysql_free_result(res);
	if( ( res = db_query((char*)"delete from `painters` where true", NULL, NONSELECT ) ) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "DELETE PAINTERS" );
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


	if( (res = db_query((char*)"alter table `users` auto_increment = 1", NULL, NONSELECT)) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "alter users" );
		return -1;
	}
	if( res ) mysql_free_result(res);

	if( (res = db_query((char*)"alter table `painters` auto_increment = 1", NULL, NONSELECT)) == -1 ){
		serverLog(WSSERVER,ERROR, "truncateDB error", "alter painters" );
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
	for( ; i < MAX_ROOM ; i++){
		if( (res = db_query((char*)"insert into `gameroom` values(0,0,NULL,NULL)", NULL, NONSELECT)) == -1 ){
			serverLog(WSSERVER,ERROR, "truncateDB error", "insert gameroom data" );
			return -1;
		}
	}
	for( i = 1 ; i <= MAX_ROOM ; i++){
		char queryBuffer[QUERY_SIZE];
		sprintf(queryBuffer, "insert into painters values(0, %d, NULL)", i);
		if( (res = db_query(queryBuffer, NULL, NONSELECT))  == -1 ){
			serverLog(WSSERVER,ERROR, "truncateDB error", "insert painters data" );
			return -1;
		}
	}

	return 0;
}



















// below lines are for actual procedure of web app.

void deleteUser(client_data * client){
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select * from `users` where fd = %d", client->fd);
	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "delete user failed","after db query(select)");
		return;
	}

	MYSQL_ROW row = NULL;

	if( !mysql_num_rows(result) ) return;
	row = mysql_fetch_row(result);
	
	if( row == NULL ) return;

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
	if( !mysql_num_rows(result) ) return;
	row = mysql_fetch_row(result);
	if( row[0] != NULL ){
		int room_id;
		room_id = atoi(row[1]);

		frame_head sendHead;
		memset(&sendHead, 0 , sizeof(frame_head));
		sendHead.fin = 1;
		sendHead.opcode = 0x1;

		struct packet p;
		memset(&p, 0x00, sizeof(struct packet));
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


	sprintf(queryBuffer, "delete from `users` where id = %d", uid );
	result = NULL;
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


	sprintf(queryBuffer, "delete from `users` where fd = %d", client->fd );
	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "user Add failed","after db query(delete)");
		goto USERADDFAIL;
	}

	sprintf(queryBuffer, "insert into `users` values(0, \'%s\',0,%d)", ((REQUEST_NICKNAME_REGISTER *)(p->ptr))->nickname, client->fd );
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "user Add failed","after db query(insert)");
		goto USERADDFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "select * from `users` where fd = %d", client->fd);
	result = NULL;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "user Add failed","after db query(select)");
		goto USERADDFAIL;
	}

	MYSQL_ROW row;

	memset(&row, 0x00, sizeof(MYSQL_ROW));
	row = mysql_fetch_row( result );

	struct packet sendPacket;
	memset(&sendPacket, 0x00, sizeof(sendPacket));
	sendPacket.major_code = 3;
	sendPacket.minor_code = 0;
	sendPacket.ptr = (RESPONSE_REGISTER*)malloc(sizeof(RESPONSE_REGISTER));

	((RESPONSE_REGISTER*)(sendPacket.ptr))->success = 1;
	((RESPONSE_REGISTER*)(sendPacket.ptr))->user.uid = atoi(row[0]);
	strcpy(((RESPONSE_REGISTER*)(sendPacket.ptr))->user.nickname, row[1]);
	((RESPONSE_REGISTER*)(sendPacket.ptr))->user.score = atoi(row[2]);

	if( result ){
		mysql_free_result(result);	
		result = NULL;
	} 

	const char * contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "user Add failed","packet sending error");
		goto USERADDFAIL;
	}


	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	return 0;

USERADDFAIL:
	memset(&sendPacket, 0x00, sizeof(sendPacket));
	sendPacket.major_code = 3;
	sendPacket.minor_code = 0;
	if( sendPacket.ptr ) {
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	sendPacket.ptr = (RESPONSE_REGISTER*)malloc(sizeof(RESPONSE_REGISTER));

	((RESPONSE_REGISTER*)(sendPacket.ptr))->success = 0;

	contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "user Add failed","packet with failure sending error");
	}
	
	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);	
		result = NULL;
	} 
	return 1;
}


int setScore(client_data * client, frame_head * sendHead, struct packet * p){ 
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "update `users` set `score`= %d where fd = %d", ((REQUEST_SET_SCORE *)(p->ptr))->score, client->fd );
	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "score set failed","after db query(update)");
		goto SETSCOREFAIL;
	}
	if( result ){
		mysql_free_result(result);	
		result = NULL;
	} 

	struct packet sendPacket;
	memset(&sendPacket, 0x00, sizeof(struct packet));
	sendPacket.major_code = 3;
	sendPacket.minor_code = 1;
	sendPacket.ptr = (RESPONSE_SET_SCORE*)malloc(sizeof(RESPONSE_SET_SCORE));
	((RESPONSE_SET_SCORE*)(sendPacket.ptr))->success = 1;

	const char * contents = NULL; 
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "score set failed","packet sending error");
		goto SETSCOREFAIL;
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	return 0;


SETSCOREFAIL:
	memset(&sendPacket, 0x00, sizeof(struct packet));
	sendPacket.major_code = 3;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr ) {
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	sendPacket.ptr = (RESPONSE_SET_SCORE*)malloc(sizeof(RESPONSE_SET_SCORE));
	((RESPONSE_SET_SCORE*)(sendPacket.ptr))->success = 0;

	contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "score set failed","packet with failure sending error");
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	} 
	return 1;
}




// waiting list
int waitingList(client_data * client, frame_head * sendHead, struct packet * p){

	char queryBuffer[QUERY_SIZE] = 	"select gameroom.id, gameroom.status, gameroom.leader_id, count(playerlist.user_id) from gameroom "
									"left join playerlist on playerlist.room_id = gameroom.id "
									"where gameroom.status <> 0 and (gameroom.status/10) < 1 "
									"group by gameroom.id";
	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "get waiting list failed","after db query(select)");
		goto WAITINGLISTFAIL;
	}

	struct packet sendPacket;
	memset(&sendPacket, 0x00, sizeof(struct packet));
	sendPacket.major_code = 2;
	sendPacket.minor_code = 0;
	sendPacket.ptr = (ROOM_LIST_DATA*)malloc(sizeof(ROOM_LIST_DATA));

	MYSQL_ROW row;
	memset(&row, 0x00, sizeof(MYSQL_ROW));
	int idx = 0;
	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) && idx < MAX_ROOM){
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
		result = NULL;
	}

	((ROOM_LIST_DATA*)(sendPacket.ptr))->idx = idx;
	((ROOM_LIST_DATA*)(sendPacket.ptr))->success = 1;


	const char * contents = NULL;
	contents = packet_to_json(sendPacket);	
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "get waiting list failed","packet sending error");
		goto WAITINGLISTFAIL;
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	return 0;



WAITINGLISTFAIL:
	sendPacket.major_code = 2;
	sendPacket.minor_code = 0;
	if( sendPacket.ptr ) {
		free( sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	sendPacket.ptr = (ROOM_LIST_DATA*)malloc(sizeof(ROOM_LIST_DATA));
	((ROOM_LIST_DATA*)(sendPacket.ptr))->success = 0;
	((ROOM_LIST_DATA*)(sendPacket.ptr))->idx = 0;


	contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "get waiting list failed","fail to send error packet");
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	} 
	return 1;
}


int requestEnterRoom(client_data * client, frame_head * sendHead, struct packet * p){
	int room_id = ((REQUEST_ENTER_ROOM *)(p->ptr))->room_id;
	char logbuffer[QUERY_SIZE];
	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select gameroom.id, gameroom.status, gameroom.leader_id, count(playerlist.user_id) from gameroom "
		"left join playerlist on gameroom.id = playerlist.room_id "
		"where gameroom.id = %d && (gameroom.status/10) < 1", room_id
	);

	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","after db query(select)");
		goto ENTERROOMFAIL;
	}


	struct packet sendPacket;
	memset(&sendPacket, 0x00, sizeof(struct packet));
	sendPacket.major_code = 2; 
	sendPacket.minor_code = 1; 
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));


	MYSQL_ROW row; 
	

	if( !mysql_num_rows(result) ) goto ENTERROOMFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ENTERROOMFAIL;

	((ROOM_DATA*)(sendPacket.ptr))->room.id = room_id;
	int status = ((ROOM_DATA*)(sendPacket.ptr))->room.status = atoi(row[1]);
	((ROOM_DATA*)(sendPacket.ptr))->leader_id = atoi(row[2]);
	int room_num = ((ROOM_DATA*)(sendPacket.ptr))->room.num = atoi(row[3]) + 1;
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	if( room_num == MAX_USER ){
		sprintf(queryBuffer, "update `gameroom` set `status`= %d where id = %d ", status + 10, room_id);
		result =  db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "entering waiting room failed","after db query for members(UPDATE)");
			goto ENTERROOMFAIL;
		}
		status = status + 10;
	}
	else if ( room_num > MAX_USER ) goto ENTERROOMFAIL;
	

	sprintf(queryBuffer, "insert into playerlist values(0, %d, %d)", room_id, ((REQUEST_ENTER_ROOM *)(p->ptr))->from.uid);
	result =  db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","after db query for members(insert)");
		goto ENTERROOMFAIL;
	}

	// sprintf(logbuffer, "21.success%d room(#%d, %d, %d)", ((ROOM_DATA*)(sendPacket.ptr))->success, ((ROOM_DATA*)(sendPacket.ptr))->room.id, ((ROOM_DATA*)(sendPacket.ptr))->room.status, ((ROOM_DATA*)(sendPacket.ptr))->room.num );//debug
	// serverLog(WSSERVER, LOG, logbuffer,"");

	sprintf(queryBuffer, "select users.id, users.nickname, users.score, users.fd from users "
			"left join playerlist on playerlist.room_id = %d "
			"where playerlist.user_id = users.id", room_id
	);

	result =  db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","after db query for members(select)");
		goto ENTERROOMFAIL;
	}

	int idx = 0;
	int fdList[MAX_USER+1];
	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) ){
		if( row[0] == NULL ) break;

		((ROOM_DATA*)(sendPacket.ptr))->members[idx].uid = atoi(row[0]);
		strcpy(((ROOM_DATA*)(sendPacket.ptr))->members[idx].nickname, row[1]);
		((ROOM_DATA*)(sendPacket.ptr))->members[idx].score = atoi(row[2]);

		fdList[idx] = atoi(row[3]);

		idx++;
	}

	((ROOM_DATA*)(sendPacket.ptr))->idx = idx;
	((ROOM_DATA*)(sendPacket.ptr))->success = 1; 

	if(result ) {
		mysql_free_result(result);
		result = NULL;
	}

	const char * contents;
	int size;
	int i = 0;
	for( ; i < idx; i++ ){
		int fd = fdList[i];
		struct packet tmp;
		tmp.major_code = 2;
		tmp.minor_code = 2;

		tmp.ptr = (ADDED_MEMBER_DATA*)malloc(sizeof(ADDED_MEMBER_DATA));
		((ADDED_MEMBER_DATA*)(tmp.ptr))->room.id = room_id;
		((ADDED_MEMBER_DATA*)(tmp.ptr))->room.num = room_num;
		((ADDED_MEMBER_DATA*)(tmp.ptr))->user.uid = ((REQUEST_ENTER_ROOM*)(p->ptr))->from.uid;
		strcpy( ((ADDED_MEMBER_DATA*)(tmp.ptr))->user.nickname, ((REQUEST_ENTER_ROOM*)(p->ptr))->from.nickname );
		((ADDED_MEMBER_DATA*)(tmp.ptr))->user.score = ((REQUEST_ENTER_ROOM*)(p->ptr))->from.score;

		contents = packet_to_json(tmp);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(fd, sendHead);
		if( write( fd, contents, size) <= 0 ){
			serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
			goto ENTERROOMFAIL;
		}
		free(tmp.ptr);
		tmp.ptr = NULL;
	}


	contents = packet_to_json(sendPacket);

	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","packet sending error");
		goto ENTERROOMFAIL;
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	if( status % 10 >= 2){
		sprintf(queryBuffer, "select * from painters left join users on users.id = painters.user_id where painters.room_id = %d", room_id);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1){
			serverLog(WSSERVER,ERROR, "enterroom error","after db query(select)");
			goto ENTERROOMFAIL;
		}
		row;
		if( !mysql_num_rows(result) ) goto ENTERROOMFAIL;
		row = mysql_fetch_row(result);
		if( row[0] == NULL ) goto ENTERROOMFAIL; 

		int painter_uid = atoi(row[2]);
		char painter_nickname[NICKNAME_SIZE + 1];
		strcpy( painter_nickname, row[4]);

		if(result){
			mysql_free_result(result);
			result = NULL;
		}


		sprintf(queryBuffer, "select questions.* from questions left join gameroom on gameroom.answer_id = questions.id where gameroom.id = %d", room_id);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1){
			serverLog(WSSERVER,ERROR, "enterroom error","after db query(select)");
			goto ENTERROOMFAIL;
		}
		if( !mysql_num_rows(result) ) goto ENTERROOMFAIL;
		row = mysql_fetch_row(result);
		if( row[0] == NULL ) goto ENTERROOMFAIL; 
		
		int answer_len = strlen(row[1]);
		answer_len = answer_len / 3;// for utf 8 length;

		if(result){
			mysql_free_result(result);
			result = NULL;
		}


		struct packet tmp;
		tmp.major_code = 2;
		tmp.minor_code = 3;
		tmp.ptr = (NEW_ROUND_DATA*)malloc(sizeof(NEW_ROUND_DATA));
		((NEW_ROUND_DATA*)(tmp.ptr))->painter.uid = painter_uid;
		strcpy(((NEW_ROUND_DATA*)(tmp.ptr))->painter.nickname, painter_nickname);
		((NEW_ROUND_DATA*)(tmp.ptr))->answerLen = answer_len;

		contents = packet_to_json(tmp);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(client->fd, sendHead);

		if( write( client->fd, contents, size ) <= 0){
			serverLog(WSSERVER, ERROR, "fail to start game", "packet sending to all ( enterGameRoom )");
			goto ENTERROOMFAIL;
		}
		free(tmp.ptr);
		tmp.ptr = NULL;
	}

	return 0;



ENTERROOMFAIL:
	memset(&sendPacket, 0x00, sizeof(struct packet));
	sendPacket.major_code = 2;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr ) free(sendPacket.ptr);
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));
	((ROOM_DATA*)(sendPacket.ptr))->success = 0;	
	((ROOM_DATA*)(sendPacket.ptr))->idx = 0;


	contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "entering waiting room failed","packet sending error");
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}
	return 1;
}


int makeRoom(client_data * client, frame_head * sendHead, struct packet * p){
	int uid = ((MAKE_ROOM*)(p->ptr))->from.uid;

	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select * from `gameroom` where status = 0");
	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(select)");
		goto MAKEROOMFAIL;
	}
	MYSQL_ROW row;
	if( !mysql_num_rows(result) ) goto MAKEROOMFAIL;
	memset(&row, 0x00, sizeof(MYSQL_ROW));
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto MAKEROOMFAIL; // all rooms are occupied;

	struct packet sendPacket;
	memset(&sendPacket , 0x0, sizeof(struct packet));
	sendPacket.major_code = 2;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr) {
		free( sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));

	int room_id = ((ROOM_DATA*)(sendPacket.ptr))->room.id = atoi(row[0]);
	((ROOM_DATA*)(sendPacket.ptr))->room.status = 1;
	if( result ){
		mysql_free_result(result);	
		result = NULL;
	}



	sprintf(queryBuffer, "update `gameroom` set status = 1, leader_id = %d where id = %d ", uid, room_id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(update)");
		goto MAKEROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "insert into playerlist values(0,%d,%d)",room_id ,((MAKE_ROOM*)(p->ptr))->from.uid);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(insert)");
		goto MAKEROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where room_id = %d", room_id );
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "makeRoom error","after db query(select)");
		goto MAKEROOMFAIL;
	}

	int idx = 0;
	memset(&row, 0x00, sizeof(MYSQL_ROW));
	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) && idx < MAX_USER){
		if( row[0] == NULL ) break;
		((ROOM_DATA*)(sendPacket.ptr))->members[idx].uid = atoi(row[3]);
		strcpy(((ROOM_DATA*)(sendPacket.ptr))->members[idx].nickname, row[4]);
		((ROOM_DATA*)(sendPacket.ptr))->members[idx].score, atoi(row[5]);
		idx ++;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}
	((ROOM_DATA*)(sendPacket.ptr))->idx = idx;
	((ROOM_DATA*)(sendPacket.ptr))->room.num = idx;
	((ROOM_DATA*)(sendPacket.ptr))->success = 1;
	((ROOM_DATA*)(sendPacket.ptr))->leader_id = uid;

	const char * contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		goto MAKEROOMFAIL;
	}
	
	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	return 0;



MAKEROOMFAIL:
	memset(&sendPacket, 0x00, sizeof(struct packet));
	sendPacket.major_code = 2;
	sendPacket.minor_code = 1;
	if( sendPacket.ptr) {
		free( sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	sendPacket.ptr = (ROOM_DATA*)malloc(sizeof(ROOM_DATA));
	((ROOM_DATA*)(sendPacket.ptr))->idx = 0;
	((ROOM_DATA*)(sendPacket.ptr))->success = 0;


	contents = NULL;
	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","fail to send error packet");
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	} 
	return 1;
}



// waiting room

int exitRoom(client_data * client, frame_head * sendHead, struct packet * p){
	
	char queryBuffer[QUERY_SIZE];
	const char * contents = NULL;

	int fromUID = ((REQUEST_EXIT_ROOM*)(p->ptr))->from.uid;
	int room_id = ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id;
	sprintf(queryBuffer, "delete from playerlist where room_id = %d and user_id = %d", room_id, fromUID);
	MYSQL_RES * result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "exit room fail", "after db query(delete)");
		goto EXITROOMFAIL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	MYSQL_ROW row;
	int room_num = 0;

	sprintf(queryBuffer, "select gameroom.status, count(playerlist.user_id) from gameroom left join playerlist on playerlist.room_id = gameroom.id where gameroom.id = %d", room_id);
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
		goto EXITROOMFAIL;
	}

	if( !mysql_num_rows(result) ) goto EXITROOMFAIL;

	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto EXITROOMFAIL;
	int status = atoi(row[0]);
	room_num = atoi(row[1]);

	if( room_num < MAX_USER ){
		sprintf(queryBuffer, "update `gameroom` set `status` =  %d where id = %d", status % 10, room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update");
			goto EXITROOMFAIL;
		}
		status = status % 10;
	}


	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where playerlist.room_id = %d ", room_id);
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
		goto EXITROOMFAIL;
	}


	int size;
	int fd;

	if( !mysql_num_rows(result) )  result = NULL;

	while( result && ( row = mysql_fetch_row(result) ) ){
		if( row[0] == NULL ) break;
		fd = atoi(row[6]);
		struct packet tmp;
		memset(&tmp, 0x00, sizeof(struct packet));
		tmp.major_code = 3;
		tmp.minor_code = 2;
		tmp.ptr = (POP_MEMBER_DATA*)malloc(sizeof(POP_MEMBER_DATA));
		((POP_MEMBER_DATA*)(tmp.ptr))->room.id = room_id;
		((POP_MEMBER_DATA*)(tmp.ptr))->room.num = room_num;
		((POP_MEMBER_DATA*)(tmp.ptr))->room.status = status;

		((POP_MEMBER_DATA*)(tmp.ptr))->user.uid = fromUID;
		strcpy( ((POP_MEMBER_DATA*)(tmp.ptr))->user.nickname, ((REQUEST_EXIT_ROOM*)(p->ptr))->from.nickname );

		contents = NULL;
		contents = packet_to_json(tmp);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(fd, sendHead);
		if( write( fd, contents, size) <= 0 ){
			serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
			goto EXITROOMFAIL;
		}

		if(tmp.ptr){
			free(tmp.ptr);
			tmp.ptr = NULL;
		}
		if(contents){
			free(contents);
			contents = NULL;
		}
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}




	struct packet sendPacket;
	sendPacket.ptr = NULL;
	if( client->fd != -1){
		memset(&sendPacket, 0x00, sizeof(struct packet));
		sendPacket.major_code = 2;
		sendPacket.minor_code = 8;
		sendPacket.ptr = (RESPONSE_EXIT*)malloc(sizeof(RESPONSE_EXIT));
		((RESPONSE_EXIT*)(sendPacket.ptr))->success = 1;


		contents = NULL;
		contents = packet_to_json(sendPacket);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(client->fd, sendHead);
		if( write( client->fd, contents, size) <= 0 ){
			serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
			goto EXITROOMFAIL;
		}
		if(sendPacket.ptr){
			free(sendPacket.ptr);
			sendPacket.ptr = NULL;
		}
		if(contents){
			free((char*)contents);	
			contents = NULL;
		}
	}
	


	int fd_table[MAX_USER + 1];

	int isPainter = 0;
	sprintf(queryBuffer, "select * from `painters` where user_id = %d", fromUID);
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
		goto EXITROOMFAIL;
	}	
	if( mysql_num_rows(result) != 0 ){
		row = mysql_fetch_row(result);
		if( row[0] != NULL ) isPainter = 1;
		else result = NULL;
	}
	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}




	if( room_num < 1 ){
		sprintf(queryBuffer, "delete from `playerlist` where room_id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(delete)");
			goto EXITROOMFAIL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}

		sprintf(queryBuffer, "update `gameroom` set status = 0, leader_id = NULL, answer_id = NULL where id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
			goto EXITROOMFAIL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}
	}
	else{
		int room_id = ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id;
		sprintf(queryBuffer, "select user_id from `playerlist` where room_id = %d", room_id);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
			goto EXITROOMFAIL;
		}
		if( result == 0 ) goto EXITROOMFAIL;
		if( !mysql_num_rows(result) ) goto EXITROOMFAIL;

		memset(&row, 0x00, sizeof(MYSQL_ROW));
		row = mysql_fetch_row(result);
		if(row[0] == NULL ) goto EXITROOMFAIL;

		int uid = atoi(row[0]);
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}

		sprintf(queryBuffer, "update `gameroom` set leader_id = %d where id = %d", uid, room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
			goto EXITROOMFAIL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}



		sprintf(queryBuffer, "select * from `users` where id = %d", uid);
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
			goto EXITROOMFAIL;
		}
		memset(&row, 0x00, sizeof(MYSQL_ROW));

		if( !mysql_num_rows(result) ) goto EXITROOMFAIL;
		row = mysql_fetch_row(result);
		if( row[0] == NULL ) goto EXITROOMFAIL;

		int leader_uid = atoi(row[0]);
		char leader_nickname[NICKNAME_SIZE + 1];
		strcpy( leader_nickname, row[1]);
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}

		if( room_num == 1 ){
			sprintf(queryBuffer, "update `gameroom` set status = 1 where id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
			result = db_query(queryBuffer, client, NONSELECT);
			if( result == -1 ){
				serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
				goto EXITROOMFAIL;
			}

			sprintf(queryBuffer, "update `painters` set user_id = NULL where room_id = %d", ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id);
			result = db_query(queryBuffer, client, NONSELECT);
			if( result == -1 ){
				serverLog(WSSERVER, ERROR, "exit room fail", "after db query(update)");
				goto EXITROOMFAIL;
			}


			struct packet tmp;
			memset(&tmp, 0x00, sizeof(struct packet));
			tmp.major_code = 3;
			tmp.minor_code = 3;
			tmp.ptr = (STATUS_CHANGED*)malloc(sizeof(STATUS_CHANGED));
			((STATUS_CHANGED*)(tmp.ptr))->room.id = ((REQUEST_EXIT_ROOM*)(p->ptr))->room_id;
			room_num = ((STATUS_CHANGED*)(tmp.ptr))->room.num = 1;
			((STATUS_CHANGED*)(tmp.ptr))->leader.uid = leader_uid;
			strcpy( ((STATUS_CHANGED*)(tmp.ptr))->leader.nickname, leader_nickname );

			contents = NULL;
			contents = packet_to_json(tmp);
			iso8859_1_to_utf8(contents, strlen(contents));
			size = sendHead->payload_length = strlen(contents);
			send_frame_head(fd, sendHead);
			if( write( fd, contents, size) <= 0 ){
				serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
				goto EXITROOMFAIL;
			}

			if(tmp.ptr){
				free(tmp.ptr);
				tmp.ptr = NULL;
			}
		}
		else{
			sprintf(queryBuffer, "select * from `gameroom` where id = %d", room_id);
			result = db_query(queryBuffer, client, SELECT);
			if( result == -1 ){
				serverLog(WSSERVER, ERROR, "exit room fail", "after db query(select)");
				goto EXITROOMFAIL;
			}
			
			if( !mysql_num_rows(result) ) goto EXITROOMFAIL;
			memset(&row, 0x00, sizeof(MYSQL_ROW));
			row = mysql_fetch_row(result);
			if( row[0] == NULL ) goto EXITROOMFAIL;

			int status = atoi(row[1]);


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

			int idx = 0;
			while( ( row = mysql_fetch_row(result)) ){
				if( row[0] == NULL ) break;
				int fd = atoi(row[6]);
				fd_table[idx] = fd;
				struct packet tmp;
				memset(&tmp, 0x00, sizeof(struct packet));
				tmp.major_code = 3;
				tmp.minor_code = 3;
				tmp.ptr = (STATUS_CHANGED*)malloc(sizeof(STATUS_CHANGED));
				((STATUS_CHANGED*)(tmp.ptr))->room.id = room_id;
				((STATUS_CHANGED*)(tmp.ptr))->room.num = room_num;
				((STATUS_CHANGED*)(tmp.ptr))->room.status = status;
				((STATUS_CHANGED*)(tmp.ptr))->leader.uid = leader_uid;
				strcpy(((STATUS_CHANGED*)(tmp.ptr))->leader.nickname, leader_nickname);


				contents = NULL;
				contents = packet_to_json(tmp);
				iso8859_1_to_utf8(contents, strlen(contents));
				size = sendHead->payload_length = strlen(contents);
				send_frame_head(fd, sendHead);
				if( write( fd, contents, size) <= 0 ){
					serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
					goto EXITROOMFAIL;
				}
				idx++;
				if(tmp.ptr){
					free(tmp.ptr);
					tmp.ptr = NULL;
				}
				if(contents){
					free(contents);
					contents = NULL;
				}
			}

			if( result ){
				mysql_free_result(result);
				result = NULL;
			}


		}
	}

	if( result ){
		mysql_free_result(result);
		result = NULL;
	} 
	
	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}




	if(isPainter && room_num >= 2){

		char painter_nickname[NICKNAME_SIZE + 1];

		struct packet sendPacket;
		sendPacket.major_code = 2;
		sendPacket.minor_code = 4;
		sendPacket.ptr = (ANSWER_DATA*)malloc(sizeof(ANSWER_DATA));
		
		sprintf(queryBuffer, "SELECT * FROM `questions` ORDER BY RAND() LIMIT 1");
		result = db_query(queryBuffer, client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "fail to start game in exitroom","after db_query(select)");
			goto EXITROOMFAIL;
		}
		if( !mysql_num_rows(result) ) goto EXITROOMFAIL;
		row = mysql_fetch_row(result);
		if( row[0] == NULL ) goto EXITROOMFAIL;
		strcpy( ((ANSWER_DATA*)(sendPacket.ptr))->answer, row[1]);
		int answer_id = atoi(row[0]);
		int answer_len  = strlen(((ANSWER_DATA*)(sendPacket.ptr))->answer);

		answer_len = answer_len / 3; // for utf8 length;

		if( result ) {
			mysql_free_result(result);
			result = NULL;
		}



		sprintf(queryBuffer, "update `gameroom` set `answer_id` = %d where id = %d", answer_id, room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "fail to start game in exitroom","after db_query(update)");
			goto EXITROOMFAIL;
		}



		sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where room_id = %d order by rand() limit 1;", room_id);
		result = db_query(queryBuffer,client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "fail to start game in exitroom", "after db_query(select)");
			goto EXITROOMFAIL;
		}
		if( !mysql_num_rows(result) ) goto EXITROOMFAIL;
		row = mysql_fetch_row(result);
		if( row[0] == NULL ) goto EXITROOMFAIL;
		strcpy(painter_nickname, row[4]);
		int painter_uid = atoi(row[2]);

		if( result ){
			mysql_free_result(result);
			result = NULL;
		}



		sprintf(queryBuffer, "update `painters` set `user_id` = %d where room_id = %d", painter_uid, room_id);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "fail to start game in exitroom","after db_query(update)");
			goto EXITROOMFAIL;
		}




		sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where playerlist.room_id = %d", room_id);
		result = db_query(queryBuffer,client, SELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "fail to start game in exitroom", "after db_query(select)");
			goto EXITROOMFAIL;
		}

		const char * contents;
		int size;
		int painter_fd = 0;

		if( !mysql_num_rows(result) ) result = NULL;
		while( result && (row = mysql_fetch_row(result)) ){
			if( row[0] == NULL ) break;
			int fd = atoi(row[6]);
			int uid = atoi(row[2]);
			
			if( uid == painter_uid ) painter_fd = fd;

			struct packet tmp;
			tmp.major_code = 2;
			tmp.minor_code = 3;
			tmp.ptr = (NEW_ROUND_DATA*)malloc(sizeof(NEW_ROUND_DATA));
			((NEW_ROUND_DATA*)(tmp.ptr))->painter.uid = painter_uid;
			strcpy(((NEW_ROUND_DATA*)(tmp.ptr))->painter.nickname, painter_nickname);
			((NEW_ROUND_DATA*)(tmp.ptr))->answerLen = answer_len;

			contents = packet_to_json(tmp);
			iso8859_1_to_utf8(contents, strlen(contents));
			size = sendHead->payload_length = strlen(contents);
			send_frame_head(fd, sendHead);

			if( write( fd, contents, size ) <= 0){
				serverLog(WSSERVER, ERROR, "fail to start game in exitroom", "packet sending to all ( exitroom )");
				goto EXITROOMFAIL;
			}
			free(tmp.ptr);
			tmp.ptr = NULL;
		}
		if( result ){
			mysql_free_result(result);
			result = NULL;
		}



		((ANSWER_DATA*)(sendPacket.ptr))->success = 1;

		contents = packet_to_json(sendPacket);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(painter_fd, sendHead);

		if( write( painter_fd, contents, size) <= 0 ){
			serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
			goto EXITROOMFAIL;
		}

		if( sendPacket.ptr) free(sendPacket.ptr);
		if( contents ) free((char*)contents);
		sendPacket.ptr = NULL;
		contents = NULL;

		if( result ){
			mysql_free_result(result);	
			result =NULL;
		} 
	}



	return 0;



EXITROOMFAIL:
	if( client->fd != -1 ){
		sendPacket.major_code = 2;
		sendPacket.minor_code = 8;
		if( sendPacket.ptr ) {
			free(sendPacket.ptr);
			sendPacket.ptr = NULL;
		}
		sendPacket.ptr = (RESPONSE_EXIT*)malloc(sizeof(RESPONSE_EXIT));
		((RESPONSE_EXIT*)(sendPacket.ptr))->success = 0;


		contents = NULL;
		contents = packet_to_json(sendPacket);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(client->fd, sendHead);
		if( write( client->fd, contents, size) <= 0 ){
			serverLog(WSSERVER, ERROR, "failed to make room in exit room","packet sending error");
		}
	}

	if(sendPacket.ptr){
		free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	} 
	return 1;
}



int enterGameRoom(client_data * client, frame_head * sendHead, struct packet * p){
	int room_id = ((REQUEST_START*)(p->ptr))->room_id;
	int uid = ((REQUEST_START*)(p->ptr))->from.uid;
	char painter_nickname[NICKNAME_SIZE + 1];

	MYSQL_RES * result;
	MYSQL_ROW row;

	char queryBuffer[QUERY_SIZE];
	sprintf(queryBuffer, "select * from gameroom where id = %d and (status%10)=1", room_id);
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game","after db_query(select)");
		goto ENTERGAMEROOMFAIL;
	}
	if( !mysql_num_rows(result) ) goto ENTERGAMEROOMFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ENTERGAMEROOMFAIL;
	int status = atoi(row[1]);


	status = status - (status % 10);
	status = status + 2;
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	sprintf(queryBuffer, "update gameroom set status = %d where id = %d", status ,room_id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game","after db_query(update)");
		goto ENTERGAMEROOMFAIL;
	}



	struct packet sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 4;
	sendPacket.ptr = (ANSWER_DATA*)malloc(sizeof(ANSWER_DATA));
	
	sprintf(queryBuffer, "SELECT * FROM `questions` ORDER BY RAND() LIMIT 1");
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game","after db_query(select)");
		goto ENTERGAMEROOMFAIL;
	}
	if( !mysql_num_rows(result) ) goto ENTERGAMEROOMFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ENTERGAMEROOMFAIL;
	strcpy( ((ANSWER_DATA*)(sendPacket.ptr))->answer, row[1]);
	int answer_id = atoi(row[0]);
	int answer_len  = strlen(((ANSWER_DATA*)(sendPacket.ptr))->answer);

	answer_len = answer_len / 3; // for utf8 length;

	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}



	sprintf(queryBuffer, "update `gameroom` set `answer_id` = %d where id = %d", answer_id, room_id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game","after db_query(select)");
		goto ENTERGAMEROOMFAIL;
	}



	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where room_id = %d order by rand() limit 1;", room_id);
	result = db_query(queryBuffer,client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game", "after db_query(select)");
		goto ENTERGAMEROOMFAIL;
	}
	if( !mysql_num_rows(result) ) goto ENTERGAMEROOMFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ENTERGAMEROOMFAIL;
	strcpy(painter_nickname, row[4]);
	int painter_uid = atoi(row[2]);

	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	sprintf(queryBuffer, "update `painters` set `user_id` = %d where room_id = %d", painter_uid, room_id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game","after db_query(select)");
		goto ENTERGAMEROOMFAIL;
	}




	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where playerlist.room_id = %d", room_id);
	result = db_query(queryBuffer,client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game", "after db_query(select)");
		goto ENTERGAMEROOMFAIL;
	}

	const char * contents;
	int size;
	int painter_fd = 0;

	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) ){
		if( row[0] == NULL ) break;
		int fd = atoi(row[6]);
		int uid = atoi(row[2]);
		
		if( uid == painter_uid ) painter_fd = fd;

		struct packet tmp;
		tmp.major_code = 2;
		tmp.minor_code = 3;
		tmp.ptr = (NEW_ROUND_DATA*)malloc(sizeof(NEW_ROUND_DATA));
		((NEW_ROUND_DATA*)(tmp.ptr))->painter.uid = painter_uid;
		strcpy(((NEW_ROUND_DATA*)(tmp.ptr))->painter.nickname, painter_nickname);
		((NEW_ROUND_DATA*)(tmp.ptr))->answerLen = answer_len;

		contents = packet_to_json(tmp);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(fd, sendHead);

		if( write( fd, contents, size ) <= 0){
			serverLog(WSSERVER, ERROR, "fail to start game", "packet sending to all ( enterGameRoom )");
			goto ENTERGAMEROOMFAIL;
		}
		free(tmp.ptr);
		tmp.ptr = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	((ANSWER_DATA*)(sendPacket.ptr))->success = 1;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(painter_fd, sendHead);

	if( write( painter_fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		goto ENTERGAMEROOMFAIL;
	}

	if( sendPacket.ptr) free(sendPacket.ptr);
	if( contents ) free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;

	if( result ){
		mysql_free_result(result);	
		result =NULL;
	} 

	return 0;

ENTERGAMEROOMFAIL:
	sendPacket.major_code = 2;
	sendPacket.minor_code = 4;
	if( sendPacket.ptr ) free(sendPacket.ptr);
	sendPacket.ptr = (ANSWER_DATA*)malloc(sizeof(ANSWER_DATA));
	((ANSWER_DATA*)(sendPacket.ptr))->success = 0;

	if( contents) free((char*)contents);

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");		
	}

	if( sendPacket.ptr ) free(sendPacket.ptr);
	if( contents) free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;

	if( result ){
		mysql_free_result(result);	
		result =NULL;
	} 
	return 1;
}


// gameroom
int drawingPoint(client_data * client, frame_head * sendHead, struct packet * p){
	// struct packet sendPacket = *p;
	/*todo
	  클라이언트에게 그림데이터 재전송까지 함
	 */
	
	char logbuf[QUERY_SIZE]; //debug
	char queryBuffer[QUERY_SIZE];
	int room_id = ((DRAW_DATA*)(p->ptr))->room_id;
	sprintf(queryBuffer, "select users.fd "
			     "from users "
			     "left join playerlist "
			     "on playerlist.room_id = %d "
			     "where playerlist.user_id = users.id", room_id);
	MYSQL_RES * result = NULL;
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "sending drawing data fail", "after db query(draw)");
		goto DRAWDATAFAIL;
	}
	
	MYSQL_ROW row;
	memset(&row, 0x00, sizeof(MYSQL_ROW));

	int idx = 0;
	int fdList[MAX_USER+1];
	

	if( !mysql_num_rows(result) ) result = NULL;
//	serverLog(WSSERVER, LOG, "fetch error?", "");//debug
	while( result && (row = mysql_fetch_row(result)) && (idx < MAX_USER)){
		if( row[0] == NULL ) break;
		fdList[idx] = atoi(row[0]);
		idx++;
	}
	//serverLog(WSSERVER, LOG, "=> NO", "");//debug

	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}


	
	((DRAW_DATA*)(p->ptr))->success = 1;
	const char * contents = NULL;
	//serverLog(WSSERVER, LOG, "packetjson trolling?", "");//debug
	contents = packet_to_json(*p);
	//serverLog(WSSERVER, LOG, "=>NO", "");//debug

	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);

	int i = 0;
	for( ; i < idx; i++ ){
		int fd = fdList[i];

		send_frame_head(fd, sendHead);
		if( write( fd, contents, size) <= 0 ){
			serverLog(WSSERVER, ERROR, "sending drawing data fail", "packet sending error");
			if(contents){
				free((char *)contents);
				contents = NULL;
			}
			goto DRAWDATAFAIL;
		}
		sprintf(logbuf, "send drawingdata to fd:%d",fd);//debug
		serverLog(WSSERVER, LOG, logbuf , "");//debug
	}
	if(contents){
		free((char*)contents);
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}
	return 0;



DRAWDATAFAIL:
	((DRAW_DATA*)(p->ptr))->success = 0;

	contents = NULL;
	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "sending drawing data fail", "packet sending error");
	}
	if(contents){
		free((char*)contents);	
		contents = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}
	return 1;
}



int echoChatData(client_data *client, frame_head *sendHead, struct packet *p) {
	char queryBuffer[QUERY_SIZE];
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;

	sprintf(queryBuffer,
			"select * from playerlist left join users on playerlist.user_id  = users.id where room_id = %d",
			((CHAT_DATA *) (p->ptr))->room_id
	);
	result = db_query(queryBuffer, client, SELECT);
	if (result == -1) {
		serverLog(WSSERVER, ERROR, "echoChatData error", "after db query(select)");
		goto ECHOCHATDATAFAIL;
	}

	int idx = 0;
	memset(&row, 0x00, sizeof(MYSQL_ROW));


	int fd_table[MAX_USER+1];
	if( !mysql_num_rows(result) ) result = NULL; 
	while ( result && (row = mysql_fetch_row(result)) && idx < MAX_USER) {
		if (row[0] == NULL) break;
		fd_table[idx] = atoi(row[6]);
		idx++;
	}
	if (result){
		mysql_free_result(result);
		result = NULL;
	}




	sprintf(queryBuffer,"select * from painters where room_id = %d",((CHAT_DATA *) (p->ptr))->room_id);
	result = db_query(queryBuffer, client, SELECT);
	if (result == -1) {
		serverLog(WSSERVER, ERROR, "echoChatData error", "after db query(select)");
		goto ECHOCHATDATAFAIL;
	}
	if( !mysql_num_rows(result) ) goto ECHOCHATDATAFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ECHOCHATDATAFAIL;

	int painter_uid = atoi(row[2]);

	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}







	sprintf(queryBuffer,
			"select questions.* from questions left join gameroom on gameroom.answer_id = questions.id where gameroom.id = %d;",
			((CHAT_DATA *) (p->ptr))->room_id
	);
	result = db_query(queryBuffer, client, SELECT);
	if (result == -1) {
		serverLog(WSSERVER, ERROR, "echoChatData error", "after db query(select)");
		goto ECHOCHATDATAFAIL;
	}
	if( !mysql_num_rows(result) ) goto ECHOCHATDATAFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ECHOCHATDATAFAIL;


	int correct = 0;
	if( ((CHAT_DATA *)(p->ptr))->from.uid != painter_uid && strcmp(row[1], ((CHAT_DATA *)(p->ptr))->msg) == 0 ) correct = 1;

	((CHAT_DATA *)(p->ptr))->success = 1;

	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}

	const char *contents = NULL;
	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	
	for (int i = 0; i < idx; i++) {
		send_frame_head(fd_table[i], sendHead);
		if (write(fd_table[i], contents, size) <= 0) {
			serverLog(WSSERVER, ERROR, "ECHOcHATdATA ERROR", "packet sending error");
			goto ECHOCHATDATAFAIL;
		}
	}
	if (contents) {
		free((char *) contents);
		contents = NULL;
	}



	if( correct ){
		sprintf(queryBuffer, "update `users` set score = %d where id = %d", ((CHAT_DATA *) (p->ptr))->from.score + 10, ((CHAT_DATA *) (p->ptr))->from.uid);
		result = db_query(queryBuffer, client, NONSELECT);
		if( result == -1 ){
			serverLog(WSSERVER, ERROR, "failed to start drawing", "after db_query(update)");
		}


		struct packet sendPacket;
		sendPacket.major_code = 0;
		sendPacket.minor_code = 5;
		sendPacket.ptr = (WINNER_DATA*)malloc(sizeof(WINNER_DATA));

		((WINNER_DATA*)(sendPacket.ptr))->room_id = ((CHAT_DATA *) (p->ptr))->room_id;
		((WINNER_DATA*)(sendPacket.ptr))->winner.uid = ((CHAT_DATA *) (p->ptr))->from.uid;
		strcpy( ((WINNER_DATA*)(sendPacket.ptr))->winner.nickname, ((CHAT_DATA *) (p->ptr))->from.nickname);
		((WINNER_DATA*)(sendPacket.ptr))->winner.score =  ((CHAT_DATA *) (p->ptr))->from.score + 10;
		strcpy( ((WINNER_DATA*)(sendPacket.ptr))->answer, ((CHAT_DATA *) (p->ptr))->msg);

		if( contents ) free((char*)contents);
		contents = NULL;
		contents = packet_to_json(sendPacket);
		iso8859_1_to_utf8(contents, strlen(contents));
		int size = sendHead->payload_length = strlen(contents);
		
		for (int i = 0; i < idx; i++) {
			send_frame_head(fd_table[i], sendHead);
			if (write(fd_table[i], contents, size) <= 0) {
				serverLog(WSSERVER, ERROR, "failed to start drawing", "packet sending error");
				goto ECHOCHATDATAFAIL;
			}
		}

		if( sendPacket.ptr ) free(sendPacket.ptr);
		sendPacket.ptr = NULL;
	}

	return 0;


ECHOCHATDATAFAIL:
	((CHAT_DATA *) (p->ptr))->success = 0;

	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if (write(client->fd, contents, size) <= 0) {
		serverLog(WSSERVER, ERROR, "failed to start drawing", "packet sending error");
	}

	free((char *) contents);
	contents = NULL;

	if (result) {
		mysql_free_result(result);
		result = NULL;
	}
	return 1;
}
int startDrawing(client_data *client, frame_head * sendHead, struct packet * p){
	char queryBuffer[QUERY_SIZE];
	MYSQL_RES * result = NULL;

	int room_id = ((REQUEST_DRAWING_START*)(p->ptr))->room_id;
	int uid = ((REQUEST_DRAWING_START*)(p->ptr))->from.uid;

	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id  = users.id where room_id = %d", ((REQUEST_DRAWING_START*)(p->ptr))->room_id);

	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "startDrawing error","after db query(select)");
		goto STARTDRAWINGFAIL;
	}

	int idx =0;
	MYSQL_ROW row;
	memset(&row, 0x00, sizeof(MYSQL_ROW));


	int fd_table[MAX_USER+1];
	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) && idx < MAX_USER){
		if( row[0] == NULL ) break;
		fd_table[idx] = atoi(row[6]);
		idx ++;
	}
	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}

	((REQUEST_DRAWING_START*)(p->ptr))->success = 1;



	const char *contents = NULL;
	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	

	for(int i=0; i<idx; i++){
		send_frame_head(fd_table[i], sendHead);
		if( write (fd_table[i], contents, size)  <= 0){
			serverLog(WSSERVER, ERROR, "failed to start drawing","packet sending error");
			goto STARTDRAWINGFAIL;

		}
	}

	if(contents){
		free((char*)contents);
		contents = NULL;
	}
	return 0;


STARTDRAWINGFAIL:
	((REQUEST_DRAWING_START*)(p->ptr))->success = 0;

	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to start drawing","packet sending error");
	}

	free((char*)contents);
	contents = NULL;

	if( result ){
		mysql_free_result(result);
		result = NULL;
	}
	return 1;
}

int endDrawing(client_data *client, frame_head * sendHead, struct packet * p){
	char queryBuffer[QUERY_SIZE];
	MYSQL_RES * result = NULL;

	int room_id = ((REQUEST_DRAWING_END*)(p->ptr))->room_id;
	int uid = ((REQUEST_DRAWING_END*)(p->ptr))->from.uid;

	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id  = users.id where room_id = %d", ((REQUEST_DRAWING_END*)(p->ptr))->room_id);

	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "endDrawing error","after db query(select)");
		goto ENDDRAWINGFAIL;
	}

	int idx =0;
	MYSQL_ROW row;
	memset(&row, 0x00, sizeof(MYSQL_ROW));


	int fd_table[MAX_USER+1];
	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) && idx < MAX_USER){
		if( row[0] == NULL ) break;
		fd_table[idx] = atoi(row[6]);
		idx ++;
	}
	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}

	((REQUEST_DRAWING_END*)(p->ptr))->success = 1;


	const char *contents = NULL;
	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);	

	for(int i=0; i<idx; i++){
		send_frame_head(fd_table[i], sendHead);
		if( write (fd_table[i], contents, size)  <= 0){
			serverLog(WSSERVER, ERROR, "failed to end drawing","packet sending error");
			goto ENDDRAWINGFAIL;
		}
	}
	if(contents){
		free((char*)contents);
		contents = NULL;
	}


/// restart game

	char painter_nickname[NICKNAME_SIZE + 1];

	struct packet sendPacket;
	sendPacket.major_code = 2;
	sendPacket.minor_code = 4;
	sendPacket.ptr = (ANSWER_DATA*)malloc(sizeof(ANSWER_DATA));
	
	sprintf(queryBuffer, "SELECT * FROM `questions` ORDER BY RAND() LIMIT 1");
	result = db_query(queryBuffer, client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game in enddrawing","after db_query(select)");
		goto ENDDRAWINGFAIL;
	}
	if( !mysql_num_rows(result) ) goto ENDDRAWINGFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ENDDRAWINGFAIL;
	strcpy( ((ANSWER_DATA*)(sendPacket.ptr))->answer, row[1]);
	int answer_id = atoi(row[0]);
	int answer_len  = strlen(((ANSWER_DATA*)(sendPacket.ptr))->answer);

	answer_len = answer_len / 3; // for utf8 length;

	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}



	sprintf(queryBuffer, "update `gameroom` set `answer_id` = %d where id = %d", answer_id, room_id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game in enddrawing","after db_query(update)");
		goto ENDDRAWINGFAIL;
	}



	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where room_id = %d order by rand() limit 1;", room_id);
	result = db_query(queryBuffer,client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game in enddrawing", "after db_query(select)");
		goto ENDDRAWINGFAIL;
	}
	if( !mysql_num_rows(result) ) goto ENDDRAWINGFAIL;
	row = mysql_fetch_row(result);
	if( row[0] == NULL ) goto ENDDRAWINGFAIL;
	strcpy(painter_nickname, row[4]);
	int painter_uid = atoi(row[2]);

	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	sprintf(queryBuffer, "update `painters` set `user_id` = %d where room_id = %d", painter_uid, room_id);
	result = db_query(queryBuffer, client, NONSELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game in enddrawing","after db_query(update)");
		goto ENDDRAWINGFAIL;
	}


	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id = users.id where playerlist.room_id = %d", room_id);
	result = db_query(queryBuffer,client, SELECT);
	if( result == -1 ){
		serverLog(WSSERVER, ERROR, "fail to start game in enddrawing", "after db_query(select)");
		goto ENDDRAWINGFAIL;
	}

	contents = NULL;
	size;
	int painter_fd = 0;

	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) ){
		if( row[0] == NULL ) break;
		int fd = atoi(row[6]);
		int uid = atoi(row[2]);
		
		if( uid == painter_uid ) painter_fd = fd;

		struct packet tmp;
		tmp.major_code = 2;
		tmp.minor_code = 3;
		tmp.ptr = (NEW_ROUND_DATA*)malloc(sizeof(NEW_ROUND_DATA));
		((NEW_ROUND_DATA*)(tmp.ptr))->painter.uid = painter_uid;
		strcpy(((NEW_ROUND_DATA*)(tmp.ptr))->painter.nickname, painter_nickname);
		((NEW_ROUND_DATA*)(tmp.ptr))->answerLen = answer_len;

		contents = packet_to_json(tmp);
		iso8859_1_to_utf8(contents, strlen(contents));
		size = sendHead->payload_length = strlen(contents);
		send_frame_head(fd, sendHead);

		if( write( fd, contents, size ) <= 0){
			serverLog(WSSERVER, ERROR, "fail to start game in enddrawing", "packet sending to all ( enddrawing )");
			goto ENDDRAWINGFAIL;
		}
		free(tmp.ptr);
		tmp.ptr = NULL;
	}
	if( result ){
		mysql_free_result(result);
		result = NULL;
	}



	((ANSWER_DATA*)(sendPacket.ptr))->success = 1;

	contents = packet_to_json(sendPacket);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(painter_fd, sendHead);

	if( write( painter_fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to make room","packet sending error");
		goto ENDDRAWINGFAIL;
	}

	if( sendPacket.ptr) free(sendPacket.ptr);
	if( contents ) free((char*)contents);
	sendPacket.ptr = NULL;
	contents = NULL;

	if( result ){
		mysql_free_result(result);	
		result =NULL;
	} 

	return 0;

ENDDRAWINGFAIL:
	((REQUEST_DRAWING_END*)(p->ptr))->success = 0;

	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to end drawing","packet sending error");
	}

	free((char*)contents);
	contents = NULL;

	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	return 1;
}

int shareTime(client_data *client, frame_head * sendHead, struct packet * p){
	char queryBuffer[QUERY_SIZE];
	MYSQL_RES * result = NULL;

	int room_id = ((REQUEST_TIME_SHARE*)(p->ptr))->room_id;
	int uid = ((REQUEST_TIME_SHARE*)(p->ptr))->from.uid;

	sprintf(queryBuffer, "select * from playerlist left join users on playerlist.user_id  = users.id where room_id = %d", ((REQUEST_DRAWING_END*)(p->ptr))->room_id);

	result = db_query(queryBuffer, client, SELECT);
	if( result == -1){
		serverLog(WSSERVER,ERROR, "shareTime error","after db query(select)");
		goto TIMESHAREFAIL;
	}

	int idx =0;
	MYSQL_ROW row;
	memset(&row, 0x00, sizeof(MYSQL_ROW));


	int fd_table[MAX_USER+1];
	if( !mysql_num_rows(result) ) result = NULL;
	while( result && (row = mysql_fetch_row(result)) && idx < MAX_USER){
		if( row[0] == NULL ) break;
		fd_table[idx] = atoi(row[6]);
		idx ++;
	}
	if( result ) {
		mysql_free_result(result);
		result = NULL;
	}

	((REQUEST_TIME_SHARE*)(p->ptr))->success = 1;
	
	const char *contents = NULL;
	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	int size = sendHead->payload_length = strlen(contents);
	

	for(int i=0; i<idx; i++){
		send_frame_head(fd_table[i], sendHead);
		if( write (fd_table[i], contents, size)  <= 0){
			serverLog(WSSERVER, ERROR, "failed to share time","packet sending error");
			goto TIMESHAREFAIL;
		}
	}

	if(contents){
		free((char*)contents);
		contents = NULL;
	}

	return 0;

TIMESHAREFAIL:
	((REQUEST_TIME_SHARE*)(p->ptr))->success = 0;

	contents = packet_to_json(*p);
	iso8859_1_to_utf8(contents, strlen(contents));
	size = sendHead->payload_length = strlen(contents);
	send_frame_head(client->fd, sendHead);

	if( write( client->fd, contents, size) <= 0 ){
		serverLog(WSSERVER, ERROR, "failed to share time","packet sending error");
	}

	free((char*)contents);
	contents = NULL;

	if( result ){
		mysql_free_result(result);
		result = NULL;
	}

	return 1;
}
