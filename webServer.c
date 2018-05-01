#include "webServer.h"

extern pthread_attr_t pthread_attr;
extern pthread_t pthread_id;

struct extension_type{
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
	{"js",	"text/js"   },
   	{"css","text/css"   },
   	{"ttf", "font/ttf"	},
   	{"eot", "font/eot"	},
   	{"otf", "font/otf"	},
   	{"opentype", "font/opentype"},
	{0,0}
};

void webServerLog(int type, char *s1, char *s2, int num){
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (type) {
		case ERROR: 
			(void)sprintf(logbuffer,"[ ERROR ] %s:%s Errno=%d exiting pid=%d",s1, s2, errno,getpid()); 
			break;
			return;

		case SORRY:
			(void)sprintf(logbuffer,"[ SORRY ] 404 not found %s:%s",s1, s2);
			break;

		case LOG:
			(void)printf("Web Server >> [INFO: %s ] %s\n",s1, s2);

		case FILELOG:
			(void)sprintf(logbuffer,"[ INFO: %s ] %s:%d\n",s1, s2, num);
			break;
	}	
	
	if( (fd = open("server.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0 ){
		(void)write(fd,logbuffer,strlen(logbuffer)); 
		(void)write(fd,"\n",1);

		(void)close(fd);
	}
}

web_client_info* web_client_new(int client_sock, char* addr){
	web_client_info *n = (web_client_info *) malloc(sizeof(web_client_info));

	n->socket_id = client_sock;
	n->client_ip = addr;

	return n;
}

void *web(void *args) {
	web_client_info *n = args;
	int j, file_fd, buflen, len, STATUSCODE = 200;
	long i, ret;
	char * fstr;
	char buffer[BUFSIZE+1];
	char* logBuffer;


	int fd 		= n->socket_id;
	int hitCnt 	= n->client_idx;
	


	ret =read(fd,buffer,BUFSIZE);
	if(ret == 0 || ret == -1) {
		webServerLog(SORRY,"failed to read browser request","",fd);
		goto IGNORE;
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

	webServerLog(FILELOG,"request",buffer, hitCnt);


	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ){
		webServerLog(SORRY,"Only simple GET operation supported",buffer,fd);
		goto IGNORE;
	}
	for(i=4;i<BUFSIZE;i++){
		if(buffer[i] == ' '){
			buffer[i] = 0;
			break;
		}
	}
	for(j=0;j<i-1;j++){
		if(buffer[j] == '.' && buffer[j+1] == '.'){
			webServerLog(SORRY,"Parent directory (..) path names not supported",buffer,fd);
			goto IGNORE;
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
		webServerLog(SORRY,"file extension type not supported",buffer,fd);
		goto IGNORE;
	}

	char* fileName =  (char *) malloc( sizeof(char)*(strlen(&buffer[5])+1) );
	strcpy(fileName, &buffer[5]);
	if( fileName)

	if(( file_fd = open(&buffer[5],O_RDONLY)) == -1){
		webServerLog(SORRY, "failed to open file",fileName,0);

		// 404 error send
		(void)sprintf(buffer,"HTTP/1.0 404 Not Found\r\n\r\n");
		(void)write(fd,buffer,strlen(buffer));

		// 404 error page send
		file_fd = open("ERROR/error.html", O_RDONLY);
		while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
			(void)write(fd,buffer,ret);
		}
		(void)sprintf(buffer,"The requested : <span>%s</span> was not found on this server", fileName);
		(void)write(fd,buffer,strlen(buffer));

		STATUSCODE = 404;

		goto END;
	}

	// 200 OK send
	webServerLog(FILELOG,"SEND",&buffer[5],hitCnt);
	(void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	(void)write(fd,buffer,strlen(buffer));

	// 202 OK file send
	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
		(void)write(fd,buffer,ret);
	}



END:

	logBuffer = (char*)malloc(sizeof(char)*BUFSIZE);
	sprintf(logBuffer, "request %s \t\t%d", fileName, STATUSCODE);
	webServerLog(LOG,"SEND",logBuffer, hitCnt);

IGNORE:
	close(fd);
	pthread_detach(n->thread_id);

	return NULL;
}

int webServerHandle(int argc, char** argv){
	int i, port, server_sock, client_sock, hitCnt;
	socklen_t length;
	int on = 1;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;


	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		(void)printf("usage: server [port] [server directory] &\tExample: server 80 ./");
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
		exit(4);
		return 0;
	}

	#ifdef DEV
		printf("[System Dev] 경로 이동 성공\n");
	#endif

	for(i=4;i<32;i++)
		(void)close(i);
	// (void)setpgrp();	 // 데몬 안해 

	printf("Web Server >> http server starting ( port: %s, pid: %d )\n",argv[1], getpid());
	webServerLog(FILELOG,"http server starting",argv[1], getpid());
	
	if((server_sock = socket(AF_INET, SOCK_STREAM,0)) <0)
		webServerLog(ERROR, "system call","socket",0);

	
	/**
	 * Allow reuse of address, when the server shuts down.
	 */
	if ( (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0 ){
		// server_error(strerror(errno), server_sock);
		error_handler(strerror(errno));
	}


	port = atoi(argv[1]);
	
	if(port < 0 || port >60000)
		webServerLog(ERROR,"Invalid port number try [1,60000]",argv[1],0);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(server_sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		webServerLog(ERROR,"system call","bind",0);
	
	if( listen(server_sock,64) <0)
		webServerLog(ERROR,"system call","listen",0);


	for(hitCnt=1; ;hitCnt++, pthread_id++) {
		#ifdef DEV
			printf("[System Dev] listening....\n");
		#endif

		length = sizeof(cli_addr);
		if( ( client_sock = accept(server_sock, (struct sockaddr *)&cli_addr, &length ) ) < 0 )
			webServerLog(ERROR,"system call","accept",0);

		#ifdef DEV
			printf("[System Dev] Accepted with fd %d!\n", client_sock);
		#endif

		char *temp = (char *) inet_ntoa(cli_addr.sin_addr);
		char *addr = (char *) malloc( sizeof(char)*(strlen(temp)+1) );
		
		web_client_info *n = web_client_new(client_sock, addr);
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

	return 0;
}

