
var connection = new WebSocket('ws://' + location.hostname + ':81/');
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

function startMotorsForwards() {
  var obj = new webSocketActionObject("forwards", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

function startMotorsForwardsSeconds() {
  var seconds = document.getElementById("seconds_forwards").value;
  if (seconds > 0) {
    var obj = new webSocketActionObject("forwards", seconds * 1000);
    var jsonString = JSON.stringify(obj);
    connection.send(jsonString);
  }
}

function startMotorsBackwardsSeconds() {
  var seconds = document.getElementById("seconds_backwards").value;
  if (seconds > 0) {
    var obj = new webSocketActionObject("backwards", seconds * 1000);
    var jsonString = JSON.stringify(obj);
    connection.send(jsonString);
  }
}

function startMotorsBackwards() {
  var obj = new webSocketActionObject("backwards", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

function stopMotors() {
  var obj = new webSocketActionObject("stop", 0);
  var jsonString = JSON.stringify(obj);
  connection.send(jsonString);
}

class webSocketActionObject {
  constructor(action, time) {
    this.action = action;
    this.time = time;
  }
}