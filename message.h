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
	int room_id;
	
	int prevX;
	int prevY;
	int x;
	int y;
	int color;
	int px;
}DRAW_DATA;

typedef struct chat_data{ //01 채팅 데이터
	int room_id;
	
	char msg[CHAT_SIZE];
	User from;
	char timestamp[CHAT_SIZE];
}CHAT_DATA;

struct start_drawing_msg{ //02 그리기 시작 요청
	int room_id;
	User user;
};
struct finish_drawing_msg{ //03 그리기 종료 요청
	int room_id;
};
struct finish_drawing_data{ //04 제한시간 정보
	int room_id;
	
	User user;
	int time;
};

//-------------------1------------------------

typedef struct request_roomlist_msg{ //10 대기방 리스트 요청
	int uid;

	int idx;
}REQUEST_ROOM_LIST;

typedef struct request_enter_msg{ //11 대기방 접속 요청
	int uid;

	int room_id;
}REQUEST_ENTER_ROOM;

typedef struct start_game_msg{ //12 게임 시작 요청
	int room_id;
}REQUEST_START;


/*server to client*/
//00 그림데이터는 재전송
//01 채팅데이터 재전송
typedef struct winner_data{ //05 정답자
	int room_id;

	User winner;
}WINNER_DATA;
//-------------------2------------------------
typedef struct send_roomlist_data{ //20대기방 리스트
	Room rlist[MAX_ROOM];
}ROOM_LIST_DATA;

typedef struct enter_room_data{ //21대기방 접속
	int room_id;
	User members[MAX_USER];
}ROOM_DATA;

typedef struct add_player_data{ //22방 인원 추가
	Room room; 
	User user;
}ADDED_MEMBER_DATA;

typedef struct game_starting_data{ //23 게임 시작 알림
	int answerLen;
	User painter;
}NEW_ROUND_DATA;

typedef struct answer_data{ //24 정답정보, painter에게 전송
	char answer[CHAT_SIZE];
}ANSWER_DATA;

typedef struct invalid_room_data{ //25 방이 사라짐
	int room_id;
}INVALID_ROOM_DATA;

typedef struct game_finished_data{ //26 게임 종료 알림
	char answer[CHAT_SIZE];
}END_ROUND_DATA;


const char * packet_to_json(struct packet p);
int json_to_packet(const char * json_string, struct packet * p);

#endif
