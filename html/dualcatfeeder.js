
var connection = new WebSocket('ws://192.168.178.25:81/');
connection.onopen = function () {
  connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {
  console.log('Server: ', e.data);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};

function startMotorsForwards (){
  var obj = new webSocketActionObject("forwards", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

function startMotorsForwardsSeconds(){
  var seconds = document.getElementById("seconds_forwards").value;
  if ( seconds > 0 ) {
    var obj = new webSocketActionObject("forwards", seconds);
    var jsonString = JSON.stringify(obj);
    connection.send(jsonString);
  }
}

function startMotorsBackwards (){
  var obj = new webSocketActionObject("backwards", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

function stopMotors (){
  var obj = new webSocketActionObject("stop", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

class webSocketActionObject{
  constructor(action, time){
    this.action = action;
    this.time   = time;
  }
}