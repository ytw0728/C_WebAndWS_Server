let status = 0; // 0 : not in gameroom , 1 : gameroom , 2 : game 
let UID = null; // user uid
let NICKNAME = null; // user nickname
let ROOM_ID = null; // room_id 
let SCORE = 0; // SCORE
let isPainter = true; // which now client is painter.
let painterUID = null;

let leaderUID = null;

let ANSWER = null;
let SOMEONEGOTIT = false; // 누군가 정답을 맞췄다.


let ws;
let chat;

let timelimit = 120000; //60초

let nicknameInputBox;
let waiting;
let app;
let notice; 
let draw;

function setDevOption(){ UID = 9999; ROOM_ID =9999;}
window.onload= function(){
	chat = new Chatting();
	nicknameInputBox = new NickNameInput();

	waiting = new Waiting();
	app = new App();
	notice = new Notice();
	draw = new Draw();

	ws = new Websocket();
	statusManager = new StatusManager();

	// setDevOption();
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
		this.startGameRequest = false;
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
			waiting.showList();
			scrollTo(0,0);
		}
		else{
			alert("닉네임 등록에 실패했습니다.");	
			if( UID != null ){
				window.location.reload();
			}
		} 
	}
	setScoreResponse(jsonObject){
		if( !jsonObject.success){
			alert("재연결 시 스코어 등록에 실패했습니다.");
			SCORE = 0;
		}
	}
	refreshRoomList(event){
		event.preventDefault();
		waiting.showList();
	}
	// waiting room
	userPopHandle(jsonObject){
		waiting.popMember(jsonObject);
		if( jsonObject.room.num == 1 && status > 1){
			app.switchToWaiting();
		}
	}
	enterRoom(event){
		event.preventDefault();
		let target = event.target;
		window.target = target;
		if( !target.hasAttribute("data-room_id") ) target = target.parentNode
			let roomid = target.getAttribute("data-room_id");		

		let json = { // 11
major_code : 1,
	     minor_code : 1,
	     from : {
uid: UID,
     nickname : NICKNAME,
     score : SCORE
	     },
room_id : roomid
		}

		let msg = JSON.stringify(json);
		ws.send(msg);
	}
	enterRoomResponse(jsonObject){
		if( jsonObject.success){
			ROOM_ID = jsonObject.room.id;
			leaderUID = jsonObject.leader_id;
			waiting.hideList();
			waiting.showWaitingRoom(jsonObject);
			status = 1;
		}
		else{
			alert("접속에 실패했습니다. 새로고침 해주세요.");
			leaderUID = null;
		}
	}
	makeRoom(event){
		if( event != null ) event.preventDefault();

		let json = { // 15
major_code : 1,
	     minor_code : 5,
	     from : {
uid : UID,
      nickname : NICKNAME
	     }
		}
		let msg = JSON.stringify(json);

		leaderUID = UID;
		ws.send(msg);
	}
	exitRoom(event){
		if( event != null ) event.preventDefault();

		let json = { // 16
major_code : 1,
	     minor_code : 6,
	     room_id : ROOM_ID,
	     from : {
uid : UID,
      nickname : NICKNAME
	     }
		}

		let msg = JSON.stringify(json);
		ws.send(msg);
	}
	exitRoomResponse(jsonObject){
		if(jsonObject.success){
			if( status == 1 ) {
				waiting.showList();
				waiting.hideWaitingRoom();
				ROOM_ID = null;
				status = 0;
			}
			else if( status > 1 ){
				app.hideAll();
				waiting.showAll();
				waiting.showList();
				waiting.hideWaitingRoom();
				ROOM_ID = null;

				status = 0;
			}
		}
		else{
			alert("나가기에 실패했습니다.");
		}
	}

	// app
	enterGameRoom(event){
		if( event != null ) event.preventDefault();
		if( leaderUID != UID ){
			alert("방장만 게임을 시작할 수 있습니다.");
			return;
		}
		if( waiting.members.length < 2 ){
			alert("2명 이상일 때 게임을 시작할 수 있습니다.");
			return;
		}
		let json = { // 12
major_code : 1,
	     minor_code : 2,
	     room_id : ROOM_ID,
	     from : {
uid : UID,
      nickname : NICKNAME
	     }
		}

		statusManager.startGameRequest = true;
		let msg = JSON.stringify(json);

		ws.send(msg);
	}
	enterGameRoomResponse(jsonObject){
		painterUID = jsonObject.painter.uid;

		if( jsonObject.painter.uid == UID ) isPainter = true;
		else isPainter = false;

		notice.showAnswerLen(jsonObject);
		waiting.hideAll();
		app.showAll();

		SOMEONEGOTIT = false;
		chat.addSystemMsg(jsonObject);
		status = 2;
	}

	setAnswer(jsonObject){
		if( jsonObject.success ){
			notice.showAnswer(jsonObject);
			app.startGame(jsonObject);
		}
		else{
			alert("게임 시작에 실패했습니다. 다시 시도해주세요.");
			painterUID = null;
			isPainter = null;
		}
	}

	exitGameRoom(event){
		if( event != null ) event.preventDefault();
		app.gameDone();
		let json = { // 16
major_code : 1,
	     minor_code : 6,
	     room_id : ROOM_ID,
	     from : {
uid : UID,
      nickname : NICKNAME
	     }
		}
		let msg = JSON.stringify(json);
		ws.send(msg);
	}


	resetRoomStatus(jsonObject){
		leaderUID = jsonObject.leader.uid;
		waiting.userNodeAddToList();

		if( status >= 2){
			switch(jsonObject.room.status){
				case 0 :

					break;
				case 1 :
					app.switchToWaiting();
					break;
				case 2 :
					app.switchToWaiting();
					app.gameDone();
					break;
				case 3 :

					break;
			}	
		}
	}


	answerShow(jsonObject){
		if (jsonObject.winner.uid ==UID ) SCORE = jsonObject.winner.score;
		SOMEONEGOTIT = true;
		app.stopTimer();
		chat.addSystemMsg(jsonObject);
		waiting.changeUserValue(jsonObject.winner);
	}

	timeOut(jsonObject){
		chat.addSystemMsg(jsonObject);
		app.gameDone(jsonObject);
	}
	timeSync(jsonObject){
		app.syncTime(jsonObject);
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
		this.members = new Array();
		this.waiting = document.getElementById("waiting");

		this.roomTitle = document.getElementById("roomnumber");

		this.roomLists = document.getElementsByClassName("roomLists")[0];
		this.ul = document.getElementsByClassName("rooms")[0];
		this.li = document.getElementsByClassName("room");

		this.waitingRoom = document.getElementsByClassName("waitingRoom")[0];
		this.waitingMembers = document.getElementsByClassName("waitingMembers")[0];


		this.gameMembers = document.getElementsByClassName("users")[0];

		this.nowPage = 0;
	}
	hideAll(){
		this.waiting.style.display = "none";
		this.waiting.style.zIndex = 0;
	}
	showAll(){
		this.waiting.style.display = "inline-block";
		this.waiting.style.zIndex = 1000;
		this.userNodeAddToList();
	}
	showList(){
		this.roomLists.style.display = "inline-block";
		document.getElementsByClassName("waitingHeader")[0].style.display = "inline-block";

		let json = { // 10 
major_code : 1,
	     minor_code : 0,
	     from : {
uid : UID,
      nickname : NICKNAME
	     }
		}

		let msg = JSON.stringify(json);
		ws.send(msg);
	}
	hideList(){
		this.roomLists.style.display = "none";
	}
	showWaitingRoom(jsonObject){
		this.waitingRoom.style.display = "inline-block";
		this.waitingMembers.innerHTML = null;
		this.roomTitle.innerHTML = "Room No. " + ROOM_ID;
		this.roomTitle.style.display = "inline-block";
		this.memberListSet(jsonObject);
		this.userNodeAddToList();
	}
	hideWaitingRoom(){
		this.waitingRoom.style.display = "none";
		this.roomTitle.style.display = "none";
		this.roomTitle.innerHTML = null;
	}


	memberListSet(jsonObject){
		this.members = jsonObject.members;
	}

	userNodeAddToList(){
		this.hideList();
		if( this.members == undefined || this.members == null ) return;
		this.waitingMembers.innerHTML = null;
		this.gameMembers.innerHTML = null;


		for( let i = 0 ; i < this.members.length;i++){
			let node = document.createElement("li");
			node.className = "members " + (this.members[i].uid == leaderUID ? "leader " : "") + ( this.members[i].uid == UID ? "itsyou " : "" );
			node.setAttribute("data-uid", this.members[i].uid);

			let nickname = document.createElement("span");
			nickname.className="nickname";
			nickname.innerHTML = this.members[i].nickname;

			node.append(nickname);
			this.waitingMembers.append(node);


			let node2 = document.createElement("li");
			node2.className = "user " + (this.members[i].uid == painterUID ? "painter " : "") + (this.members[i].uid == leaderUID ? "leader " : "") + ( this.members[i].uid == UID ? "itsyou " : "");
			node2.setAttribute("data-uid", this.members[i].uid);

			let nickname2 = document.createElement("span");
			nickname2.className = "nickname";
			nickname2.innerHTML = this.members[i].nickname;

			let score = document.createElement("span");
			score.className = "score";
			score.innerHTML = this.members[i].score + " point";

			node2.append(nickname2);
			node2.append(score);

			this.gameMembers.append(node2);
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
		this.room = jsonObject.rlist;
		this.maxLength = parseInt(this.room.length / 8) + (this.room.length % 8 ? 1 : 0) - 1;
		this.nowPage = 0;

		waiting.roomNodeAddToList();
	}

	roomNodeAddToList(){
		this.hideWaitingRoom();
		if( this.room == undefined || this.room == null ) return;
		this.ul.innerHTML  = null;
		let cnt = 0;
		let since = this.nowPage * 8;
		let until = this.room.length < since + 8 ? this.room.length : since + 8;


		for( let i = since; i < until; i++){
			let node = document.createElement("li");
			node.className = "room " + ( (this.room[i].status%10) <= 1 ? "" : "ongame");
			node.addEventListener("click", statusManager.enterRoom, false);
			node.setAttribute("data-room_id", this.room[i].id);
			node.setAttribute("data-room_num", this.room[i].num);

			let roomID = document.createElement("span");
			roomID.innerHTML = "Room No." + this.room[i].id;
			roomID.className = "roomID";

			let roomNum = document.createElement("span");
			roomNum.innerHTML = this.room[i].num + "/6"; // 6 is max number of member;
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
	changeUserValue(member){
		for( let i = 0 ; i < this.members.length;i ++){
			if( this.members[i].uid == member.uid ){
				this.members[i].nickname = member.nickname;
				this.members[i].score = member.score;
			}
		}
		this.userNodeAddToList();
	}
	insertMember(jsonObject){
		this.members.push( { uid : jsonObject.user.uid, nickname : jsonObject.user.nickname, score : jsonObject.user.score } );
		this.userNodeAddToList();
	}
	popMember(jsonObject){
		for( let i = 0 ; i < this.members.length;i ++){
			if( this.members[i].uid == jsonObject.user.uid ){
				this.members.splice(i,1);
				break;
			}
		}
		this.userNodeAddToList();
	}
}




class App{
	constructor(){
		this.onGame = false;

		this.app = document.getElementById("app");
		this.clock = document.getElementsByClassName("clock")[0];

		this.nowTime = 0;
		this.roundEndTime = 0;

		this.loop = false;
	}
	showAll(){
		this.app.style.display = "inline-block";
		this.app.style.zIndex = 1000;
		draw.clear();
		if( !this.onGame ){
			this.onGame = true;
			chat.clear();
		}
		waiting.userNodeAddToList();
	}
	hideAll(){
		this.onGame = false;
		this.stopTimer();
		this.app.style.display = "none";
		this.app.style.zIndex = 0;
	}

	switchToWaiting(){
		this.hideAll();
		isPainter = false;
		waiting.showAll();
		waiting.hideList();
		status = 1;
	}

	startGame(jsonObject){
		draw.clear();
		this.nowTime = (new Date()).getTime();
		this.lastTime = this.nowTime;
		this.roundEndTime = this.nowTime + timelimit;
		this.clock.innerHTML = Math.floor( (this.roundEndTime - this.nowTime)/10) / 100;
		this.loop = false;
		this.sendTime();
	}

	stopTimer(){
		this.loop = false;
		this.initTime();
	}

	sendTime(){
		this.lastTime = this.nowTime;
		let json = { // 04
major_code : 0,
	     minor_code : 4,
	     room_id : ROOM_ID,
	     from : {
uid : UID,
      nickname : NICKNAME,
      score : SCORE
	     },
nowTime : ""+this.nowTime,
	  endTime : ""+this.roundEndTime
		}
		let msg = JSON.stringify(json);
		ws.send(msg);
	}

	painterTimer(){
		if( !isPainter ) return;
		this.nowTime = (new Date()).getTime();
		if( this.nowTime - this.lastTime > 500 )this.sendTime(this.nowTime);
		if( this.roundEndTime > this.nowTime ){
			this.clock.innerHTML = Math.floor( (this.roundEndTime - this.nowTime)/10) / 100;
			if( this.loop ) setTimeout(() => this.painterTimer(), 10);
		}
		else{
			this.clock.innerHTML = "0.00";
			this.timeDone();
			return;
		}
	}

	timer(){
		if( isPainter ) return;	
		if( this.roundEndTime > this.nowTime ){
			this.clock.innerHTML = Math.floor( (this.roundEndTime - this.nowTime)/10) / 100;
			if(this.loop){
				this.nowTime += 10;
				setTimeout( () =>this.timer(), 10);
			}
		}
		else{
			this.clock.innerHTML = "0.00";
			this.initTime();
			return;
		}
	}


	timeDone(){
		let json = { // 03
major_code : 0,
	     minor_code : 3,
	     room_id : ROOM_ID,
	     from : {
uid : UID,
      nickname : NICKNAME,
      score : SCORE
	     },
answer : ANSWER
		}

		let msg = JSON.stringify(json);
		ws.send(msg);
		this.initTime();
	}

	gameDone(){
		this.initTime();
		ANSWER = null;
		this.loop = false;
		this.nowTime = this.endTime = 0;
		isPainter = null; ANSWER = null; painterUID = null;
		notice.hideAnswer();
	}

	initTime(){
		this.nowTime = 0;
		this.roundEndTime = 0;
		this.loop = false;

	}

	syncTime(jsonObject){
		if( isPainter && !this.loop){
			this.loop = true;
			setTimeout(() => this.painterTimer(), 10);
			return;
		}
		if( this.nowTime == 0 ){
			this.nowTime = parseInt(jsonObject.nowTime);
			this.roundEndTime = parseInt(jsonObject.endTime);
			this.loop = true;
			setTimeout( () => this.timer(), 10);
		}
		else{
			this.nowTime = parseInt(jsonObject.nowTime);
			this.roundEndTime = parseInt(jsonObject.endTime);
		}
	}
}

class Notice{
	constructor(){
		this.answer = document.getElementsByClassName("answer")[0];
		this.clock = document.getElementsByClassName("clock")[0];
	}
	showAnswerLen(jsonObject){
		if( isPainter ) return;
		ANSWER = null;
		this.answer.innerHTML = null;
		for( let i = 0 ; i < jsonObject.answerLen; i++){
			this.answer.innerHTML += "_ ";
		}
	}
	showAnswer(jsonObject){
		this.answer.innerHTML = null;
		ANSWER = this.answer.innerHTML = jsonObject.answer;
	}
	hideAnswer(){
		this.answer.innerHTML = null;
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
		if(jsonObject.success ){
			if( jsonObject.from.uid == UID ) SCORE = jsonObject.from.score;

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
	}

	addSystemMsg(jsonObject){
		if(jsonObject.major_code == 0 ){
			if( jsonObject.minor_code == 3 ){ // 시간 초과 

				if( !SOMEONEGOTIT ){
					this.chatHistory.innerHTML += "\
								       <div class = 'msg'>\
								       <span class = 'contents system'>"
								       +"시간초과! 정답은 ["+ jsonObject.answer +"] 입니다!"+
								       "</span>\
								       </div>";
					this.chatHistory.scrollTop = this.chatHistory.scrollHeight;
				}
			}
			if(jsonObject.minor_code == 5){ // 정답자 공개 
				this.chatHistory.innerHTML += "\
							       <div class = 'msg'>\
							       <span class = 'contents system'>"
							       +"["+jsonObject.answer+"] '"+ jsonObject.winner.nickname + "' 님 정답입니다! (+10)"  +
							       "</span>\
							       </div>";
				this.chatHistory.scrollTop = this.chatHistory.scrollHeight;
			}
		}
		else if( jsonObject.major_code == 2){
			if( jsonObject.minor_code == 3){ // 23
				this.chatHistory.innerHTML += "\
							       <div class = 'msg'>\
							       <span class = 'contents system'>"
							       +"다음 문제 시작합니다. 그릴 사람은 '"+ jsonObject.painter.nickname + "' ("+jsonObject.answerLen+"글자)"
							       "</span>\
							       </div>";
				this.chatHistory.scrollTop = this.chatHistory.scrollHeight;
			}
		}
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
		let jsonObject = { // 01
major_code : 0,
	     minor_code : 1,
	     msg : box.value,
	     from : {
uid : UID,
      nickname : NICKNAME,
      score : SCORE
	     },
timestamp : (new Date()).getTime(),
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

		this.sendDrawingPoint(0);
	}

	drawWithJson(jsonObject){
		if( isPainter ) return;
		if( jsonObject.success){
			if(jsonObject.clear){
				this.canvas.clearRect(0, 0, this.tag.clientWidth, this.tag.clientHeight);
			}
			else{
				this.canvas.fillStyle = jsonObject.color;
				this.canvas.strokeStyle = jsonObject.color;
				this.canvas.lineWidth = jsonObject.px;

				this.prevX = jsonObject.prevX;
				this.prevY = jsonObject.prevY;
				this.x = jsonObject.x;
				this.y = jsonObject.y;
				this.draw();
			}
		}
	}

	sendDrawingPoint(clear_flag){
		if( !isPainter ) return;
		let jsonObject = { // 00
major_code : 0,
	     minor_code : 0,
	     prevX : this.prevX,
	     prevY : this.prevY,
	     x : this.x,
	     y : this.y,
	     color : this.brushType ? this.color : "#ffffff",
	     px : this.px,
	     room_id : ROOM_ID,
	     clear : clear_flag
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
		if(!isPainter) return;
		this.canvas.clearRect(0, 0, this.tag.clientWidth, this.tag.clientHeight);
		this.sendDrawingPoint(1);
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
			console.log("receive : " + json);

			let jsonObject = JSON.parse(json);

			if( jsonObject.major_code == 0 ){
				switch( jsonObject.minor_code ){
					case 0: // 00
						draw.drawWithJson(jsonObject);
						return;
					case 1: // 01
						chat.addMsg(jsonObject);
						return;
					case 2: // 02

						return;
					case 3: // 03
						statusManager.timeOut(jsonObject);
						return;
					case 4: // 04
						statusManager.timeSync(jsonObject);
						return;
					case 5: // 05
						statusManager.answerShow(jsonObject);
						return;
					default : console.log("undefined msg");return;
				}
			}
			else if( jsonObject.major_code == 2){
				switch( jsonObject.minor_code ){
					case 0 : // 20
						waiting.roomListSet(jsonObject);
						return;
					case 1 : // 21
						statusManager.enterRoomResponse(jsonObject);
						return;
					case 2 : // 22
						waiting.insertMember(jsonObject);
						return;
					case 3 : // 23
						statusManager.enterGameRoomResponse(jsonObject);
						return;
					case 4 : // 24
						statusManager.setAnswer(jsonObject);
						return;
					case 8 : // 28
						statusManager.exitRoomResponse(jsonObject);
						return;

					default : console.log("undefined msg");return;
				}
			}
			else if( jsonObject.major_code == 3){
				switch(jsonObject.minor_code ){
					case 0 : // 30
						statusManager.userAddResponse(jsonObject);
						return;
					case 1 :  // 31
						statusManager.setScoreResponse(jsonObject);
						return;
					case 2 : // 32
						statusManager.userPopHandle(jsonObject);
						return;
					case 3 : // 33
						statusManager.resetRoomStatus(jsonObject);
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
		console.log("send : " + msg);
		this.ws.send(msg);
	}
}
