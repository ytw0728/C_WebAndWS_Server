#include "Includes.h"
#include "message.h"

#include "json/json.h"
#include "json/json_tokener.h"

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

	if(major_code == 0){ //echo
		if(minor_code == 0){	
			printf("00그림 그리기\n");

			json_object_object_add(obj, "prevX", json_object_new_int(((DRAW_DATA *)(p.ptr))->prevX));
			json_object_object_add(obj, "prevY", json_object_new_int(((DRAW_DATA *)(p.ptr))->prevY));
			json_object_object_add(obj, "x", json_object_new_int(((DRAW_DATA *)(p.ptr))->x));
			json_object_object_add(obj, "y", json_object_new_int(((DRAW_DATA *)(p.ptr))->y));
			json_object_object_add(obj, "color", json_object_new_int(((DRAW_DATA *)(p.ptr))->color));
			json_object_object_add(obj, "px", json_object_new_int(((DRAW_DATA *)(p.ptr))->px));
		//	printf("p2j::json string: %s\n", json_object_to_json_string(obj));//debug
			
			ptr = json_object_to_json_string(obj);

			//strncpy(json_string, json_object_to_json_string(obj), strlen(json_object_to_json_string(obj)));
	//		printf("p2j::json string: %s\n", json_string);//debug
			return ptr;
		}
		else if(minor_code == 1){
			printf("01채팅 보내기\n");
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
	}
	else if(major_code == 1){ //simple request
		if(minor_code == 0){
			printf("10대기방 리스트 요청\n");
		}
		else if(minor_code == 1){
			printf("11대기방 접속 요청\n");
		}
		else if(minor_code == 2){
			printf("12게임 시작 요청\n");
		}

	}
	else if(major_code == 2){ //server response
	}
}

int json_to_packet(const char * json_string, struct packet * p)
{
	json_object * jobj, * jbuf;
	printf("j2p::json_string : %s\n", json_string);//debug
	jobj = json_tokener_parse(json_string); //read json

	///*
	//printf("json string : %s\n", json_object_to_json_string(jobj));//debug

	json_object_object_get_ex(jobj, "major_code", &jbuf);
	int major_code = json_object_get_int(jbuf);
	json_object_object_get_ex(jobj, "minor_code", &jbuf);
	int minor_code = json_object_get_int(jbuf);

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
			printf("j2p::<data>");   
			printf("(%d, %d)->(%d, %d)\n", ((DRAW_DATA *)(p->ptr))->prevX, ((DRAW_DATA *)(p->ptr))->prevY, ((DRAW_DATA *)(p->ptr))->x, ((DRAW_DATA *)(p->ptr))->y);
			printf("color : %d, px : %d\n", ((DRAW_DATA *)(p->ptr))->color, ((DRAW_DATA *)(p->ptr))->px); //debug */
			free(p->ptr);
		}
		else if(minor_code == 1){
			printf("01채팅 보내기\n");
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
	}
	else if(major_code == 1){ //simple request
		if(minor_code == 0){
			printf("10대기방 리스트 요청\n");
		}
		else if(minor_code == 1){
			printf("11대기방 접속 요청\n");
		}
		else if(minor_code == 2){
			printf("12게임 시작 요청\n");
		}


	}
	else if(major_code == 2){ //server response
	}
	return 0;
}

//----------------this is test-----------------
int main()
{
	const char *json_string;// = "{\"major_code\":0,\"minor_code\":0,\"key\":\"value\",\"id\":20162468}";
	struct packet p;
	p.major_code = 0;
	p.minor_code = 0;
	p.ptr = (void *)((DRAW_DATA *)malloc(sizeof(DRAW_DATA)));
	((DRAW_DATA *)(p.ptr))->prevX= 1;
	((DRAW_DATA *)(p.ptr))->prevY= 2;	
	((DRAW_DATA *)(p.ptr))->x= 3;
	((DRAW_DATA *)(p.ptr))->y= 4;	
	((DRAW_DATA *)(p.ptr))->color= 6;
	((DRAW_DATA *)(p.ptr))->px= 10;	

	json_string = packet_to_json(p);
	free(p.ptr);
	printf("json_string : %s\n", json_string);

	json_to_packet(json_string, &p);

	/*
	   enum json_type type = json_object_get_type(jobj);
	   switch (type) {
	   case json_type_null: 
	   printf("json_type_null\n");
	   break;
	   case json_type_boolean: 
	   printf("json_type_boolean\n");
	   break;
	   case json_type_double: 
	   printf("json_type_double\n");
	   break;
	   case json_type_int: 
	   printf("json_type_int\n");
	   break;
	   case json_type_object: 
	   printf("json_type_object\n");
	   break;
	   case json_type_array: 
	   printf("json_type_array\n");
	   break;
	   case json_type_string: 
	   printf("json_type_string\n");
	   break;
	   }//debug */
}
