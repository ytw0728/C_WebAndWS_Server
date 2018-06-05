let status = 0; // 0 : waiting , 1 : gameroom , 2 : game 
let uid = null; // user uid
let nickname = null; // user nickname

let ws;
let chat;
let waiting;
let app;
window.onload= function(){
	chat = new Chatting();
	waiting = new Waiting();
	app = new App();
	notice = new Notice();
	ws = new Websocket();

	statusManager = new StatusManager();
	layout();

	canvas = new Draw();
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
		document.getElementsByClassName("nicknameInputBox")[0].style.display = "none";
		document.getElementsByClassName("nicknameInputBox")[0].style.opacity = "0";
		document.getElementsByClassName("waitingHeader")[0].style.display = "inline-block";

		waiting.showList(null);
		scrollTo(0,0);
	}

	enterRoom(event){
		event.preventDefault();
		let target = event.target;
		window.target = target;
		if( !target.hasAttribute("data-roomid") ) target = target.parentNode
		let roomid = target.getAttribute("data-roomid");
		alert(roomid);

		waiting.hideList();
		waiting.showWaitingRoom(null);
	}
	makeRoom(event){
		event.preventDefault();
		alert("activated");

		waiting.hideList();
		waiting.showWaitingRoom()
	}

	exitRoom(event){
		waiting.hideWaitingRoom();
		waiting.showList();
	}

	enterGameRoom(event){
		waiting.hideAll();
		app.showAll();
	}
	exitGameRoom(event){
		app.hideAll();
		waiting.showAll();
		waiting.showList(null);
		waiting.hideWaitingRoom();
	}
}

class App{
	constructor(){
		this.app = document.getElementById("app");
	}
	showAll(){
		this.app.style.display = "inline-block";
		this.app.style.zIndex = 1000;
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
		this.initEvent();
	}
	addMsg(jsonObject){
		this.chatHistory.innerHTML += "\
			<div class = 'msg'>\
				<span class = 'sender'>"
					+ jsonObject.nickname  + " | " + 
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
		this.ws.send(box.value);

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

class Websocket{
	constructor(){
		this.ws = new WebSocket("ws://localhost:12345");
		this.ws.onopen = function (event) {
		  this.send("Hello, Server? 안녕, 서버?");
		};
		this.ws.onmessage = function (event){

			// let packet = event.data;

			let tmp = event.data.replace(/(?:\r\n|\n|\r)/g, '<br/>'); // dummyData
			let packet = '00{"code":"00","msg":"'+tmp+'","uid":1,"nickname":"someone","timestamp":"'+new Date()+'"}';
			let code = packet.substring(0,2);
			let json = packet.substring(2,packet.length);
			let jsonObject = JSON.parse(json);


			if( code === "00" ){ // message
				chat.addMsg(jsonObject);
				return;
			}
			if( code === "23"){
				notice.showAnswerLen(jsonObject);
				return;
			}
		}
		this.ws.onclose = function(event){
			console.log("ws is closed");
		}
		this.ws.onerror = function(event){
			console.log("Err");
			this.close();
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

        this.canvas.lineWidth = "5";
        this.canvas.lineCap = "round";
        this.canvas.lineJoin = "round";

		this.prevX = this.prevY = 0;
		this.x = this.y = 0;

		this.initEvent();
	}

	initEvent(){
		let colors = document.getElementsByClassName("color");
		for( let i = 0 ; i < colors.length ; i++){
			colors[i].addEventListener("click", this.colorChange.bind(this));
		}

		let brush = document.getElementsByClassName("brush");
		for( let i = 0 ; i < brush.length ; i++){
			brush[i].addEventListener("click", this.brushChange.bind(this));
		}

		let pencil = document.getElementsByClassName("pencil")[0];
		pencil.addEventListener("click", this.setPencil.bind(this));
		let eraser = document.getElementsByClassName("eraser")[0];
		eraser.addEventListener("click", this.setEraser.bind(this));

		let clear = document.getElementsByClassName("clear")[0];
		clear.addEventListener("click", this.clearCanvas.bind(this));
	}

	sendCoordinate(prevX, prevY, x, y){
		// make to json and send it
	}
	mouseHandle(event){
		event.preventDefault();
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
  	}

  	colorChange(event){
  		let target = event.target;
  		let color = target.getAttribute("data-color");
		this.canvas.fillStyle = this.canvas.strokeStyle = this.color = color;
  	}

  	brushChange(event){
  		let target = event.target;
  		let px = target.getAttribute("data-px");
  		this.canvas.lineWidth = px;
  	}
  	setPencil(event){
  		this.brushType = true;
  		this.canvas.fillStyle = this.color;
  		this.canvas.strokeStyle = this.color;
  	}
  	setEraser(event){
  		this.brushType = false;
		this.canvas.fillStyle = "#ffffff";
        this.canvas.strokeStyle = "#ffffff";
  	}
  	clearCanvas(event){
  		this.canvas.clearRect(0, 0, this.tag.clientWidth, this.tag.clientHeight);
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
				{roomID : 1,roomNum : 3},
				{roomID : 2,roomNum : 3},
				{roomID : 3,roomNum : 3},
				{roomID : 4,roomNum : 3},
				{roomID : 5,roomNum : 3},
				{roomID : 6,roomNum : 3},
				{roomID : 7,roomNum : 3},
				{roomID : 8,roomNum : 3},
				{roomID : 9,roomNum : 3},
				{roomID : 10,roomNum : 3},
				{roomID : 11,roomNum : 3},
				{roomID : 12,roomNum : 3},
				{roomID : 13,roomNum : 3},
				{roomID : 14,roomNum : 3},
				{roomID : 15,roomNum : 3},
				{roomID : 16,roomNum : 3},
				{roomID : 17,roomNum : 3}
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
			node.setAttribute("data-roomid", this.room[i].roomID);

			let roomID = document.createElement("span");
			roomID.innerHTML = "Room No." + this.room[i].roomID;
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
