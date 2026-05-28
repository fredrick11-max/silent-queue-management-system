#include <esp_now.h>
#include <WiFi.h>

#define MOTOR_PIN 10
#define LED_PIN 5
#define MY_BAND_ID 1// Change to 2 for the second band

typedef struct {
  int band_id;
  int cmd;
} struct_message;

typedef struct {
  int band_id;
  int status;
} struct_ack;

struct_message incoming;
struct_ack ack;

uint8_t hostMAC[6];

void triggerBand() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
}

void sendAck() {
  ack.band_id = MY_BAND_ID;
  ack.status = 1;
  esp_now_send(hostMAC, (uint8_t*)&ack, sizeof(ack));
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(struct_message)) return;

  memcpy(&incoming, data, sizeof(incoming));

  if (incoming.band_id != MY_BAND_ID) return;

  Serial.printf("Command received for band %d\n", MY_BAND_ID);

  // sender MAC address is inside info struct
  memcpy(hostMAC, info->src_addr, 6);

  if (incoming.cmd == 1) {
    triggerBand();
    sendAck();
    Serial.println("Triggered and ACK sent");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  Serial.printf("Band %d ready. MAC: ", MY_BAND_ID);
  Serial.println(WiFi.macAddress());
}

void loop() {}