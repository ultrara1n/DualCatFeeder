class webSocketActionObject {
  constructor(action, time) {
    this.action = action;
    this.time = time;
  }
}

class webSocketTimerObject {
  constructor(action, timestamp, forSeconds, activated) {
    this.action = action;
    //this.hour = hour;
    //this.minute = minute;
    this.timestamp = timestamp
    this.forSeconds = forSeconds;
    this.activated = activated;
  }
}

var connection = new WebSocket('ws://' + location.hostname + ':81/');
connection.onopen = function () {
  connection.send('Connect ' + new Date());
  var element = document.getElementById("connectionStatus");
  element.classList.remove("alert-warning");
  element.classList.remove("alert-error");
  element.classList.add("alert-success");
  element.innerText = "Connected!";
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
  var element = document.getElementById("connectionStatus");
  element.classList.remove("alert-warning");
  element.classList.remove("alert-success");
  element.classList.add("alert-danger");
  element.innerText = "Cannot connect!";
};
connection.onmessage = function (e) {
  console.log('Server: ', e.data);

  //Load initial status
  var jsonObj = JSON.parse(e.data);
  if (jsonObj.type == "initial"){
    timerOnConnect(jsonObj);
    statsOnConnect(jsonObj);
  } else if (jsonObj.type == "answer" ){
    
  }
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
  var element = document.getElementById("connectionStatus");
  element.classList.remove("alert-warning");
  element.classList.remove("alert-success");
  element.classList.add("alert-danger");
  element.innerText = "Connection lost!";
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
  connection.send('{"action":"stop"}');
}

function saveTimer1() {
  var action = "saveTimer";
  var selectedTime = document.getElementById("timer1Time").value;
  var selectedHour = parseInt(selectedTime.split(":")[0]);
  var selectedMinute = parseInt(selectedTime.split(":")[1]);
  var forSeconds = parseInt(document.getElementById("timer1Seconds").value);
  var radioActivated = document.getElementById("timer1Activated").checked;
  var radioDeactivated = document.getElementById("timer1Deactivated").checked;

  var calculatedDate = new Date(1970, 0, 1, selectedHour, selectedMinute);
  var calculatedTimestamp = Math.round(calculatedDate.getTime()/1000);

  if (selectedTime != null && forSeconds != null && (radioActivated == true || radioDeactivated == true)) {
    if (radioActivated == true) {
      var active = true;
    } else {
      active = false;
    }
    //var timerObject = new webSocketTimerObject(action, selectedHour, selectedMinute, forSeconds, active);
    var timerObject = new webSocketTimerObject(action, calculatedTimestamp, forSeconds, active);
    var jsonString = JSON.stringify(timerObject);

    connection.send(jsonString);
  }
}

function timerOnConnect(jsonObj) {
  //Timer 1
  var timer1 = jsonObj.timer[1];
  //var selectedTime = leadingZero(timer1.hour) + ':' + leadingZero(timer1.minute);
  var timestampToDate = new Date(timer1.timestamp*1000);

  document.getElementById("timer1Time").value = leadingZero(timestampToDate.getHours()) + ':' + leadingZero(timestampToDate.getMinutes());


  document.getElementById("timer1Seconds").value = timer1.seconds

  if (timer1.active == true) {
    document.getElementById("timer1Activated").checked = true;
  } else {
    document.getElementById("timer1Deactivated").checked = true;
  }

}

function statsOnConnect(jsonObj) {
  var dateBootTime = new Date(jsonObj.boottime*1000);
  var dateLastFeedTime = new Date(jsonObj.lastfeedtime*1000);

  document.getElementById("bootTime").value = dateBootTime.toUTCString();
  document.getElementById("rebootReason").value = jsonObj.rebootreason;
  document.getElementById("lastFeedTime").value = dateLastFeedTime.toUTCString();
  document.getElementById("lastFeedDuration").value = jsonObj.lastfeedduration / 1000;

}

function leadingZero(number) {
  if (number < 10){
    return '0' + number;
  } else {
    return number;
  }
}

function rebootESP() {
  connection.send('{"action":"reboot"}');
}

function flushPreferencesESP(){
  connection.send('{"action":"flush"}');
}

function activateLog(){
  var textDiv = document.getElementById("logTextArea");
  textDiv.style.display = "";
}