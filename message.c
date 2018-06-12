#include "message.h"

/*
   todo
   헤더파일 맞게 수정하기
   데이터 유형별로 파싱하는 거 다 만들기(괄호가 완성한 코드)
   다 만든 json string utf8로 컨버트 - utf8 string으로 바꿔주는 함수 있지 않나?
 */


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//////////////////	server -> client //////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

const char * packet_to_json(struct packet p)
{
	const char *ptr;
	int major_code = p.major_code;
	int minor_code = p.minor_code;
	
	struct json_object * pobj = json_object_new_object();
	struct json_object * uobj , * aobj, * robj;

	uobj = aobj = robj = NULL;

	json_object_object_add(pobj, "major_code", json_object_new_int(major_code));
	json_object_object_add(pobj, "minor_code", json_object_new_int(minor_code));

	serverLog(WSSERVER, LOG, "[ data to send ] ", "");
	if(major_code == 0){ //echo
		if(minor_code == 0){ // 00
			serverLog(WSSERVER, LOG, "00그림 그리기\n", "");

			json_object_object_add(pobj, "success", json_object_new_int(((DRAW_DATA *)(p.ptr))->success));
			if( ((DRAW_DATA *)(p.ptr))->success ){
				json_object_object_add(pobj, "prevX", json_object_new_int(((DRAW_DATA *)(p.ptr))->prevX));
				json_object_object_add(pobj, "prevY", json_object_new_int(((DRAW_DATA *)(p.ptr))->prevY));
				json_object_object_add(pobj, "x", json_object_new_int(((DRAW_DATA *)(p.ptr))->x));
				json_object_object_add(pobj, "y", json_object_new_int(((DRAW_DATA *)(p.ptr))->y));
				json_object_object_add(pobj, "color", json_object_new_string(((DRAW_DATA *)(p.ptr))->color));
				json_object_object_add(pobj, "px", json_object_new_int(((DRAW_DATA *)(p.ptr))->px));
				json_object_object_add(pobj,"room_id", json_object_new_int(((DRAW_DATA *)(p.ptr))->room_id));
			}

			// json_object_object_add(pobj, "ptr", obj);
		}
		else if(minor_code == 1){ // 01
			serverLog(WSSERVER, LOG, "01채팅 보내기\n", "");
			json_object_object_add(pobj,"success", json_object_new_int(((CHAT_DATA *)(p.ptr))->success));
			

			if( ((CHAT_DATA *)(p.ptr))->success ){
				json_object_object_add(pobj, "msg", json_object_new_string(((CHAT_DATA *)(p.ptr))->msg));
				uobj = json_object_new_object();
					json_object_object_add(uobj, "uid", json_object_new_int(((CHAT_DATA *)(p.ptr))->from.uid));
					json_object_object_add(uobj, "nickname", json_object_new_string(((CHAT_DATA *)(p.ptr))->from.nickname));
				json_object_object_add(pobj, "from", uobj);
				json_object_object_add(pobj,"room_id", json_object_new_int(((CHAT_DATA *)(p.ptr))->room_id));
				json_object_object_add(pobj, "timestamp", json_object_new_string(((CHAT_DATA *)(p.ptr))->timestamp));
			}

			// json_object_object_add(pobj, "ptr", obj);

		}
		else if(minor_code == 2){ // 02
			serverLog(WSSERVER, LOG, "02게임 시작 요청(호스트)\n", "");
			json_object_object_add(pobj, "success", json_object_new_int( ((REQUEST_DRAWING_START*)(p.ptr))->success ) );
			if(((REQUEST_DRAWING_START*)(p.ptr))->success) {
				json_object_object_add(pobj, "room_id",
									   json_object_new_int(((REQUEST_DRAWING_START *) (p.ptr))->room_id));

				uobj = json_object_new_object();
				json_object_object_add(uobj, "uid", json_object_new_int(((REQUEST_DRAWING_START *) (p.ptr))->from.uid));
				json_object_object_add(uobj, "nickname",
									   json_object_new_string(((REQUEST_DRAWING_START *) (p.ptr))->from.nickname));
				json_object_object_add(pobj, "from", uobj);
			}


			// json_object_object_add( pobj, "ptr", obj);
			
		}
		else if(minor_code == 3){ // 03
			serverLog(WSSERVER, LOG, "03게임 종료 요청(그림 그리는 사람)\n", "");
			json_object_object_add(pobj, "success", json_object_new_int( ((REQUEST_DRAWING_END*)(p.ptr))->success ) );
			if(((REQUEST_DRAWING_END*)(p.ptr))->success) {
				json_object_object_add(pobj, "room_id",
									   json_object_new_int(((REQUEST_DRAWING_END *) (p.ptr))->room_id));

				uobj = json_object_new_object();
				json_object_object_add(uobj, "uid", json_object_new_int(((REQUEST_DRAWING_END *) (p.ptr))->from.uid));
				json_object_object_add(uobj, "nickname",
									   json_object_new_string(((REQUEST_DRAWING_END *) (p.ptr))->from.nickname));
				json_object_object_add(pobj, "from", uobj);
			}


			// json_object_object_add( pobj, "ptr", obj);
		}
		else if(minor_code == 4){ // 04
			serverLog(WSSERVER, LOG, "04제한 시간 정보(그림 그리는 사람)\n", "");
			json_object_object_add(pobj, "room_id", json_object_new_int( ((REQUEST_TIME_SHARE*)(p.ptr))->room_id ) );
			
			uobj = json_object_new_object();
				json_object_object_add( uobj, "uid", json_object_new_int( ((REQUEST_TIME_SHARE*)(p.ptr))->from.uid ));
				json_object_object_add( uobj, "nickname", json_object_new_string( ((REQUEST_TIME_SHARE*)(p.ptr))->from.nickname ));
			json_object_object_add(pobj, "from", uobj);
			json_object_object_add(pobj, "time", json_object_new_int( ((REQUEST_TIME_SHARE*)(p.ptr))->time ));

			// json_object_object_add( pobj, "ptr", obj);
			
		}
		else if(minor_code == 5){ // 05
			serverLog(WSSERVER, LOG, "05정답자 데이터(그림 그리는 사람)\n", "");


			uobj = json_object_new_object();
				json_object_object_add(uobj, "uid", json_object_new_int(((WINNER_DATA *)(p.ptr))->winner.uid));
				json_object_object_add(uobj, "nickname", json_object_new_string(((WINNER_DATA *)(p.ptr))->winner.nickname));
				json_object_object_add(uobj, "score", json_object_new_int(((WINNER_DATA *)(p.ptr))->winner.score));
			json_object_object_add(pobj, "winner", uobj);

			// json_object_object_add(pobj, "ptr", obj);
		}
	}
	/*
	else if(major_code == 1){ //simple request
		if(minor_code == 0){
			serverLog(WSSERVER, LOG, "10대기방 리스트 요청\n", "");

			json_object_object_add(pobj, "idx", json_object_new_int(((REQUEST_ROOM_LIST *)(p.ptr))->idx));
			json_object_object_add(pobj, "ptr", obj);

			uobj = json_object_new_object();
				json_object_object_add(uobj, "id", json_object_new_int(((REQUEST_ROOM_LIST *)(p.ptr))->from.uid));
				json_object_object_add(uobj, "nickname", json_object_new_string(((REQUEST_ROOM_LIST *)(p.ptr))->from.nickname));

			json_object_object_add(pobj, "from", obj);

		}
		else if(minor_code == 1){
			serverLog(WSSERVER, LOG, "11대기방 접속 요청\n", "");

			json_object_object_add(pobj, "room_id", json_object_new_int(((REQUEST_ENTER_ROOM *)(p.ptr))->room_id));
			// json_object_object_add(pobj, "ptr", obj);
		}
		else if(minor_code == 2){
			serverLog(WSSERVER, LOG, "12게임 시작 요청\n", "");

			json_object_object_add(pobj, "room_id", json_object_new_int(((REQUEST_START *)(p.ptr))->room_id));
			// json_object_object_add(pobj, "ptr", obj);
		}

	}
	*/
	
	else if(major_code == 2){ //server response	
		if(minor_code == 0){ // 20
			serverLog(WSSERVER, LOG, "20대기방 리스트 응답\n", "");

			
			aobj = json_object_new_array();
			int i = 0;
			for( i = 0 ; i < (int)((ROOM_LIST_DATA *)(p.ptr))->idx; i++){
				robj = json_object_new_object();
				json_object_object_add(robj, "id", json_object_new_int(((ROOM_LIST_DATA *)(p.ptr))->rlist[i].id));
				json_object_object_add(robj, "num", json_object_new_int(((ROOM_LIST_DATA *)(p.ptr))->rlist[i].num));
				json_object_object_add(robj, "status", json_object_new_int(((ROOM_LIST_DATA *)(p.ptr))->rlist[i].status));

				json_object_array_add(aobj, robj);
			}
			json_object_object_add(pobj, "rlist", aobj);
			json_object_object_add(pobj, "success", json_object_new_int(((ROOM_LIST_DATA *)(p.ptr))->success));

			// json_object_object_add(pobj, "ptr", obj);


		}
		else if(minor_code == 1){ // 21
			serverLog(WSSERVER, LOG, "21대기방 접속\n", "");
			json_object_object_add(pobj, "success", json_object_new_int(((ROOM_DATA *)(p.ptr))->success));
			if( ((ROOM_DATA *)(p.ptr))->success ){

				robj = json_object_new_object();
					json_object_object_add(robj, "id", json_object_new_int(((ROOM_DATA *)(p.ptr))->room.id));
					json_object_object_add(robj, "num", json_object_new_int(((ROOM_DATA *)(p.ptr))->room.num));
					json_object_object_add(robj, "status", json_object_new_int(((ROOM_DATA *)(p.ptr))->room.status));
				json_object_object_add(pobj, "room", robj);
				aobj = json_object_new_array();
				int i;
				for(i = 0; i < (int)((ROOM_DATA *)(p.ptr))->idx; i++){
					uobj = json_object_new_object();
						json_object_object_add(uobj, "uid", json_object_new_int(((ROOM_DATA *)(p.ptr))->members[i].uid));
						json_object_object_add(uobj, "nickname", json_object_new_string(((ROOM_DATA *)(p.ptr))->members[i].nickname));
						json_object_object_add(uobj, "score", json_object_new_int(((ROOM_DATA *)(p.ptr))->members[i].score));
					json_object_array_add(aobj, uobj);
				}
				json_object_object_add(pobj, "members", aobj);
				json_object_object_add(pobj, "leader_id", json_object_new_int(((ROOM_DATA *)(p.ptr))->leader_id));
			}
			

			// json_object_object_add(pobj, "ptr", obj);

		}
		else if(minor_code == 2){ // 22
			serverLog(WSSERVER, LOG, "22방 인원 추가\n", "");
			
				robj = json_object_new_object();

				json_object_object_add(robj, "id", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->room.id));
				json_object_object_add(robj, "num", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->room.num));
			json_object_object_add(pobj, "room", robj);
			
				uobj = json_object_new_object();
				json_object_object_add(uobj, "uid", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->user.uid));
				json_object_object_add(uobj, "nickname", json_object_new_string(((ADDED_MEMBER_DATA *)(p.ptr))->user.nickname));
				json_object_object_add(uobj, "score", json_object_new_int(((ADDED_MEMBER_DATA *)(p.ptr))->user.score));
			json_object_object_add(pobj, "user", uobj);

			// json_object_object_add(pobj, "ptr", obj);

		}
		else if(minor_code == 3){ // 23
			serverLog(WSSERVER, LOG, "23게임 시작 알림\n", "");
			
			json_object_object_add(pobj, "answerLen", json_object_new_int(((NEW_ROUND_DATA *)(p.ptr))->answerLen));

				uobj = json_object_new_object();
				json_object_object_add(uobj, "uid", json_object_new_int(((NEW_ROUND_DATA *)(p.ptr))->painter.uid));
				json_object_object_add(uobj, "nickname", json_object_new_string(((NEW_ROUND_DATA *)(p.ptr))->painter.nickname));
			json_object_object_add(pobj, "painter", uobj);
			// json_object_object_add(pobj, "ptr", obj);
		}
		else if(minor_code == 4){ // 24
			serverLog(WSSERVER, LOG, "24정답 알림\n", "");
			json_object_object_add(pobj, "success", json_object_new_int( ((ANSWER_DATA*)(p.ptr))->success));
			if(((ANSWER_DATA*)(p.ptr))->success ){ 
				json_object_object_add(pobj, "answer", json_object_new_string(((ANSWER_DATA *)(p.ptr))->answer));
			}
			// json_object_object_add(pobj, "ptr", obj);
		}
		else if(minor_code == 5){ //25 
			serverLog(WSSERVER, LOG, "25방이 사라짐\n", "");
			
			json_object_object_add(pobj, "room_id", json_object_new_int(((INVALID_ROOM_DATA *)(p.ptr))->room_id));
			// json_object_object_add(pobj, "ptr", obj);
		}
		else if(minor_code == 6){ // 26
			serverLog(WSSERVER, LOG, "26그리기 종료 알림\n", "");
		
			json_object_object_add(pobj, "answer", json_object_new_string(((END_ROUND_DATA *)(p.ptr))->answer));
			// json_object_object_add(pobj, "ptr", obj);
		}
		else if( minor_code == 7 ){ // 27 // 임시 폐쇄
			serverLog(WSSERVER, LOG, "27 게임방 나가기 응답\n","");
			json_object_object_add(pobj, "success", json_object_new_int(((RESPONSE_EXIT_GAMEROOM *)(p.ptr))->success));
			// json_object_object_add(pobj,"ptr",obj);
		}
		else if( minor_code == 8 ){ // 28
			serverLog(WSSERVER, LOG, "28 대기방, 게임방 나가기 응답\n","");
			json_object_object_add(pobj, "success", json_object_new_int(((RESPONSE_EXIT *)(p.ptr))->success));
			// json_object_object_add(pobj, "ptr", obj);
		}

	}
	else if( major_code == 3){
		if( minor_code == 0 ){ // 30
			serverLog(WSSERVER, LOG, "30 닉네임 등록 응답\n","");
			json_object_object_add(pobj, "success", json_object_new_int( ((RESPONSE_REGISTER*)(p.ptr))->success ));
			if( ((RESPONSE_REGISTER*)(p.ptr))->success ){
					uobj = json_object_new_object();
					json_object_object_add(uobj, "uid", json_object_new_int(((RESPONSE_REGISTER*)(p.ptr))->user.uid));
					json_object_object_add(uobj, "nickname", json_object_new_string(((RESPONSE_REGISTER*)(p.ptr))->user.nickname));
					json_object_object_add(uobj, "score", json_object_new_int(((RESPONSE_REGISTER*)(p.ptr))->user.score));
				json_object_object_add(pobj, "user", uobj);
			}

			// json_object_object_add(pobj, "ptr", obj);

		}
		else if( minor_code == 1 ){ // 31
			serverLog(WSSERVER, LOG, "31 스코어 등록 응답\n","");
			json_object_object_add(pobj, "success", json_object_new_int( ((RESPONSE_SET_SCORE*)(p.ptr))->success ));
		}
		else if( minor_code == 2 ){ // 32
			serverLog(WSSERVER, LOG, "32 방 인원 감소 알림\n","");
			robj = json_object_new_object();
				json_object_object_add(robj,"id", json_object_new_int(((POP_MEMBER_DATA*)(p.ptr))->room.id)  );
				json_object_object_add(robj,"num", json_object_new_int(((POP_MEMBER_DATA*)(p.ptr))->room.num)  );
			json_object_object_add(pobj,"room", robj);
			uobj = json_object_new_object();
				json_object_object_add(uobj,"uid", json_object_new_int(((POP_MEMBER_DATA*)(p.ptr))->user.uid) );
				json_object_object_add(uobj,"nickname", json_object_new_string(((POP_MEMBER_DATA*)(p.ptr))->user.nickname) );
			json_object_object_add(pobj, "user", uobj);
		}
		else if( minor_code == 3){ // 33 
			serverLog(WSSERVER, LOG, "33 방 상태 변경\n","");
			robj = json_object_new_object();
				json_object_object_add(robj, "id", json_object_new_int( ((STATUS_CHANGED*)(p.ptr))->room.id));
				json_object_object_add(robj, "num", json_object_new_int( ((STATUS_CHANGED*)(p.ptr))->room.num));
				json_object_object_add(robj, "status", json_object_new_int( ((STATUS_CHANGED*)(p.ptr))->room.status));
			json_object_object_add(pobj, "room", robj);
			uobj = json_object_new_object();
				json_object_object_add(uobj,"uid", json_object_new_int(((STATUS_CHANGED*)(p.ptr))->leader.uid) );
				json_object_object_add(uobj,"nickname", json_object_new_string(((STATUS_CHANGED*)(p.ptr))->leader.nickname) );
			json_object_object_add(pobj, "leader", uobj);
		}
	}
	

	ptr = json_object_to_json_string(pobj);
	
	if( aobj ){
		int i;
		for(i = 0; i < json_object_array_length(aobj); i++)
				json_object_put(json_object_array_get_idx(aobj, i));
		json_object_put(aobj);
	}	
	if( uobj ) json_object_put(uobj);
	if( robj ) json_object_put(robj);

	if( pobj ){
		// printf(" \n\n\n\n\n pobj is not null \n\n\n\n");
		// json_object_put(pobj);
	}
	

	aobj = pobj = uobj= robj = NULL;

	return ptr;
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
//////////////////	client -> server //////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

int json_to_packet(const char * json_string, struct packet * p)
{
	// json_object * jobj, * jbuf, *obj;
	struct json_object * jbuf, *obj, *uobj;
	uobj = jbuf = obj = NULL;

	obj = json_tokener_parse(json_string); //read json


	///*
	//serverLog(WSSERVER, LOG, "json string : %s\n", json_object_to_json_string(jobj));//deb, ""ug

	json_object_object_get_ex(obj, "major_code", &jbuf);
	int major_code = json_object_get_int(jbuf);
	json_object_object_get_ex(obj, "minor_code", &jbuf);
	int minor_code = json_object_get_int(jbuf);

	// json_object_object_get_ex(obj, "ptr", &jobj);

	p->major_code = major_code;
	p->minor_code = minor_code;

	serverLog(WSSERVER, LOG, "[ data to receive ] ", "");

	if(major_code == 0){ //echo
		if(minor_code == 0){	// 00
			serverLog(WSSERVER, LOG, "00그림 그리기\n", "");
			serverLog(WSSERVER, LOG, json_string , "sended");//debug
			p->ptr = (void *)((DRAW_DATA *)malloc(sizeof(DRAW_DATA)));

			json_object_object_get_ex(obj, "prevX", &jbuf);
			((DRAW_DATA *)(p->ptr))->prevX = json_object_get_int(jbuf);
			json_object_object_get_ex(obj, "prevY", &jbuf);
			((DRAW_DATA *)(p->ptr))->prevY = json_object_get_int(jbuf);
			json_object_object_get_ex(obj, "x", &jbuf);
			((DRAW_DATA *)(p->ptr))->x = json_object_get_int(jbuf);
			json_object_object_get_ex(obj, "y", &jbuf);
			((DRAW_DATA *)(p->ptr))->y = json_object_get_int(jbuf);
			json_object_object_get_ex(obj, "color", &jbuf);
			strcpy( ((DRAW_DATA *)(p->ptr))->color, json_object_get_string(jbuf));
			json_object_object_get_ex(obj, "px", &jbuf);
			((DRAW_DATA *)(p->ptr))->px = json_object_get_int(jbuf);
			((DRAW_DATA *)(p->ptr))->success = 0;

		}
		else if(minor_code == 1){  // 01
			serverLog(WSSERVER, LOG, "01채팅 보내기\n", "");
			p->ptr = (void *)((CHAT_DATA *)malloc(sizeof(CHAT_DATA)));


			json_object_object_get_ex(obj, "msg", &jbuf);
			strcpy(((CHAT_DATA *)(p->ptr))->msg, json_object_get_string(jbuf));
			
			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((CHAT_DATA *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((CHAT_DATA *)(p->ptr))->from.nickname, json_object_get_string(jbuf));

			json_object_object_get_ex(obj, "timestamp", &jbuf);
			strcpy(((CHAT_DATA *)(p->ptr))->timestamp, json_object_get_string(jbuf));

			json_object_object_get_ex(obj, "room_id", &jbuf);
			((CHAT_DATA *)(p->ptr))->room_id = json_object_get_int(jbuf);
			((CHAT_DATA *)(p->ptr))->success = 0;
		
		}
		else if(minor_code == 2){ // 02
			serverLog(WSSERVER, LOG, "02게임 시작 요청(호스트)\n", "");		
			p->ptr = (void *)((REQUEST_DRAWING_START *)malloc(sizeof(REQUEST_DRAWING_START)));
			json_object_object_get_ex(obj, "room_id", &jbuf);
			((REQUEST_DRAWING_START*)(p->ptr))->room_id = json_object_get_int(jbuf);


			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_DRAWING_START*)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_DRAWING_START*)(p->ptr))->from.nickname, json_object_get_string(jbuf));

		}
		else if(minor_code == 3){ // 03
			serverLog(WSSERVER, LOG, "03게임 종료 요청(그림 그리는 사람)\n", "");

			p->ptr = (void *)((REQUEST_DRAWING_END *)malloc(sizeof(REQUEST_DRAWING_END)));
			json_object_object_get_ex(obj, "room_id", &jbuf);
			((REQUEST_DRAWING_END*)(p->ptr))->room_id = json_object_get_int(jbuf);


			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_DRAWING_END*)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_DRAWING_END*)(p->ptr))->from.nickname, json_object_get_string(jbuf));
		}
		/*
		else if(minor_code == 4){
			serverLog(WSSERVER, LOG, "04제한 시간 정보(그림 그리는 사람)\n", "");


			p->ptr = (void *)((REQUEST_TIME_SHARE *)malloc(sizeof(REQUEST_TIME_SHARE)));
			json_object_object_get_ex(obj, "room_id", &jbuf);
			((REQUEST_TIME_SHARE*)(p->ptr))->room_id = json_object_get_int(jbuf);


			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_TIME_SHARE*)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_TIME_SHARE*)(p->ptr))->from.nickname, json_object_get_string(jbuf));

			json_object_object_get_ex(obj, "time", &jbuf);
			((REQUEST_TIME_SHARE*)(p->ptr))->time = json_object_get_int(jbuf);
		}
		else if(minor_code == 5){
			serverLog(WSSERVER, LOG, "05정답자 데이터(그림 그리는 사람)\n", "");


			p->ptr = (void *)((CHAT_DATA *)malloc(sizeof(CHAT_DATA)));

			json_object_object_get_ex(obj, "winner", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((WINNER_DATA *)(p->ptr))->winner.uid = json_object_get_int(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((WINNER_DATA *)(p->ptr))->winner.nickname, json_object_get_string(jbuf));

			//
			serverLog(WSSERVER, LOG, "j2p::<winner data>"); , ""  
			serverLog(WSSERVER, LOG, "WINNER!: (#%d)%s\n", ((WINNER_DATA *)(p->ptr))->winner.uid, ((WINNER_DATA *)(p->ptr))->winner.nickname, "");

			free(uobj);
		}
		*/
	}
	else if(major_code == 1){ //simple request
		if(minor_code == 0){
			serverLog(WSSERVER, LOG, "10대기방 리스트 요청\n", "");
			p->ptr = (void *)((REQUEST_ROOM_LIST *)malloc(sizeof(REQUEST_ROOM_LIST)));

			// from

			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_ROOM_LIST *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_ROOM_LIST *)(p->ptr))->from.nickname, json_object_get_string(jbuf));

		}
		else if(minor_code == 1){ // 11
			serverLog(WSSERVER, LOG, "11대기방 접속 요청\n", ""); 
			
			p->ptr = (void *)((REQUEST_ENTER_ROOM *)malloc(sizeof(REQUEST_ENTER_ROOM)));
			// from 

			json_object_object_get_ex(obj,"from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_ENTER_ROOM *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_ENTER_ROOM *)(p->ptr))->from.nickname, json_object_get_string(jbuf));
				json_object_object_get_ex(uobj, "score", &jbuf);
				((REQUEST_ENTER_ROOM *)(p->ptr))->from.score = json_object_get_int(jbuf);

			// room_id
			json_object_object_get_ex(obj, "room_id", &jbuf);
			((REQUEST_ENTER_ROOM *)(p->ptr))->room_id = json_object_get_int(jbuf);

			
		}
		else if(minor_code == 2){// 12
			serverLog(WSSERVER, LOG, "12게임 시작 요청\n", "");
			p->ptr = (void *)((REQUEST_START *)malloc(sizeof(REQUEST_START)));

			json_object_object_get_ex(obj, "room_id", &jbuf);
			((REQUEST_START *)(p->ptr))->room_id = json_object_get_int(jbuf);

			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_START *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_START *)(p->ptr))->from.nickname, json_object_get_string(jbuf));

		}
		else if(minor_code == 3 ){ // 13 // 임시 폐쇄
			serverLog(WSSERVER, LOG, "13 게임방 나가기\n", "");
			p->ptr = (void *)((REQUEST_EXIT_GAMEROOM *)malloc(sizeof(REQUEST_EXIT_GAMEROOM)));
			
			json_object_object_get_ex(obj, "room_id", &jbuf);
			((REQUEST_EXIT_GAMEROOM *)(p->ptr))->room_id = json_object_get_int(jbuf);


			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_EXIT_GAMEROOM *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_EXIT_GAMEROOM *)(p->ptr))->from.nickname, json_object_get_string(jbuf));

		}
		else if(minor_code == 4 ){ // 14
			serverLog(WSSERVER, LOG, "14 닉네임 등록\n", "");
			p->ptr = (void *)((REQUEST_NICKNAME_REGISTER *)malloc(sizeof(REQUEST_NICKNAME_REGISTER)));

			json_object_object_get_ex(obj, "nickname", &jbuf);
			strcpy(((REQUEST_NICKNAME_REGISTER *)(p->ptr))->nickname, json_object_get_string(jbuf));
		}
		else if( minor_code == 5){ // 15
			serverLog(WSSERVER, LOG, "15 대기방 생성\n", "");
			p->ptr = (void *)((MAKE_ROOM *)malloc(sizeof(MAKE_ROOM)));
			json_object_object_get_ex(obj, "from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((MAKE_ROOM *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((MAKE_ROOM *)(p->ptr))->from.nickname, json_object_get_string(jbuf));
		}
		else if( minor_code == 6){ // 16
			serverLog(WSSERVER, LOG, "16 대기방 나가기\n", "");
			p->ptr = (void *)((REQUEST_EXIT_ROOM *)malloc(sizeof(REQUEST_EXIT_ROOM)));

			json_object_object_get_ex(obj,"room_id", &jbuf);
			((REQUEST_EXIT_ROOM *)(p->ptr))->room_id = json_object_get_int(jbuf);


			json_object_object_get_ex(obj,"from", &uobj);
				json_object_object_get_ex(uobj, "uid", &jbuf);
				((REQUEST_EXIT_ROOM *)(p->ptr))->from.uid = json_object_get_int(jbuf);
				json_object_object_get_ex(uobj, "nickname", &jbuf);
				strcpy(((REQUEST_EXIT_ROOM *)(p->ptr))->from.nickname, json_object_get_string(jbuf));
		}

		else if( minor_code == 7 ){ // 17
			serverLog(WSSERVER, LOG, "17 스코어 등록\n", "");
			p->ptr = (void*)((REQUEST_SET_SCORE*)malloc(sizeof(REQUEST_SET_SCORE)));

			json_object_object_get_ex(obj, "score", &jbuf);
			((REQUEST_SET_SCORE*)(p->ptr))->score = json_object_get_int(jbuf);

		}

	}
	/*
	else if(major_code == 2){ //server response
		if(minor_code == 0){
			json_object * aobj, * robj;

			serverLog(WSSERVER, LOG, "20대기방 리스트 응답\n", "");
			
			p->ptr = (void *)((ROOM_LIST_DATA *)malloc(sizeof(ROOM_LIST_DATA)));

			json_object_object_get_ex(obj, "rlist", &aobj);
			for(i = 0; i < json_object_array_length(aobj); i++){
				robj = json_object_array_get_idx(aobj, i);

				json_object_object_get_ex(robj, "id", &jbuf);
				((ROOM_LIST_DATA *)(p->ptr))->rlist[i].uid = json_object_get_int(jbuf);
				free(jbuf);
				json_object_object_get_ex(robj, "num", &jbuf);
				((ROOM_LIST_DATA *)(p->ptr))->rlist[i].num = json_object_get_int(jbuf);
				free(jbuf);
				free(robj);
			}
			
			for(i = 0; i < json_object_array_length(aobj); i++)
				free(json_object_array_get_idx(aobj, i));
			free(aobj);

			serverLog(WSSERVER, LOG, "j2p::<from server : roomdata>\n", "");
			for(i = 0; i < 20; i++){
				serverLog(WSSERVER, LOG, "room#%d, users : %d\n", ((ROOM_LIST_DATA *)(p->ptr))->rlist[i].uid, ((ROOM_LIST_DATA *)(p->ptr))->rlist[i].num, "");
			}
		}
		else if(minor_code == 1){
			json_object * aobj, * robj;
			
			serverLog(WSSERVER, LOG, "21대기방 접속\n", "");
			p->ptr = (void *)((ROOM_DATA *)malloc(sizeof(ROOM_DATA)));

			json_object_object_get_ex(obj, "room_id", &jbuf);
			((ROOM_DATA *)(p->ptr))->room_id = json_object_get_int(jbuf);
			free(jbuf);

			json_object_object_get_ex(obj, "members", &aobj);
			for(i = 0; i < json_object_array_length(aobj); i++){
				robj = json_object_array_get_idx(aobj, i);

				json_object_object_get_ex(robj, "id", &jbuf);
				((ROOM_DATA *)(p->ptr))->members[i].uid = json_object_get_int(jbuf);
				free(jbuf);
				json_object_object_get_ex(robj, "nickname", &jbuf);
				strcpy(((ROOM_DATA *)(p->ptr))->members[i].nickname, json_object_get_string(jbuf));
				free(jbuf);
				free(robj);
			}
			
			for(i = 0; i < json_object_array_length(aobj); i++)
				free(json_object_array_get_idx(aobj, i));
			free(aobj);

			serverLog(WSSERVER, LOG, "j2p::<from server>\n", "");
			serverLog(WSSERVER, LOG, "<member of room#%d>\n", ((ROOM_DATA *)(p->ptr))->room_id, "");
			for(i = 0; i < MAX_USER; i++){
				serverLog(WSSERVER, LOG, "user#%d : %s\n", ((ROOM_DATA *)(p->ptr))->members[i].uid, ((ROOM_DATA *)(p->ptr))->members[i].nickname, "");
			}
		}
		else if(minor_code == 2){
			serverLog(WSSERVER, LOG, "22방 인원 추가\n", "");
			

			p->ptr = (void *)((ADDED_MEMBER_DATA *)malloc(sizeof(ADDED_MEMBER_DATA)));

			json_object_object_get_ex(obj, "room", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((ADDED_MEMBER_DATA *)(p->ptr))->room.uid = json_object_get_int(jbuf);
			free(jbuf);
			json_object_object_get_ex(uobj, "num", &jbuf);
			((ADDED_MEMBER_DATA *)(p->ptr))->room.num = json_object_get_int(jbuf);
			free(jbuf);
			free(uobj);
			
			json_object_object_get_ex(obj, "user", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((ADDED_MEMBER_DATA *)(p->ptr))->user.uid = json_object_get_int(jbuf);
			free(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((ADDED_MEMBER_DATA *)(p->ptr))->user.nickname, json_object_get_string(jbuf));
			free(jbuf);
			free(uobj);

			///*
			serverLog(WSSERVER, LOG, "j2p::<add member>"); , ""  
			serverLog(WSSERVER, LOG, "room#%d, members:%d\nuser#%d:%s\n", ((ADDED_MEMBER_DATA *)(p->ptr))->room.uid, ((ADDED_MEMBER_DATA *)(p->ptr))->room.num, ((ADDED_MEMBER_DATA *)(p->ptr))->user.uid, ((ADDED_MEMBER_DATA *)(p->ptr))->user.nickname, "");
		}
		else if(minor_code == 3){
			serverLog(WSSERVER, LOG, "23게임 시작 알림\n", "");


			p->ptr = (void *)((NEW_ROUND_DATA *)malloc(sizeof(NEW_ROUND_DATA)));

			json_object_object_get_ex(obj, "answerLen", &jbuf);
			((NEW_ROUND_DATA *)(p->ptr))->answerLen = json_object_get_int(jbuf);
			free(jbuf);

			json_object_object_get_ex(obj, "painter", &uobj);
			json_object_object_get_ex(uobj, "id", &jbuf);
			((NEW_ROUND_DATA *)(p->ptr))->painter.uid = json_object_get_int(jbuf);
			free(jbuf);
			json_object_object_get_ex(uobj, "nickname", &jbuf);
			strcpy(((NEW_ROUND_DATA *)(p->ptr))->painter.nickname, json_object_get_string(jbuf));
			free(jbuf);
			free(uobj);
			
			serverLog(WSSERVER, LOG, "j2p::<new round data>\n", "");
			for(i = 0; i < ((NEW_ROUND_DATA *)(p->ptr))->answerLen; i++)
				serverLog(WSSERVER, LOG, "_ ", "");
			serverLog(WSSERVER, LOG, "\npainter: %s(#%d)\n", ((NEW_ROUND_DATA *)(p->ptr))->painter.nickname, ((NEW_ROUND_DATA *)(p->ptr))->painter.uid, "");
		}
		else if(minor_code == 4){
			serverLog(WSSERVER, LOG, "24정답 알림\n", "");
			
			p->ptr = (void *)((ANSWER_DATA *)malloc(sizeof(ANSWER_DATA)));
			json_object_object_get_ex(obj, "answer", &jbuf);
			strcpy(((ANSWER_DATA *)(p->ptr))->answer, json_object_get_string(jbuf));
			
			serverLog(WSSERVER, LOG, "j2p::<answer data>\n", "");
			serverLog(WSSERVER, LOG, "answer: %s\n", ((ANSWER_DATA *)(p->ptr))->answer, "");
		}
		else if(minor_code == 5){
			serverLog(WSSERVER, LOG, "25방이 사라짐\n", "");
			
			p->ptr = (void *)((INVALID_ROOM_DATA *)malloc(sizeof(INVALID_ROOM_DATA)));
			json_object_object_get_ex(obj, "room_id", &jbuf);
			((INVALID_ROOM_DATA *)(p->ptr))->room_id = json_object_get_int(jbuf);
			
			serverLog(WSSERVER, LOG, "j2p::<room diappeard>\n", "");
			serverLog(WSSERVER, LOG, "room_id: %d\n", ((INVALID_ROOM_DATA *)(p->ptr))->room_id, "");
		}
		else if(minor_code == 6){
			serverLog(WSSERVER, LOG, "26그리기 종료 알림\n", "");
			
			p->ptr = (void *)((END_ROUND_DATA *)malloc(sizeof(END_ROUND_DATA)));
			json_object_object_get_ex(obj, "answer", &jbuf);
			strcpy(((END_ROUND_DATA *)(p->ptr))->answer, json_object_get_string(jbuf));
			
			serverLog(WSSERVER, LOG, "j2p::<end round>\n", "");
			serverLog(WSSERVER, LOG, "answer: %s\n", ((END_ROUND_DATA *)(p->ptr))->answer, "");
		}
	}*/

	
	
	
	if( uobj ){
		json_object_put(uobj);
		// free(uobj);
	} 
	if( jbuf ){
		json_object_put(jbuf);
		// free(jbuf);	
	} 
	if( obj ){
		json_object_put(obj);
		// free(obj);	
	}
	
	uobj = jbuf = obj = NULL;
	// free(jobj);
	
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
	serverLog(WSSERVER, LOG, "json_string : %s\n\n", json_string);//deb, ""ug

	json_to_packet(json_string, &p);

	free((void *)json_string);
	free(p.ptr);
}*/
