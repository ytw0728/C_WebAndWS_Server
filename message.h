/*
 
Message Protocol
"code" : 종류코드 ( 길이2 , 0X : 전체공유, 1X : 단순 요청, 2X : 서버 응답)
example : 00{"major_code":0, "minor_code":0, "awefawef":"asdf", ... }
 
 */


#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "Includes.h"
#include "json/json.h"
#include "json/json_tokener.h" 
#include <sys/time.h> //timeval

#define CHAT_SIZE 128
#define NICKNAME_SIZE 30
#define MAX_USER 6
#define MAX_ROOM 20

struct packet{
	int major_code;
	int minor_code;
	void *ptr;
};


typedef struct user_data{
	int id;
	char nickname[NICKNAME_SIZE];
}User;

typedef struct room_data{
	int id;
	int num;
}Room;

/*client to server*/
typedef struct drawing_data{ //00 그림 데이터
	int prevX;
	int prevY;
	int x;
	int y;
	int color;
	int px;
}DRAW_DATA;

typedef struct chat_data{ //01 채팅 데이터
	char msg[CHAT_SIZE];
	User from;
	//struct timeval timestamp;
	char timestamp[CHAT_SIZE];
}CHAT_DATA;

typedef struct request_roomlist_msg{ //10 대기방 리스트 요청
	int idx;
}REQUEST_ROOM_LIST;

typedef struct request_enter_msg{ //11 대기방 접속 요청
	int room_id;
}REQUEST_ENTER_ROOM;

typedef struct start_game_msg{ //12 게임 시작 요청
	int room_id;
}REQUEST_START;

struct start_drawing_msg{ //02 그리기 시작 요청
	int major_code;
	int minor_code;
};
struct finish_drawing_msg{ //03 그리기 종료 요청
	int major_code;
	int minor_code;
};
struct finish_drawing_data{ //04 제한시간 정보
	int major_code;
	int minor_code;
	struct timeval timestamp;
};

/*server to client*/
//00 그림데이터는 재전송
/*struct chat_data{ //01 채팅 데이터
	int major_code;
	int minor_code;

	char msg[CHAT_SIZE];
	User from;
	//timestamp //js에 어떻게 적용할까
};*/
typedef struct winner_data{ //05 정답자
	User winner;
}WINNER_DATA;

struct send_roomlist_data{ //20대기방 리스트
	int major_code;
	int minor_code;

	Room rlist[MAX_ROOM];
};
struct enter_room_data{ //21대기방 접속
	int major_code;
	int minor_code;

	User members[MAX_USER];
	int room_id;
};
struct add_player_data{ //22방 인원 추가
	int major_code;
	int minor_code;

	Room room; 
	User user;
};
struct game_starting_data{ //23 게임 시작 알림
	int major_code;
	int minor_code;

	int answerLen;
	User painter;
};
struct answer_data{ //24 정답정보, painter에게 전송
	int major_code;
	int minor_code;

	char answer[CHAT_SIZE];
};
struct invalid_room_data{ //25 방이 사라짐
	int major_code;
	int minor_code;

	int room_id;
};
struct game_finished_data{ //26 게임 종료 알림
	int major_code;
	int minor_code;
	
	char answer[CHAT_SIZE];
	User painter;
};


#endif
