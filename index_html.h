static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang=en>
<head>
<meta charset="UTF-8">
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>WiFi Control 0.01</title>
<style type="text/css">
    table {
        display: table;
        table-layout: fixed;
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
        height: 100%;
        width: 100%;
        border-collapse: collapse;
    }

    td {
        border: 1px solid;
        font-size: 200%;
        text-align: center;
    }
</style>
<script type="text/javascript">
const WS_OPEN = 1;
const WS_CLOSED = 3;
let websock;
let websockQueue = [];
let LastEvent = "";
let connected = false;

function websockQueue_flush() {
  if (websock.readyState === WS_OPEN) {
    for (let i = 0; i < websockQueue.length; i++) {
      websock.send(websockQueue[i]);
    }
    websockQueue = [];
  }
}

const websock_interval = setInterval(websockQueue_flush, 50);

function websock_send(event) {
  if (websock.readyState === WS_OPEN) {
    websockQueue_flush();
    websock.send(event);
    LastEvent = event;
  } else {
    websockQueue.push(event);
    if (websock.readyState === WS_CLOSED) {
      websock = new WebSocket('ws://' + window.location.hostname + ':81/');
    }
  }
}

function button_LED_on(event) {
  websock_send("LED_on");
}

function button_LED_off(event) {
  websock_send("LED_off");
}

function start() {
  websock = new WebSocket('ws://' + location.hostname + ':81/');
  websock.onopen = function(evt) {
    console.log('websock onopen', evt);
    connected = true;
    websockQueue_flush();
  };
  websock.onclose = function(evt) {
    console.log('websock onclose', evt);
    if (LastEvent !== "") {
      websock_send(LastEvent);
      LastEvent = "";
    }
    connected = false;
  };
  websock.onerror = function(evt) {
    console.log('websock onerror', evt);
    if (LastEvent !== "") {
      websock_send(LastEvent);
      LastEvent = "";
    }
    connected = false;
  };
  websock.onmessage = function(evt) {
    console.log('websock onmessage', evt);
  };

  let e = document.getElementById("LED_on");
  e.addEventListener("click", button_LED_on);
  e = document.getElementById("LED_off");
  e.addEventListener("click", button_LED_off);
}

document.addEventListener("DOMContentLoaded", start);

</script>
</head>
<body>
  <table>
    <tbody>
      <tr>
        <td id="LED_on"><button>LED On</button></td>
        <td id="LED_off"><button>LED Off</button></td>
      </tr>
    </tbody>
  </table>
  <br>
  <div id="keyCodes">
  </div>
</body>
</html>
)rawliteral";
