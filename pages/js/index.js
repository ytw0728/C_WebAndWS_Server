let ws;
window.onload= function(){
	let p = document.getElementsByClassName("chatHistory")[0];
	let app = document.getElementById("app");



	ws = new WebSocket("ws://localhost:12345");

	ws.onopen = function (event) {
	  // ws.send("Hello, Server?"); 
	  ws.send("안녕, 서버?"); 
	};

	ws.onmessage = function (event) {
		let tmp = event.data.replace( /(?:\r\n|\r|\n)/g, '<br>');

		p.innerHTML += "<div class = 'msg'><span class = 'sender'>client | </span><span class = 'contents'>" + tmp + "</span></div>";
		let chatHistory = document.getElementsByClassName("chatHistory")[0];
		chatHistory.scrollTop = chatHistory.scrollHeight;
	}

	ws.onclose = function(event){
		console.log("ws is closed");
	}
	ws.onerror = function(event){
		console.log("Err");
		ws.close();
	}
}


function sendMessage(event){
	event.preventDefault();
	console.log("connected");
	let box = document.getElementById("chatInput");
	ws.send(box.value);
	box.value = null;
}

function keypressHandle(event){
	let keycode =  event.which? event.which : event.keyCode;

	if( keycode == 	13 ){
		if( !event.shiftKey){
			sendMessage(event);
		}
	}
}