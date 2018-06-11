/*
 
Message Protocol
"code" : 종류코드 ( 길이2 , 0X : 전체공유, 1X : 단순 요청, 2X : 서버 응답)
example : { "major_code": 0, "minor_code": 1, "ptr": { "msg": "Hello, Server! I'm rollcake!", "from": { "id": 404, "nickname": "rollcake" }, "timestamp":     { "tv_sec": 1528443375, "tv_usec": 496229 } } }
 
 */


#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "Includes.h"
#include "json/json.h"
#include "json/json_tokener.h" 



#define CHAT_SIZE 128
#define NICKNAME_SIZE 30
#define MAX_USER 6
#define MAX_ROOM 20
#define COLOR_SIZE 8

struct packet{
	int major_code;
	int minor_code;
	void *ptr;
};

typedef struct user_data{
	int uid;
	char nickname[NICKNAME_SIZE+1];
	int score;
}User;

typedef struct room_data{
	int id;
	int num;
	int status;
}Room;


/*client to server*/
// 닉네임
typedef struct nickname_register{ // 14 닉네임 등록
	char nickname[NICKNAME_SIZE+1];
}REQUEST_NICKNAME_REGISTER;

typedef struct set_score{ // 17 스코어  등록
	int score;
}REQUEST_SET_SCORE;



// 대기방 리스트 
typedef struct request_roomlist_msg{ //10 대기방 리스트 요청
	User from;
}REQUEST_ROOM_LIST;

typedef struct request_enter_msg{ //11 대기방 접속 요청
	User from;
	int room_id;
}REQUEST_ENTER_ROOM;

typedef struct make_room_msg{ // 15 대기방 생성
	User from;
}MAKE_ROOM;



// 대기방
typedef struct start_game_msg{ //12 게임 시작 요청
	int room_id;
}REQUEST_START;

typedef struct request_exit_msg{ // 16 대기방 나가기 
	User from;
	int room_id;
}REQUEST_EXIT_ROOM;



// 게임방 


typedef struct drawing_data{ //00 그림 데이터	
	int prevX;
	int prevY;
	int x;
	int y;
	char color[COLOR_SIZE+1];
	int px;
	int room_id;
}DRAW_DATA;

typedef struct chat_data{ //01 채팅 데이터
	char msg[CHAT_SIZE+1];
	User from;
	char timestamp[CHAT_SIZE+1];
	int room_id;
}CHAT_DATA;

typedef struct start_drawing_msg{ //02 그리기 시작 요청
	int room_id;
	User from;
}REQUEST_DRAWING_START;

typedef struct finish_drawing_msg{ //03 그리기 종료 요청
	int room_id;
	User from;
}REQUEST_DRAWING_END;

typedef struct finish_drawing_data{ //04 제한시간 정보
	int room_id;
	
	User from;
	int time;
}REQUEST_TIME_SHARE;

//-------------------1------------------------

typedef struct request_exit_gameroom_msg{ // 13 게임방 나가기
	int room_id;
	User from;
}REQUEST_EXIT_GAMEROOM;










/*server to client*/
// 닉네임 입력
typedef struct response_register_nickname{ // 30 닉네임 등록 응답 
	int success;
	User user;
}RESPONSE_REGISTER;

typedef struct response_set_score{ // 31 스코어 등록 응답 
	int success;
}RESPONSE_SET_SCORE;




// 대기방 리스트
typedef struct send_roomlist_data{ //20대기방 리스트
	int success;
	int idx;
	Room rlist[MAX_ROOM];
}ROOM_LIST_DATA;


// 대기방
typedef struct enter_room_data{ //21대기방 접속
	int success;
	Room room;
	User members[MAX_USER];
	int idx;
}ROOM_DATA;

typedef struct response_exit_room{ // 28 대기방 나가기 응답
	int success;
}RESPONSE_EXIT;

// 게임방 

//00 그림데이터는 재전송
//01 채팅데이터 재전송
typedef struct winner_data{ //05 정답자
	int room_id;
	User winner;
}WINNER_DATA;
//-------------------2------------------------

typedef struct add_player_data{ //22방 인원 추가
	Room room; 
	User user;
}ADDED_MEMBER_DATA;

typedef struct game_starting_data{ //23 게임 시작 알림
	int answerLen;
	User painter;
}NEW_ROUND_DATA;

typedef struct answer_data{ //24 정답정보, painter에게 전송
	char answer[CHAT_SIZE+1];
}ANSWER_DATA;

typedef struct invalid_room_data{ //25 방이 사라짐(문제 발생)
	int room_id;
}INVALID_ROOM_DATA;

typedef struct game_finished_data{ //26 그리기 종료 알림
	char answer[CHAT_SIZE+1];
}END_ROUND_DATA;

typedef struct response_exit_gameroom{ // 27 게임방 나가기 응답
	int success;
}RESPONSE_EXIT_GAMEROOM;


typedef struct pop_player_data{ // 32 방 인원 감소
	Room room;
	User user;
}POP_MEMBER_DATA;


typedef struct leader_changed{ // 33 방 상태 변경 ( 리더는 항시, 상태값은 필요에 따라 )
	Room room;
	User leader;
}STATUS_CHANGED;

const char * packet_to_json(struct packet p);
int json_to_packet(const char * json_string, struct packet * p);

#endif
