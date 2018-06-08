#include "message.h"

/*
   todo
   데이터 유형별로 파싱하는 거 다 만들기
   데이터 코드 겹치는 거 있음 수정(02번)
   makefile 연결해야함
 */

const char * packet_to_json(struct packet p)
{
	const char *ptr;
	int major_code = p.major_code;
	int minor_code = p.minor_code;

	json_object * obj = json_object_new_object();
	json_object * pobj = json_object_new_object();

	json_object_object_add(pobj, "major_code", json_object_new_int(major_code));
	json_object_object_add(pobj, "minor_code", json_object_new_int(minor_code));

	if(major_code == 0){ //echo
		if(minor_code == 0){
			printf("00그림 그리기\n");

			json_object_object_add(obj, "prevX", json_object_new_int(((DRAW_DATA *)(p.ptr))->prevX));
			json_object_object_add(obj, "prevY", json_object_new_int(((DRAW_DATA *)(p.ptr))->prevY));
			json_object_object_add(obj, "x", json_object_new_int(((DRAW_DATA *)(p.ptr))->x));
			json_object_object_add(obj, "y", json_object_new_int(((DRAW_DATA *)(p.ptr))->y));
			json_object_object_add(obj, "color", json_object_new_int(((DRAW_DATA *)(p.ptr))->color));
			json_object_object_add(obj, "px", json_object_new_int(((DRAW_DATA *)(p.ptr))->px));

			json_object_object_add(pobj, "ptr", obj);

			ptr = json_object_to_json_string(pobj);
		}
		else if(minor_code == 1){
			printf("01채팅 보내기\n");

			json_object * uobj;
			uobj = json_object_new_object();

			json_object_object_add(obj, "msg", json_object_new_string(((CHAT_DATA *)(p.ptr))->msg));

			json_object_object_add(uobj, "id", json_object_new_int(((CHAT_DATA *)(p.ptr))->from.id));
			json_object_object_add(uobj, "nickname", json_object_new_string(((CHAT_DATA *)(p.ptr))->from.nickname));
			json_object_object_add(obj, "from", uobj);

			json_object_object_add(obj, "timestamp", json_object_new_string(((CHAT_DATA *)(p.ptr))->timestamp));

			json_object_object_add(pobj, "ptr", obj);

			ptr = json_object_to_json_string(pobj);

			free(uobj);
		}
		else if(minor_code == 2){
			printf("02게임 시작 요청(호스트)\n");

		}
		else if(minor_code == 3){
			printf("03게임 종료 요청(그림 그리는 사람)\n");
		}
		else if(minor_code == 4){
			printf("04제한 시간 정보(그림 그리는 사람)\n");
		}
		else if(minor_code == 5){
			printf("05정답자 데이터(그림 그리는 사람)\n");

			json_object * uobj;
			uobj = json_object_new_object();

			json_object_object_add(uobj, "id", json_object_new_int(((WINNER_DATA *)(p.ptr))->winner.id));
			json_object_object_add(uobj, "nickname", json_object_new_string(((WINNER_DATA *)(p.ptr))->winner.nickname));
			json_object_object_add(obj, "winner", uobj);

			json_object_object_add(pobj, "ptr", obj);

			ptr = json_object_to_json_string(pobj);

			free(uobj);
		}
	}
	else if(major_code == 1){ //simple request
		if(minor_code == 0){
			printf("10대기방 리스트 요청\n");

			json_object_object_add(obj, "idx", json_object_new_int(((REQUEST_ROOM_LIST *)(p.ptr))->idx));
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
		}
		else if(minor_code == 1){
			printf("11대기방 접속 요청\n");

			json_object_object_add(obj, "room_id", json_object_new_int(((REQUEST_ENTER_ROOM *)(p.ptr))->room_id));
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
		}
		else if(minor_code == 2){
			printf("12게임 시작 요청\n");

			json_object_object_add(obj, "room_id", json_object_new_int(((REQUEST_START *)(p.ptr))->room_id));
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
		}

	}
	else if(major_code == 2){ //server response	
		if(minor_code == 0){
			printf("20대기방 리스트 응답\n");
		}
		else if(minor_code == 1){
			printf("21대기방 접속\n");
		}
		else if(minor_code == 2){
			printf("22방 인원 추가\n");
		}
		else if(minor_code == 3){
			printf("23게임 시작 알림\n");
		}
		else if(minor_code == 4){
			printf("24정답 알림\n");
		}
		else if(minor_code == 5){
			printf("25방이 사라짐\n");
		}
		else if(minor_code == 6){
			printf("26그리기 종료 알림\n");
		}
	}

	free(obj);
	free(pobj);
	return ptr;
}

int json_to_packet(const char * json_string, struct packet * p)
{
	json_object * jobj, * jbuf, *obj;
	printf("j2p::json_string : %s\n", json_string);//debug
	obj = json_tokener_parse(json_string); //read json

	///*
	//printf("json string : %s\n", json_object_to_json_string(jobj));//debug

	json_object_object_get_ex(obj, "major_code", &jbuf);
	int major_code = json_object_get_int(jbuf);
	json_object_object_get_ex(obj, "minor_code", &jbuf);
	int minor_code = json_object_get_int(jbuf);

	json_object_object_get_ex(obj, "ptr", &jobj);

	p->major_code = major_code;
	p->minor_code = minor_code;

	printf("j2p::code : %d%d\n", p->major_code, p->minor_code);//debug

	if(major_code == 0){ //echo
		if(minor_code == 0){	
			printf("00그림 그리기\n");
			p->ptr = (void *)((DRAW_DATA *)malloc(sizeof(DRAW_DATA)));

			json_object_object_get_ex(jobj, "prevX", &jbuf);
			((DRAW_DATA *)(p->ptr))->prevX = json_object_get_int(jbuf);
			json_object_object_get_ex(jobj, "prevY", &jbuf);
			((DRAW_DATA *)(p->ptr))->prevY = json_object_get_int(jbuf);
			json_object_object_get_ex(jobj, "x", &jbuf);
			((DRAW_DATA *)(p->ptr))->x = json_object_get_int(jbuf);
			json_object_object_get_ex(jobj, "y", &jbuf);
			((DRAW_DATA *)(p->ptr))->y = json_object_get_int(jbuf);
			json_object_object_get_ex(jobj, "color", &jbuf);
			((DRAW_DATA *)(p->ptr))->color = json_object_get_int(jbuf);
			json_object_object_get_ex(jobj, "px", &jbuf);
			((DRAW_DATA *)(p->ptr))->px = json_object_get_int(jbuf);

			///*
			printf("j2p::<drawing>");   
			printf("(%d, %d)->(%d, %d)\n", ((DRAW_DATA *)(p->ptr))->prevX, ((DRAW_DATA *)(p->ptr))->prevY, ((DRAW_DATA *)(p->ptr))->x, ((DRAW_DATA *)(p->ptr))->y);
			printf("color : %d, px : %d\n", ((DRAW_DATA *)(p->ptr))->color, ((DRAW_DATA *)(p->ptr))->px); //debug */
		}
		else if(minor_code == 1){
			printf("01채팅 보내기\n");
			json_object * uobj;

			p->ptr = (void *)((CHAT_DATA *)malloc(sizeof(CHAT_DATA)));

			json_object_object_get_ex(jobj, "msg", &jbuf);
			strcpy(((CHAT_DATA *)(p->ptr))->msg, json_object_get_string(jbuf));

			json_object_object_get_ex(jobj, "from", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((CHAT_DATA *)(p->ptr))->from.id = json_object_get_int(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((CHAT_DATA *)(p->ptr))->from.nickname, json_object_get_string(jbuf));

			json_object_object_get_ex(jobj, "timestamp", &jbuf);
			strcpy(((CHAT_DATA *)(p->ptr))->timestamp, json_object_get_string(jbuf));

			///*
			printf("j2p::<chat data>");   
			printf("[%s](#%d)%s: %s\n", ((CHAT_DATA *)(p->ptr))->timestamp,  ((CHAT_DATA *)(p->ptr))->from.id, ((CHAT_DATA *)(p->ptr))->from.nickname, ((CHAT_DATA *)(p->ptr))->msg);
			free(uobj);
		}
		else if(minor_code == 2){
			printf("02게임 시작 요청(호스트)\n");
		}
		else if(minor_code == 3){
			printf("03게임 종료 요청(그림 그리는 사람)\n");
		}
		else if(minor_code == 4){
			printf("04제한 시간 정보(그림 그리는 사람)\n");
		}
		else if(minor_code == 5){
			printf("05정답자 데이터(그림 그리는 사람)\n");
			json_object * uobj;

			p->ptr = (void *)((CHAT_DATA *)malloc(sizeof(CHAT_DATA)));

			json_object_object_get_ex(jobj, "winner", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((WINNER_DATA *)(p->ptr))->winner.id = json_object_get_int(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((WINNER_DATA *)(p->ptr))->winner.nickname, json_object_get_string(jbuf));

			///*
			printf("j2p::<winner data>");   
			printf("WINNER!: (#%d)%s\n", ((WINNER_DATA *)(p->ptr))->winner.id, ((WINNER_DATA *)(p->ptr))->winner.nickname);

			free(uobj);
		}
	}
	else if(major_code == 1){ //simple request
		if(minor_code == 0){
			printf("10대기방 리스트 요청\n");
			p->ptr = (void *)((REQUEST_ROOM_LIST *)malloc(sizeof(REQUEST_ROOM_LIST)));

			json_object_object_get_ex(jobj, "idx", &jbuf);
			((REQUEST_ROOM_LIST *)(p->ptr))->idx = json_object_get_int(jbuf);

			printf("j2p::<request room list>");   
			printf("requested room list idx: %d\n", ((REQUEST_ROOM_LIST *)(p->ptr))->idx);
		}
		else if(minor_code == 1){
			printf("11대기방 접속 요청\n");
			p->ptr = (void *)((REQUEST_ENTER_ROOM *)malloc(sizeof(REQUEST_ENTER_ROOM)));

			json_object_object_get_ex(jobj, "room_id", &jbuf);
			((REQUEST_ENTER_ROOM *)(p->ptr))->room_id = json_object_get_int(jbuf);

			printf("j2p::<request entering room>");   
			printf("room_id to enter: %d\n", ((REQUEST_ENTER_ROOM *)(p->ptr))->room_id);
		}
		else if(minor_code == 2){
			printf("12게임 시작 요청\n");
			p->ptr = (void *)((REQUEST_START *)malloc(sizeof(REQUEST_START)));

			json_object_object_get_ex(jobj, "room_id", &jbuf);
			((REQUEST_START *)(p->ptr))->room_id = json_object_get_int(jbuf);

			printf("j2p::<request start game>");   
			printf("room_id to start: %d\n", ((REQUEST_START *)(p->ptr))->room_id);
		}


	}
	else if(major_code == 2){ //server response
		if(minor_code == 0){
			printf("20대기방 리스트 응답\n");
		}
		else if(minor_code == 1){
			printf("21대기방 접속\n");
		}
		else if(minor_code == 2){
			printf("22방 인원 추가\n");
		}
		else if(minor_code == 3){
			printf("23게임 시작 알림\n");
		}
		else if(minor_code == 4){
			printf("24정답 알림\n");
		}
		else if(minor_code == 5){
			printf("25방이 사라짐\n");
		}
		else if(minor_code == 6){
			printf("26그리기 종료 알림\n");
		}
	}

	free(jbuf);
	free(jobj);
	free(obj);
	return 0;
}


/*/----------------this is test-----------------
int main()
{
	const char *json_string;
	struct packet p;

	p.major_code = 2;
	p.minor_code = 0;
	p.ptr = (void *)((WINNER_DATA *)malloc(sizeof(WINNER_DATA)));
	((WINNER_DATA *)(p.ptr))->winner.id = 33;
	strcpy(((WINNER_DATA *)(p.ptr))->winner.nickname, "ROLLCAKE");

	json_string = packet_to_json(p);
	free(p.ptr);
	printf("json_string : %s\n\n", json_string);//debug

	json_to_packet(json_string, &p);

	free((void *)json_string);
	free(p.ptr);
}// */
