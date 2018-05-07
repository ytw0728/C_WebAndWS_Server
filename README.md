# Doodle with WS
> 2018 네트워크프로그래밍 프로젝트 | C로 구현한 WS 게임


## BUILD 
```sh
make
```

## RUN 
```sh
make run PORT=80 DIR=./pages
```

* PORT : port of webServer
* DIR : ROOT DIR of resources

++ websocket port : 12345 ( default )

## DEV
'''sh
make dev PORT=80 DIR='/pages
'''

print log를 콘솔 상에 출력합니다.
make run 은 print log 를 출력하지 않음.

## MEMO
libssl 및 libcrypto 설치를 위해 다음을 실행해주세요. ( ubuntu )
```sh
sudo apt-get install libssl-dev
```

## PAGES
* index.html : doodle index page
* testWeb.html : page to test ws ( echo )


## File Hierarchical Structure
```sh
Includes.h 	┬ webServer.h 	    ┬ server.c 
		┕ webSocketServer.h ┘
```
