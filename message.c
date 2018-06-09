#include "message.h"

/*
   todo
   헤더파일 맞게 수정하기
   데이터 유형별로 파싱하는 거 다 만들기(괄호가 완성한 코드)
   요청 패킷, uid 추가, 전체 공유 데이터 room_id추가
   (00) (01) 02 03 04 (05) (10) (11) (12) (20) (21) (22) (23) (24) (25) (26)
   다 만든 json string utf8로 컨버트 - utf8 string으로 바꿔주는 함수 있지 않나?
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

			json_object * aobj, *robj;
			aobj = json_object_new_array();

			for(int i = 0; i < 20; i++){
				robj = json_object_new_object();
				json_object_object_add(robj, "id", json_object_new_int(((ROOM_LIST_DATA *)(p.ptr))->rlist[i].id));
				json_object_object_add(robj, "num", json_object_new_int(((ROOM_LIST_DATA *)(p.ptr))->rlist[i].num));

				json_object_array_add(aobj, robj);
			}
			json_object_object_add(obj, "rlist", aobj);
			json_object_object_add(pobj, "ptr", obj);

			ptr = json_object_to_json_string(pobj);
			for(int i = 0; i < json_object_array_length(aobj); i++)
				free(json_object_array_get_idx(aobj, i));
			free(aobj);
		}
		else if(minor_code == 1){
			printf("21대기방 접속\n");
			
			json_object * aobj, *uobj;
			aobj = json_object_new_array();
			
			json_object_object_add(obj, "room_id", json_object_new_int(((ROOM_DATA *)(p.ptr))->room_id));
			for(int i = 0; i < 6; i++){
				uobj = json_object_new_object();
				json_object_object_add(uobj, "id", json_object_new_int(((ROOM_DATA *)(p.ptr))->members[i].id));
				json_object_object_add(uobj, "nickname", json_object_new_string(((ROOM_DATA *)(p.ptr))->members[i].nickname));

				json_object_array_add(aobj, uobj);
			}
			json_object_object_add(obj, "members", aobj);
			json_object_object_add(pobj, "ptr", obj);

			ptr = json_object_to_json_string(pobj);
			for(int i = 0; i < json_object_array_length(aobj); i++)
				free(json_object_array_get_idx(aobj, i));
			free(aobj);
		}
		else if(minor_code == 2){
			printf("22방 인원 추가\n");
			
			json_object * uobj, *robj;
			uobj = json_object_new_object();
			robj = json_object_new_object();

			json_object_object_add(robj, "id", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->room.id));
			json_object_object_add(robj, "num", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->room.num));
			json_object_object_add(obj, "room", robj);
			
			json_object_object_add(uobj, "id", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->user.id));
			json_object_object_add(uobj, "nickname", json_object_new_string(((ADDED_MEMBER_DATA *)(p.ptr))->user.nickname));
			json_object_object_add(obj, "user", uobj);

			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
			free(robj);
			free(uobj);

		}
		else if(minor_code == 3){
			printf("23게임 시작 알림\n");
			
			json_object * uobj;

			json_object_object_add(obj, "answerLen", json_object_new_int(((NEW_ROUND_DATA *)(p.ptr))->answerLen));

			uobj = json_object_new_object();
			json_object_object_add(uobj, "id", json_object_new_int(((NEW_ROUND_DATA *)(p.ptr))->painter.id));
			json_object_object_add(uobj, "nickname", json_object_new_string(((NEW_ROUND_DATA *)(p.ptr))->painter.nickname));
			json_object_object_add(obj, "painter", uobj);
			
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);

			free(uobj);
		}
		else if(minor_code == 4){
			printf("24정답 알림\n");

			json_object_object_add(obj, "answer", json_object_new_string(((ANSWER_DATA *)(p.ptr))->answer));
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
		}
		else if(minor_code == 5){
			printf("25방이 사라짐\n");
			
			json_object_object_add(obj, "room_id", json_object_new_int(((INVALID_ROOM_DATA *)(p.ptr))->room_id));
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
		}
		else if(minor_code == 6){
			printf("26그리기 종료 알림\n");
		
			json_object_object_add(obj, "answer", json_object_new_string(((END_ROUND_DATA *)(p.ptr))->answer));
			json_object_object_add(pobj, "ptr", obj);
			ptr = json_object_to_json_string(pobj);
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
			json_object * aobj, * robj;

			printf("20대기방 리스트 응답\n");
			
			p->ptr = (void *)((ROOM_LIST_DATA *)malloc(sizeof(ROOM_LIST_DATA)));

			json_object_object_get_ex(jobj, "rlist", &aobj);
			for(int i = 0; i < json_object_array_length(aobj); i++){
				robj = json_object_array_get_idx(aobj, i);

				json_object_object_get_ex(robj, "id", &jbuf);
				((ROOM_LIST_DATA *)(p->ptr))->rlist[i].id = json_object_get_int(jbuf);
				free(jbuf);
				json_object_object_get_ex(robj, "num", &jbuf);
				((ROOM_LIST_DATA *)(p->ptr))->rlist[i].num = json_object_get_int(jbuf);
				free(jbuf);
				free(robj);
			}
			
			for(int i = 0; i < json_object_array_length(aobj); i++)
				free(json_object_array_get_idx(aobj, i));
			free(aobj);

			printf("j2p::<from server : roomdata>\n");
			for(int i = 0; i < 20; i++){
				printf("room#%d, users : %d\n", ((ROOM_LIST_DATA *)(p->ptr))->rlist[i].id, ((ROOM_LIST_DATA *)(p->ptr))->rlist[i].num);
			}
		}
		else if(minor_code == 1){
			json_object * aobj, * robj;
			
			printf("21대기방 접속\n");
			p->ptr = (void *)((ROOM_DATA *)malloc(sizeof(ROOM_DATA)));

			json_object_object_get_ex(jobj, "room_id", &jbuf);
			((ROOM_DATA *)(p->ptr))->room_id = json_object_get_int(jbuf);
			free(jbuf);

			json_object_object_get_ex(jobj, "members", &aobj);
			for(int i = 0; i < json_object_array_length(aobj); i++){
				robj = json_object_array_get_idx(aobj, i);

				json_object_object_get_ex(robj, "id", &jbuf);
				((ROOM_DATA *)(p->ptr))->members[i].id = json_object_get_int(jbuf);
				free(jbuf);
				json_object_object_get_ex(robj, "nickname", &jbuf);
				strcpy(((ROOM_DATA *)(p->ptr))->members[i].nickname, json_object_get_string(jbuf));
				free(jbuf);
				free(robj);
			}
			
			for(int i = 0; i < json_object_array_length(aobj); i++)
				free(json_object_array_get_idx(aobj, i));
			free(aobj);

			printf("j2p::<from server>\n");
			printf("<member of room#%d>\n", ((ROOM_DATA *)(p->ptr))->room_id);
			for(int i = 0; i < MAX_USER; i++){
				printf("user#%d : %s\n", ((ROOM_DATA *)(p->ptr))->members[i].id, ((ROOM_DATA *)(p->ptr))->members[i].nickname);
			}
		}
		else if(minor_code == 2){
			printf("22방 인원 추가\n");
			
			json_object * uobj;
			p->ptr = (void *)((ADDED_MEMBER_DATA *)malloc(sizeof(ADDED_MEMBER_DATA)));

			json_object_object_get_ex(jobj, "room", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((ADDED_MEMBER_DATA *)(p->ptr))->room.id = json_object_get_int(jbuf);
			free(jbuf);
			json_object_object_get_ex(uobj, "num", &jbuf);
			((ADDED_MEMBER_DATA *)(p->ptr))->room.num = json_object_get_int(jbuf);
			free(jbuf);
			free(uobj);
			
			json_object_object_get_ex(jobj, "user", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((ADDED_MEMBER_DATA *)(p->ptr))->user.id = json_object_get_int(jbuf);
			free(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((ADDED_MEMBER_DATA *)(p->ptr))->user.nickname, json_object_get_string(jbuf));
			free(jbuf);
			free(uobj);

			///*
			printf("j2p::<add member>");   
			printf("room#%d, members:%d\nuser#%d:%s\n", ((ADDED_MEMBER_DATA *)(p->ptr))->room.id, ((ADDED_MEMBER_DATA *)(p->ptr))->room.num, ((ADDED_MEMBER_DATA *)(p->ptr))->user.id, ((ADDED_MEMBER_DATA *)(p->ptr))->user.nickname);
		}
		else if(minor_code == 3){
			printf("23게임 시작 알림\n");
			json_object * uobj;

			p->ptr = (void *)((NEW_ROUND_DATA *)malloc(sizeof(NEW_ROUND_DATA)));

			json_object_object_get_ex(jobj, "answerLen", &jbuf);
			((NEW_ROUND_DATA *)(p->ptr))->answerLen = json_object_get_int(jbuf);
			free(jbuf);

			json_object_object_get_ex(jobj, "painter", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((NEW_ROUND_DATA *)(p->ptr))->painter.id = json_object_get_int(jbuf);
			free(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((NEW_ROUND_DATA *)(p->ptr))->painter.nickname, json_object_get_string(jbuf));
			free(jbuf);
			free(uobj);
			
			printf("j2p::<new round data>\n");
			for(int i = 0; i < ((NEW_ROUND_DATA *)(p->ptr))->answerLen; i++)
				printf("_ ");
			printf("\npainter: %s(#%d)\n", ((NEW_ROUND_DATA *)(p->ptr))->painter.nickname, ((NEW_ROUND_DATA *)(p->ptr))->painter.id);
		}
		else if(minor_code == 4){
			printf("24정답 알림\n");
			
			p->ptr = (void *)((ANSWER_DATA *)malloc(sizeof(ANSWER_DATA)));
			json_object_object_get_ex(jobj, "answer", &jbuf);
			strcpy(((ANSWER_DATA *)(p->ptr))->answer, json_object_get_string(jbuf));
			
			printf("j2p::<answer data>\n");
			printf("answer: %s\n", ((ANSWER_DATA *)(p->ptr))->answer);
		}
		else if(minor_code == 5){
			printf("25방이 사라짐\n");
			
			p->ptr = (void *)((INVALID_ROOM_DATA *)malloc(sizeof(INVALID_ROOM_DATA)));
			json_object_object_get_ex(jobj, "room_id", &jbuf);
			((INVALID_ROOM_DATA *)(p->ptr))->room_id = json_object_get_int(jbuf);
			
			printf("j2p::<room diappeard>\n");
			printf("room_id: %d\n", ((INVALID_ROOM_DATA *)(p->ptr))->room_id);
		}
		else if(minor_code == 6){
			printf("26그리기 종료 알림\n");
			
			p->ptr = (void *)((END_ROUND_DATA *)malloc(sizeof(END_ROUND_DATA)));
			json_object_object_get_ex(jobj, "answer", &jbuf);
			strcpy(((END_ROUND_DATA *)(p->ptr))->answer, json_object_get_string(jbuf));
			
			printf("j2p::<end round>\n");
			printf("answer: %s\n", ((END_ROUND_DATA *)(p->ptr))->answer);
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
	p.minor_code = 6;
	p.ptr = (void *)((END_ROUND_DATA *)malloc(sizeof(END_ROUND_DATA)));
	strcpy(((END_ROUND_DATA *)(p.ptr))->answer, "몽구스");


	json_string = packet_to_json(p);
	free(p.ptr);
	printf("json_string : %s\n\n", json_string);//debug

	json_to_packet(json_string, &p);

	free((void *)json_string);
	free(p.ptr);
}*/
