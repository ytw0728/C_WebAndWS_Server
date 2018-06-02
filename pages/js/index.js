let ws;

window.onload= function(){
	ws = new Websocket();
	layout();

	canvas = new Draw();
}

function layout(){
	let chat = document.getElementById("chat");
	let chatHistory = document.getElementsByClassName("chatHistory")[0];
	let inputBox = document.getElementsByClassName("inputBox")[0];

	chatHistory.style.height = ( chat.offsetHeight - inputBox.clientHeight - chat.offsetHeight / 100 * 5 ) + "px";
}


class Websocket{
	constructor(){
		this.ws = new WebSocket("ws://localhost:12345");
		this.ws.onopen = function (event) {
		  this.send("Hello, Server? 안녕, 서버?");
		};
		this.ws.onmessage = function (event) {
			let tmp = event.data.replace( /(?:\r\n|\r|\n)/g, '<br>');
			let chatHistory = document.getElementsByClassName("chatHistory")[0];
			console.log(event.data);

			chatHistory.innerHTML += "\
					<div class = 'msg'>\
						<span class = 'sender'>\
							client | \
						</span>\
						<span class = 'contents'>"
						 + tmp + 
						"</span>\
					</div>";

			chatHistory.scrollTop = chatHistory.scrollHeight;
		}
		this.ws.onclose = function(event){
			console.log("ws is closed");
		}
		this.ws.onerror = function(event){
			console.log("Err");
			this.close();
		}

		this.initEvent();
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

	mouseHandle(event){
		event.preventDefault();
		if( event.type == "mousedown"){
			let position = this.getMousePos(event);
			this.flag = true;
			this.prevX = this.x = position.x;
			this.prevY = this.y = position.y;
			this.draw();
		}
		else if( event.type == "mousemove"){
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
			let position = this.getMousePos(event);
			this.flag = false;
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
        // this.canvas.arc(this.x, this.y,  this.canvas.lineWidth, 0,Math.PI*2,true);
        // this.canvas.fill();
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