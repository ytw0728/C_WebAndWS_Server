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
   	{"ico", "image/x-icon"	},
   	{"opentype", "font/opentype"},
	{0,0}
};

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
		serverLog(WEBSERVER,ERROR,"failed to read browser request","");
		goto IGNORE;
	}
	if(ret > 0 && ret < BUFSIZE) buffer[ret]=0;
	else buffer[0]=0;

	for(i=0;i<ret;i++){
		if(buffer[i] == '\r'){
			buffer[i]=' ';
		}
		else if( buffer[i] == '\n'){
			buffer[i] = '\n';
		}
	}

	serverLog(WEBSERVER,FILELOG,"request",buffer);

	if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ){
		serverLog(WEBSERVER,ERROR,"Only simple GET operation supported",buffer);
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
			serverLog(WEBSERVER,ERROR,"Parent directory (..) path names not supported",buffer);
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
		serverLog(WEBSERVER,ERROR,"file extension type not supported",buffer);
		goto IGNORE;
	}

	char* fileName =  (char *) malloc( sizeof(char)*(strlen(&buffer[5])+1) );
	strcpy(fileName, &buffer[5]);
	if( fileName)

	if(( file_fd = open(&buffer[5],O_RDONLY)) == -1){
		serverLog(WEBSERVER,ERROR, "failed to open file",fileName);

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
	serverLog(WEBSERVER,FILELOG,&buffer[5],"SEND");
	(void)sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	(void)write(fd,buffer,strlen(buffer));

	// 202 OK file send
	while (	(ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {
		(void)write(fd,buffer,ret);
	}

END:
	logBuffer = (char*)malloc(sizeof(char)*BUFSIZE);
	sprintf(logBuffer, "request %s \t\t%d", fileName, STATUSCODE);
	serverLog(WEBSERVER,LOG,logBuffer,"SEND");

IGNORE:
	close(fd);
	pthread_detach(n->thread_id);

	return NULL;
}

int webServerHandle(int argc, char** argv){
	serverLog(WEBSERVER,LOG,"WEB SERVER FUNCTION START", "MESSAGE"); // pid 넣을지말지 

	int i, port, server_sock, client_sock, hitCnt;
	socklen_t length;
	int on = 1;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;
	char logBuffer[1024];

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		serverLog(WEBSERVER,ERROR,"usage: server [port] [server directory] ", "Example: server 80 ./");
		serverLog(WEBSERVER,ERROR, "Not Supported directories", "/ /etc /bin /lib /tmp /usr /dev /sbin" );
		exit(EXIT_FAILURE);
	}

	if( !strncmp(argv[2],"/"   ,2 ) || !strncmp(argv[2],"/etc", 5 ) ||
	    !strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) ||
	    !strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) ||
	    !strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){
		serverLog(WEBSERVER, ERROR, "Bad top directory", argv[2]);
		exit(EXIT_FAILURE);
	}


	serverLog(WEBSERVER, LOG, "경로 양호", "SEND");	


	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
		exit(EXIT_FAILURE);
		return 0;
	}

	serverLog(WEBSERVER, LOG, "경로 이동 성공", "SEND");
	
	
	if((server_sock = socket(AF_INET, SOCK_STREAM,0)) <0)
		serverLog(WEBSERVER,ERROR, "system call error","socket()");

	
	/**
	 * Allow reuse of address, when the server shuts down.
	 */
	if ( (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0 ){
		serverLog(WEBSERVER, ERROR, "webServer setting Socket option Error\n","");
		error_handler(strerror(errno));
	}


	port = atoi(argv[1]);
	
	if(port < 0 || port >60000)
		serverLog(WEBSERVER,ERROR,"Invalid port number try [1,60000]",argv[1]);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(server_sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		serverLog(WEBSERVER,ERROR,"system call error","bind()");
	
	if( listen(server_sock,64) <0)
		serverLog(WEBSERVER,ERROR,"system call error","listen()");

	serverLog(WEBSERVER,LOG,"WEB SERVER SERVICE START", "MESSAGE"); // pid 넣을지말지 

	for(hitCnt=1; ;hitCnt++) {

		// serverLog(WEBSERVER, LOG, "Listening","")

		length = sizeof(cli_addr);
		if( ( client_sock = accept(server_sock, (struct sockaddr *)&cli_addr, &length ) ) < 0 )
			serverLog(WEBSERVER,ERROR,"system call error","accept()");

		sprintf(logBuffer,"fd = %d", client_sock);
		serverLog(WEBSERVER, LOG, "Accepted",logBuffer);	

		char *temp = (char *) inet_ntoa(cli_addr.sin_addr);
		char *addr = (char *) malloc( sizeof(char)*(strlen(temp)+1) );
		
		web_client_info *n = web_client_new(client_sock, addr);
		n->client_idx = hitCnt;
		
		if( (pthread_create(&(n->thread_id), &pthread_attr, web, (void *) n)) < 0 ){
			serverLog(WEBSERVER, ERROR, "Thread Error", "");
		}
		int status;
		pthread_join(n->thread_id, (void **)&status);
	}

	return 0;
}

