#include "Websocket.h"


#define KEYSIZE 16 				/* The size of the key in Hybi-00 */
#define BUFFERSIZE 8192 		/* Buffer size = 8KB */
#define MAXMESSAGE 1048576 		/* Max size message = 1MB */
#define ORIGIN_REQUIRED 0 		/* If this value is other than 0, client must 
								   supply origin in header */
#define DEV 1

#define BUFSIZE 8096
#define ERROR 42
#define SORRY 43
#define LOG   44

ws_list *l;

#if !defined(WS)
	#define WSPORT "12345"
#endif

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"php", "image/php" },
	{"cgi", "text/cgi"  },
	{"asp","text/asp"   },
	{"jsp", "image/jsp" },
	{"xml", "text/xml"  },
	{"js","text/js"     },
   	{"css","test/css"   },
	{0,0}
};

void serverLog(int type, char *s1, char *s2, int num){
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (type) {
		case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid()); break;
		return;

		case SORRY:
			(void)sprintf(logbuffer, "<HTML><BODY><H1>404 Not Found: %s %s</H1></BODY></HTML>\r\n", s1, s2);
			// (void)write(num,logbuffer,strlen(logbuffer));
			(void)sprintf(logbuffer,"SORRY: %s:%s",s1, s2);
			break;

		case LOG:
			(void)sprintf(logbuffer," INFO: %s | \n%s:%d\n",s1, s2,num);
			break;
	}	
	
	if( (fd = open("server.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0 ){
		(void)write(fd,logbuffer,strlen(logbuffer)); 
		(void)write(fd,"\n",1);
		
		(void)printf("[log] %s\n", logbuffer);

		(void)close(fd);
	}
}

void *web(void *args) {
	ws_client *n = args;
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	char buffer[BUFSIZE+1];

	int fd 		= n->socket_id;
	int hitCnt 	= n->client_idx;

	ret =read(fd,buffer,BUFSIZE);
	if(ret == 0 || ret == -1) {
		serverLog(SORRY,"failed to read browser request","",fd);
		goto END;
	}
	if(ret > 0 && ret < BUFSIZE)
		buffer[ret]=0;
	else buffer[0]=0;

	for(i=0;i<ret;i++){
		if(buffer[i] == '\r'){
			buffer[i]=' ';
		}
		else if( buffer[i] == '\n'){
			buffer[i] = '\n';
		}
	}
	serverLog(LOG,"request",buffer, hitCnt);

	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ){
		serverLog(SORRY,"Only simple GET operation supported",buffer,fd);
		goto END;
	}
	for(i=4;i<BUFSIZE;i++){
		if(buffer[i] == ' '){
			buffer[i] = 0;
			break;
		}
	}
	for(j=0;j<i-1;j++){
		if(buffer[j] == '.' && buffer[j+1] == '.'){
			serverLog(SORRY,"Parent directory (..) path names not supported",buffer,fd);
			goto END;
		}
	}

	if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) 
		(void)strcpy(buffer,"GET /index.html");

	buflen=strlen(buffer);
	fstr = (char *)0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr =extensions[i].filetype;
			break;
		}
	}
	if(fstr == 0) {
		serverLog(SORRY,"file extension type not supported",buffer,fd);
		goto END;
	}

	if(( file_fd = open(&buffer[5],O_RDONLY)) == -1){
		serverLog(SORRY, "failed to open file",&buffer[5],0);
		(void)sprintf(buffer,"HTTP/1.0 404 Not Found\r\n\r\n");
		(void)write(fd,buffer,sizeof(buffer));
		goto END;
	}

	serverLog(LOG,"SEND",&buffer[5],hitCnt);

	(void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	(void)write(fd,buffer,strlen(buffer));

	
	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
		(void)write(fd,buffer,ret);
		printf("%s", buffer);
	}
	

	#ifdef LINUX
		sleep(1);
	#endif

END:
	close(fd);
	pthread_detach(n->thread_id);

	return NULL;
}


void *webSocketServer(/*void *args*/){

	int port = atoi(WSPORT);
	#ifdef WS
		port = atoi(args[1]);
	#endif
	int server_sock, client_sock, on = 1;
	int hitCnt;

	pthread_attr_t pthread_attr;
	pthread_t pthread_id;
	pthread_attr_init(&pthread_attr);
	pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&pthread_attr, 524288);

	socklen_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;
	
	if((server_sock = socket(AF_INET, SOCK_STREAM,0)) <0)
		serverLog(ERROR, "system call","socket",0);

	
	/**
	 * Allow reuse of address, when the server shuts down.
	 */
	if ( (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0 ){
		server_error(strerror(errno), server_sock, l);
	}

	/**
	 * Create commandline, such that we can do simple commands on the server.
	 */
	if ( (pthread_create(&pthread_id, &pthread_attr, cmdline, NULL)) < 0 ){
		server_error(strerror(errno), server_sock, l);
	}

	/**
	 * Do not wait for the thread to terminate.
	 */
	pthread_detach(pthread_id);


	
	
	if(port < 0 || port >60000)
		serverLog(ERROR,"Invalid port number try [1,60000]",WSPORT,0);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if(bind(server_sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		serverLog(ERROR,"system call","bind",0);
	
	if( listen(server_sock,64) <0)
		serverLog(ERROR,"system call","listen",0);


	for(hitCnt=1; ;hitCnt++, pthread_id++) {
		#ifdef DEV
			printf("[System Dev] WS listening....\n");
		#endif

		length = sizeof(cli_addr);
		if( ( client_sock = accept(server_sock, (struct sockaddr *)&cli_addr, &length ) ) < 0 )
			serverLog(ERROR,"system call","accept",0);

		#ifdef DEV
			printf("[System Dev] WS Accepted with fd %d!\n", client_sock);
		#endif

		char *temp = (char *) inet_ntoa(cli_addr.sin_addr);
		char *addr = (char *) malloc( sizeof(char)*(strlen(temp)+1) );
		
		ws_client *n = client_new(client_sock, addr);
		n->client_idx = hitCnt;

		if ( (pthread_create(&pthread_id, &pthread_attr, handleClient, (void *) n)) < 0 ){
			server_error(strerror(errno), server_sock, l);
		}
	}
	list_free(l);
	l = NULL;
	close(server_sock);
	pthread_attr_destroy(&pthread_attr);
}

int main(int argc, char **argv){
	int i, port, server_sock, client_sock, hitCnt;
	socklen_t length;
	int on = 1;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;
	
	
	l = list_new();


	(void) signal(SIGINT, &sigint_handler);
	(void) signal(SIGSEGV, &sigint_handler);
	(void) signal(SIGPIPE, &sigint_handler);


	pthread_attr_t pthread_attr;
	pthread_t pthread_id;
	pthread_attr_init(&pthread_attr);
	pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&pthread_attr, 524288);

	// argv  ws port 들어올떄 처리하기 ( 값 websocketserver 에 넘겨주면 됨 )
	if( pthread_create(&pthread_id, &pthread_attr, webSocketServer, 0) < 0 ){
		serverLog(ERROR,"webSocketServer Thread Error","failed",0);
		return -1;
	}


	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		(void)printf("usage: server [port] [server directory] &\tExample: server 80 ./ &\n\n\tOnly Supports:");
		
		for(i=0;extensions[i].ext != 0;i++)
			(void)printf(" %s",extensions[i].ext);

		(void)printf("\n\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
		// exit(0);
		return 0;
	}

	if( !strncmp(argv[2],"/"   ,2 ) || !strncmp(argv[2],"/etc", 5 ) ||
	    !strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) ||
	    !strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) ||
	    !strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){
		(void)printf("ERROR: Bad top directory %s, see server -?\n",argv[2]);
		// exit(3);
		return 0;
	}

	#ifdef DEV
		printf("[System Dev] 경로 양호\n");
	#endif

	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
		// exit(4);
		return 0;
	}

	#ifdef DEV
		printf("[System Dev] 경로 이동 성공\n");
	#endif

	(void)signal(SIGCLD, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	for(i=4;i<32;i++)
		(void)close(i);
	// (void)setpgrp();	 // 데몬 안해 


	serverLog(LOG,"http server starting",argv[1],getpid());
	
	if((server_sock = socket(AF_INET, SOCK_STREAM,0)) <0)
		serverLog(ERROR, "system call","socket",0);

	
	/**
	 * Allow reuse of address, when the server shuts down.
	 */
	if ( (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0 ){
		server_error(strerror(errno), server_sock, l);
	}




	port = atoi(argv[1]);
	
	if(port < 0 || port >60000)
		serverLog(ERROR,"Invalid port number try [1,60000]",argv[1],0);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(server_sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		serverLog(ERROR,"system call","bind",0);
	
	if( listen(server_sock,64) <0)
		serverLog(ERROR,"system call","listen",0);


	for(hitCnt=1; ;hitCnt++, pthread_id++) {
		#ifdef DEV
			printf("[System Dev] listening....\n");
		#endif

		length = sizeof(cli_addr);
		if( ( client_sock = accept(server_sock, (struct sockaddr *)&cli_addr, &length ) ) < 0 )
			serverLog(ERROR,"system call","accept",0);

		#ifdef DEV
			printf("[System Dev] Accepted with fd %d!\n", client_sock);
		#endif

		char *temp = (char *) inet_ntoa(cli_addr.sin_addr);
		char *addr = (char *) malloc( sizeof(char)*(strlen(temp)+1) );
		
		ws_client *n = client_new(client_sock, addr);
		n->client_idx = hitCnt;

		#ifdef DEV
			printf("[System Dev]before function Web\n");
		#endif
		
		if( (pthread_create(&(n->thread_id), &pthread_attr, web, (void *) n)) < 0 ){
			printf("Thread_error!\n");
		}
		int status;
		pthread_join(n->thread_id, (void **)&status);
		#ifdef DEV
			printf("[System Dev]after function Web\n");
		#endif
	}
}
