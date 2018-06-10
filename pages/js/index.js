let status = 0; // 0 : not in gameroom , 1 : gameroom , 2 : game 
let UID = null; // user uid
let NICKNAME = null; // user nickname
let ROOM_ID = null; // room_id 
let SCORE = 0; // SCORE
let isPainter = true; // which now client is painter.


let ws;
let chat;


let nicknameInputBox;
let waiting;
let app;
let notice; 
let draw;

function setDevOption(){ uid = 9999; room_id =9999;}
window.onload= function(){
	chat = new Chatting();
	nicknameInputBox = new NickNameInput();
	
	waiting = new Waiting();
	app = new App();
	notice = new Notice();
	draw = new Draw();

	ws = new Websocket();
	statusManager = new StatusManager();

	setDevOption();
	layout();	
}

function layout(){
	let chat = document.getElementById("chat");
	let chatHistory = document.getElementsByClassName("chatHistory")[0];
	let inputBox = document.getElementsByClassName("inputBox")[0];

	chatHistory.style.height = ( chat.offsetHeight - inputBox.clientHeight - chat.offsetHeight / 100 * 5 ) + "px";
}

class StatusManager{
	constructor(){

	}

	userAdd(event){		
		if( (NICKNAME = document.getElementById("nickname").value) == "" ) NICKNAME = "Lorem Ipsum";

		let jsonObject = { // 14
			major_code : 1,
			minor_code : 4,
			nickname : NICKNAME
		}

		let json = JSON.stringify(jsonObject);
		ws.send(json);
	}
	userAddResponse(jsonObject){
		if( jsonObject.success ){
			if( UID == null ){
				SCORE = jsonObject.user.score;
			}
			else{ // score set
				let scoreJson = { // 17
					major_code : 1,
					minor_code : 7,
					score: SCORE
				}

				let msg = JSON.stringify(scoreJson);
				ws.send(msg);
			}

			UID = jsonObject.user.uid;
			NICKNAME = jsonObject.user.nickname;
			nicknameInputBox.hide();
			waiting.showList(null);
			scrollTo(0,0);
		}
		else{
			alert("닉네임 등록에 실패하셨습니다.");	
			if( UID != null ){
				window.location.reload();
			}
		} 
	}

	setScoreResponse(jsonObject){
		if( !jsonObject.success){
			alert("재연결 시 스코어 등록에 실패하셨습니다.");
			SCORE = 0;
		}
	}

	refreshRoomLish(event){
		event.preventDefault();
		
		// reset with  ws.send
	}
// waiting room
	enterRoom(event){
		event.preventDefault();
		let target = event.target;
		window.target = target;
		if( !target.hasAttribute("data-roomid") ) target = target.parentNode
		let roomid = target.getAttribute("data-roomid");

		waiting.hideList();
		waiting.showWaitingRoom(null);

		status = 1;
	}
	makeRoom(event){
		if( event != null ) event.preventDefault();
		waiting.hideList();
		waiting.showWaitingRoom()

		status = 1;
	}
	exitRoom(event){
		if( event != null ) event.preventDefault();
		waiting.hideWaitingRoom();
		waiting.showList();

		status = 0;
	}

// app
	enterGameRoom(event){
		if( event != null ) event.preventDefault();
		waiting.hideAll();
		app.showAll();

		status = 2;
	}
	exitGameRoom(event){
		if( event != null ) event.preventDefault();
		app.hideAll();
		waiting.showAll();
		waiting.showList(null);
		waiting.hideWaitingRoom();


		status = 0;
	}
}



class NickNameInput{
	constructor(){
		this.inputBox = document.getElementsByClassName("nicknameInputBox")[0];
		this.header = document.getElementsByClassName("waitingHeader")[0];
	}
	show(){
		this.inputBox.style.display = "inline-block";
		this.inputBox.style.opacity = "1";
		this.header.style.display = "inline-block";
	}
	hide(){
		this.inputBox.style.display = "none";
		this.inputBox.style.opacity = "0";
		this.header.style.display = "none";
	}

}


class Waiting{
	constructor(){
		this.waiting = document.getElementById("waiting");

		this.roomLists = document.getElementsByClassName("roomLists")[0];
		this.ul = document.getElementsByClassName("rooms")[0];
		this.li = document.getElementsByClassName("room");

		this.waitingRoom = document.getElementsByClassName("waitingRoom")[0];
		this.waitingMembers = document.getElementsByClassName("waitingMembers")[0];

		this.nowPage = 0;
	}
	hideAll(){
		this.waiting.style.display = "none";
		this.waiting.style.zIndex = 0;
	}
	showAll(){
		this.waiting.style.display = "inline-block";
		this.waiting.style.zIndex = 1000;
	}
	showList(jsonObject){
		this.roomLists.style.display = "inline-block";
		document.getElementsByClassName("waitingHeader")[0].style.display = "inline-block";
		this.roomListSet(jsonObject);
		this.roomNodeAddToList(0);
	}
	hideList(){
		this.roomLists.style.display = "none";
	}
	showWaitingRoom(jsonObject){
		this.waitingRoom.style.display = "inline-block";
		this.waitingMembers.innerHTML = null;
		this.memberListSet(jsonObject);
		this.userNodeAddToList();
	}
	hideWaitingRoom(){
		this.waitingRoom.style.display = "none";
	}

	memberListSet(jsonObject){
		jsonObject = {
			members: [
				{nickname : "someone1",uid : 1},
				{nickname : "someone2",uid : 2},
				{nickname : "someone3",uid : 3},
				{nickname : "someone4",uid : 4},
				{nickname : "someone5",uid : 5},
				{nickname : "someone6",uid : 6}
			]
		}
		this.members = jsonObject.members;
	}

	userNodeAddToList(){
		if( this.members == undefined || this.members == null ) return;
		this.waitingMembers.innerHTML = null;
		for( let i = 0 ; i < this.members.length;i++){
			let node = document.createElement("li");
			node.className = "members";
			node.setAttribute("data-uid", this.members[i].uid);

			let nickname = document.createElement("span");
			nickname.className="nickname";
			nickname.innerHTML = this.members[i].nickname;

			node.append(nickname);
			this.waitingMembers.append(node);
		}
	}

	next(e){
		if( this.nowPage < this.maxLength ) this.nowPage++;
		this.roomNodeAddToList(this.nowPage);
	}
	prev(e){
		if( this.nowPage > 0 ) this.nowPage--;
		this.roomNodeAddToList(this.nowPage);
	}

	roomListSet(jsonObject){
		jsonObject = {
			roomList : [
				{room_id : 1,roomNum : 3},
				{room_id : 2,roomNum : 3},
				{room_id : 3,roomNum : 3},
				{room_id : 4,roomNum : 3},
				{room_id : 5,roomNum : 3},
				{room_id : 6,roomNum : 3},
				{room_id : 7,roomNum : 3},
				{room_id : 8,roomNum : 3},
				{room_id : 9,roomNum : 3},
				{room_id : 10,roomNum : 3},
				{room_id : 11,roomNum : 3},
				{room_id : 12,roomNum : 3},
				{room_id : 13,roomNum : 3},
				{room_id : 14,roomNum : 3},
				{room_id : 15,roomNum : 3},
				{room_id : 16,roomNum : 3},
				{room_id : 17,roomNum : 3}
			]
		}

		this.room = jsonObject.roomList;
		this.maxLength = parseInt(this.room.length / 8) + (this.room.length % 8 ? 1 : 0) - 1;
	}

	roomNodeAddToList(page){
		if( this.room == undefined || this.room == null ) return;
		this.ul.innerHTML  = null;
		let cnt = 0;
		let since = page * 8;
		let until = this.room.length < since + 8 ? this.room.length : since + 8;

		
		for( let i = since; i < until; i++){
			let node = document.createElement("li");
			node.className = "room";
			node.addEventListener("click", statusManager.enterRoom,false);
			node.setAttribute("data-room_id", this.room[i].room_id);

			let roomID = document.createElement("span");
			roomID.innerHTML = "Room No." + this.room[i].room_id;
			roomID.className = "roomID";

			let roomNum = document.createElement("span");
			roomNum.innerHTML = this.room[i].roomNum + "/6"; // 6 is max number of member;
			roomNum.className = "roomNum";

			node.append(roomID);
			node.append(roomNum);
			this.ul.append(node);
			cnt++;
		}

		if( cnt % 2 ){
			let node = document.createElement("span");
			node.className = "empty";

			this.ul.append(node);
		}
	}
}




class App{
	constructor(){
		this.app = document.getElementById("app");
	}
	showAll(){
		this.app.style.display = "inline-block";
		this.app.style.zIndex = 1000;
		draw.clear();
		chat.clear();

	}
	hideAll(){
		this.app.style.display = "none";
		this.app.style.zIndex = 0;
	}

}

class Notice{
	constructor(){
		this.answer = document.getElementsByClassName("answer")[0];
		this.clock = document.getElementsByClassName("clock")[0];
	}
	showAnswerLen(jsonObject){
		this.answer.innerHTMl = null;
		for( let i = 0 ; i < jsonObject.answerLen; i++){
			this.answer.innerHTML += "_ ";
		}
	}
}

class Chatting{
	constructor(){
		this.chatHistory = document.getElementsByClassName("chatHistory")[0];
		this.chatInput = document.getElementById("chatInput");
		this.initEvent();
	}
	clear(){
		this.chatHistory.innerHTML = null;
		this.chatInput.value = null;
	}
	addMsg(jsonObject){
		this.chatHistory.innerHTML += "\
			<div class = 'msg'>\
				<span class = 'sender'>"
					+ jsonObject.from.nickname  + " | " + 
				"</span>\
				<span class = 'contents'>"
				 + jsonObject.msg + 
				"</span>\
			</div>";
		this.chatHistory.scrollTop = this.chatHistory.scrollHeight;
	}

	initEvent(){
		let chatInput = document.getElementById("chatInput");
		let sendBtn = document.getElementById("sendBtn");

		sendBtn.addEventListener("click", this.sendMessage);
		chatInput.addEventListener("keydown", this.keyPressHandle.bind(this));
	}
	sendMessage(event){
		event.preventDefault();
		let box = document.getElementById("chatInput");
		if( box.value == "" ) return;		
		let jsonObject = {
			major_code : 0,
			minor_code : 1,
			msg : box.value,
			from : {
				uid : UID,
				nickname : nickname
			},
			timestamp : new Date(),
			room_id : ROOM_ID
		}
		let msg = JSON.stringify(jsonObject);
		ws.send(msg);

		box.value = null;
	}	

	keyPressHandle(event){
		let keycode =  event.which? event.which : event.keyCode;
		if( keycode == 	13 ){
			if( !event.shiftKey){
				this.sendMessage(event);
			}
		}
	}
}

class Draw{
	constructor(){
		this.tag = document.getElementById("canvas");
		this.tag.width = this.tag.offsetWidth;
		this.tag.height = this.tag.offsetHeight;

		canvas.addEventListener("mousedown", this.mouseHandle.bind(this));
		canvas.addEventListener("mousemove", this.mouseHandle.bind(this));
		canvas.addEventListener("mouseup", this.mouseHandle.bind(this));
		this.canvas = this.tag.getContext("2d");
		
		this.brushType = true;
		this.color = "#000000";
		this.canvas.fillStyle = this.color;
        this.canvas.strokeStyle = this.color;

        this.px = 3;
        this.canvas.lineWidth = this.px;
        this.canvas.lineCap = "round";
        this.canvas.lineJoin = "round";

		this.prevX = this.prevY = 0;
		this.x = this.y = 0;

		this.initEvent();
		this.prepareToDraw();
	}
	clear(){
		this.canvas.fillStyle = this.canvas.strokeStyle = this.color = this.colors[1].getAttribute("data-color");

		for(let i = 0; i< this.colors.length; i++){
			this.colors[i].style.backgroundBlendMode = "normal";
			this.colors[i].className = "color";
		}
		this.colors[1].style.backgroundBlendMode = "darken";
		this.colors[1].className = "color on";
		this.setPencil();

  		this.canvas.lineWidth = this.px = this.brush[0].getAttribute("data-px");

  		for( let i = 0 ; i < this.brush.length; i++){
  			this.brush[i].style.background = "none";
  		}
  		this.brush[0].style.background = "#D2EAFF";

  		this.clearCanvas();
	}

	initEvent(){
		this.colors = document.getElementsByClassName("color");
		for( let i = 0 ; i < this.colors.length ; i++){
			this.colors[i].addEventListener("click", this.colorChange.bind(this));
			this.colors[i].style.background = this.colors[i].getAttribute("data-color");
		}

		
		this.brush = document.getElementsByClassName("brush");
		for( let i = 0 ; i < this.brush.length ; i++){
			this.brush[i].addEventListener("click", this.brushChange.bind(this));
			this.brush[i].children[0].style.width = this.brush[i].getAttribute("data-px") + "px";
			this.brush[i].children[0].style.height = this.brush[i].getAttribute("data-px") + "px";
			this.brush[i].children[0].style.borderRadius = this.brush[i].getAttribute("data-px")/2 + 2 + "px";
		}
		

		this.pencil = document.getElementsByClassName("pencil")[0];
		this.pencil.addEventListener("click", this.setPencil.bind(this));
		

		this.eraser = document.getElementsByClassName("eraser")[0];
		this.eraser.addEventListener("click", this.setEraser.bind(this));

		let clear = document.getElementsByClassName("clear")[0];
		clear.addEventListener("click", this.clearCanvas.bind(this));
	}

	prepareToDraw(){
		if( !isPainter ) return;
		this.colors[1].style.backgroundBlendMode="darken";
		this.colors[1].style.className = "color on";

		this.brush[0].style.background = "#D2EAFF";

		this.pencil.style.backgroundColor = "#ccc";
	}

	mouseHandle(event){
		event.preventDefault();
		if( !isPainter ) return;
		if( this.flag ){
			if( this.locked ) return;
			this.locked = true;
			setTimeout(() => this.locked = false, 20);
		}
		if( event.type == "mousedown"){
			this.flag = true;
			let position = this.getMousePos(event);
			this.prevX = this.x = position.x;
			this.prevY = this.y = position.y;
			this.draw();
		}
		else if( this.flag && event.type == "mousemove"){
			if( this.flag ){
				let position = this.getMousePos(event);
				this.prevX = this.x;
				this.prevY = this.y;
				this.x = position.x;
				this.y = position.y;
				this.draw();
			}
		}
		else if( event.type == "mouseup"){
			this.flag = false;
			let position = this.getMousePos(event);
			this.prevX = this.x;
			this.prevY = this.y;
			this.x = position.x;
			this.y = position.y;
			this.draw();
		}
		
	}

	getMousePos(evt) {
		let rect = this.tag.getBoundingClientRect();
		return {
			x: evt.offsetX,
			y: evt.offsetY
		};
  	}

  	draw(){
        this.canvas.beginPath();
        this.canvas.moveTo(this.prevX,this.prevY);
        this.canvas.lineTo(this.x, this.y);
        this.canvas.stroke();

        this.sendDrawingPoint();
  	}

  	drawWithJson(){
  		if( isPainter ) return;

  	}
  	sendDrawingPoint(){
  		if( !isPainter ) return;
  		let jsonObject = {
  			prevX : this.prevX,
  			prevY : this.prevY,
  			x : this.x,
  			y : this.y,
  			color : this.color,
  			px : this.px
  		}
  		let msg = JSON.stringify(jsonObject);
  		ws.send(msg);
  	}

  	colorChange(event){
  		if( !isPainter ) return;
  		let target = event.target;
  		if( !target.hasAttribute("data-color") ) target = target.parentNode;

  		let color = target.getAttribute("data-color");
		this.canvas.fillStyle = this.canvas.strokeStyle = this.color = color;

		for(let i = 0; i< this.colors.length; i++){
			this.colors[i].style.backgroundBlendMode = "normal";
			this.colors[i].className = "color";
		}
		target.style.backgroundBlendMode = "darken";
		target.className = "color on";
		this.setPencil();
  	}

  	brushChange(event){
  		if( !isPainter ) return;
  		event.preventDefault();
  		let target = event.target;
  		if( !target.hasAttribute("data-px") )target = target.parentNode;

  		this.px = target.getAttribute("data-px");
  		this.canvas.lineWidth = this.px;

  		for( let i = 0 ; i < this.brush.length; i++){
  			this.brush[i].style.background = "none";
  		}

  		target.style.background = "#D2EAFF";
  	}
  	setPencil(event){
  		if( !isPainter ) return;
  		this.brushType = true;
  		this.canvas.fillStyle = this.color;
  		this.canvas.strokeStyle = this.color;
		this.eraser.style.backgroundColor = "#fff";
		this.pencil.style.backgroundColor = "#ccc";
  	}
  	setEraser(event){
  		if( !isPainter ) return;
  		this.brushType = false;
		this.canvas.fillStyle = "#ffffff";
        this.canvas.strokeStyle = "#ffffff";
        this.pencil.style.backgroundColor = "#fff";
        this.eraser.style.backgroundColor = "#ccc";
  	}
  	clearCanvas(event){
  		this.canvas.clearRect(0, 0, this.tag.clientWidth, this.tag.clientHeight);
  	}
}



class Websocket{
	constructor(){
		this.ws = new WebSocket("ws://"+window.location.hostname+":8889");
		this.ws.onopen = function (event) {
			console.log("ws connected");
			if( UID != null ) statusManager.userAdd();
		};
		this.ws.onmessage = function (event){

			let json = event.data;
			console.log(json);

			let jsonObject = JSON.parse(json);

			if( jsonObject.major_code == 0 ){
				switch( jsonObject.minor_code ){
					case 0:
						draw.drawWithJson(jsonObject);
						return;
					case 1:
						chat.addMsg(jsonObject);
						return;
					case 2:
						return;
					case 3:
						return;
					default : console.log("undefined msg");return;
				}
			}
			else if( jsonObject.major_code == 2){
				switch( jsonObject.minor_code ){
					case 0 :
						return;	
					case 1 :
						return;
					case 2 :
						return;
					case 3 :
						notice.showAnswerLen(jsonObject);
						return;		

					default : console.log("undefined msg");return;
				}
			}
			else if( jsonObject.major_code == 3){
				switch(jsonObject.minor_code ){
					case 0 :
						statusManager.userAddResponse(jsonObject);
						return;
					case 1 : 
						statusManager.setScoreResponse(jsonObject);
						return;
					default : console.log("undefined msg "); return;
				}
			}

		}
		this.ws.onclose = function(event) {
			console.log("ws close");
		    setTimeout(() => {
		    	ws = new Websocket();
		    	if( status != 0 ){
		    		console.log("reconnect ws");
		    		if( status == 1 ) statusManager.exitRoom();
		    		else if( status == 2 ) statusManager.exitGameRoom();
		    	}
			}, 500);
		};
		this.ws.onerror = function(event){
			// this.ws.close();
			console.log("ws catched error");
		}
	}

	send(msg){
		this.ws.send(msg);
	}
}