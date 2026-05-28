#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>

WebServer server(80);

uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
  int band_id;
  int cmd;
} struct_message;

typedef struct {
  int band_id;
  int status;
} struct_ack;

struct_message outgoing;

// FIXED: use 1-based indexing safely by allocating 3 slots (0 unused)
volatile bool ackReceived[3] = {false, false, false};
volatile unsigned long ackTime[3] = {0, 0, 0};

// ================= ESP-NOW CALLBACKS (CORE 3.x FIXED) =================

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(struct_ack)) return;

  struct_ack ack;
  memcpy(&ack, data, sizeof(ack));

  if (ack.band_id >= 1 && ack.band_id <= 2 && ack.status == 1) {
    ackReceived[ack.band_id] = true;
    ackTime[ack.band_id] = millis();
    Serial.printf("ACK received from band %d\n", ack.band_id);
  }
}

// ================= ESP-NOW SEND =================

void sendToBand(int band_id) {
  outgoing.band_id = band_id;
  outgoing.cmd = 1;

  ackReceived[band_id] = false;

  esp_err_t result = esp_now_send(broadcastMAC, (uint8_t*)&outgoing, sizeof(outgoing));

  if (result == ESP_OK) {
    Serial.printf("Broadcast sent for band %d\n", band_id);
  } else {
    Serial.println("ESP-NOW send failed");
  }
}

// ================= WEB HANDLERS =================

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Band Control</title>
<style>
body { font-family: Arial; background:#0f172a; color:white; text-align:center; }
.card { margin:20px; padding:20px; background:#1e293b; border-radius:12px; display:inline-block; }
button { padding:10px 20px; margin-top:10px; }
.status { margin-top:10px; }
</style>
</head>
<body>

<h2>Band Control Panel</h2>

<div class="card">
  <h3>Patient 1</h3>
  <button onclick="trigger(1)">TRIGGER</button>
  <div id="status1">Ready</div>
</div>

<div class="card">
  <h3>Patient 2</h3>
  <button onclick="trigger(2)">TRIGGER</button>
  <div id="status2">Ready</div>
</div>

<script>
function trigger(id){
  document.getElementById('status'+id).innerText="Sending...";
  fetch('/trigger?id='+id)
  .then(()=> poll(id,0));
}

function poll(id,count){
  if(count>20){
    document.getElementById('status'+id).innerText="No ACK";
    return;
  }

  fetch('/ack?id='+id)
  .then(r=>r.text())
  .then(t=>{
    if(t==="1"){
      document.getElementById('status'+id).innerText="ACK received";
    } else {
      setTimeout(()=>poll(id,count+1),300);
    }
  });
}
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleTrigger() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "missing id");
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 1 || id > 2) {
    server.send(400, "text/plain", "invalid id");
    return;
  }

  sendToBand(id);
  server.send(200, "text/plain", "sent");
}

void handleAckCheck() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "missing id");
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 1 || id > 2) {
    server.send(400, "text/plain", "invalid id");
    return;
  }

  server.send(200, "text/plain", ackReceived[id] ? "1" : "0");
}

// ================= SETUP =================

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("BandControl", "12345678");

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);

  server.on("/", handleRoot);
  server.on("/trigger", handleTrigger);
  server.on("/ack", handleAckCheck);

  server.begin();
  Serial.println("Host ready");
}

// ================= LOOP =================

void loop() {
  server.handleClient();
}