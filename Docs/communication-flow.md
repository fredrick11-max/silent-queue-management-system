# Communication Flow

## Protocol: ESP-NOW

ESP-NOW is a connectionless wireless protocol developed by Espressif that operates over the 2.4 GHz band. It does not require a WiFi router or internet connection. Devices communicate peer-to-peer using MAC addresses, with payloads up to 250 bytes and latency typically under 10ms.

---

## Packet Structures

### Host to Band (Command Packet)

```cpp
typedef struct {
  int band_id;   // Target band identifier (1 or 2)
  int cmd;       // Command code (1 = trigger)
} struct_message;
```

Total size: 8 bytes

### Band to Host (Acknowledgement Packet)

```cpp
typedef struct {
  int band_id;   // ID of the band sending the ACK
  int status;    // 1 = successfully triggered
} struct_ack;
```

Total size: 8 bytes

---

## Full Communication Sequence

```
Staff Browser          Host ESP32                  Band N (ESP32-C3)
     |                      |                             |
     |-- GET /trigger?id=1 ->|                             |
     |                      |                             |
     |                      | esp_now_send(               |
     |                      |   FF:FF:FF:FF:FF:FF,        |
     |                      |   {band_id=1, cmd=1}        |
     |                      | )                           |
     |                      |------ ESP-NOW Broadcast --->|
     |                      |                             |
     |                      |          [Band checks band_id == MY_BAND_ID]
     |                      |          [Match found: trigger motor + LED]
     |                      |          [Pulse 2x: 300ms ON / 200ms OFF]
     |                      |                             |
     |                      |<----- ESP-NOW Unicast ACK --|
     |                      |   {band_id=1, status=1}     |
     |                      |                             |
     |                      | ackReceived[1] = true       |
     |                      |                             |
     |-- GET /ack?id=1 ----->|                             |
     |<-- "1" --------------|                             |
     |                      |                             |
  [UI shows green confirmation]
```

---

## Broadcast and Filtering

The host always sends to the broadcast MAC address `FF:FF:FF:FF:FF:FF`. This means all ESP-NOW-registered devices within range receive every packet. ID filtering happens entirely at the receiver:

```cpp
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  memcpy(&incoming, data, sizeof(incoming));

  if (incoming.band_id != MY_BAND_ID) return;  // drop if not for this band

  // proceed with trigger
}
```

This approach means:
- Adding a new band requires no changes to the host firmware
- No pairing or address registration needed per band
- All bands can coexist and operate independently

---

## Acknowledgement Flow

1. After triggering, the band captures the sender MAC from the incoming packet parameter.
2. The band registers the host as a temporary peer and sends a unicast ACK packet back.
3. The host receives the ACK in its own `onReceive` callback and sets `ackReceived[band_id] = true`.
4. The web UI polls `/ack?id=N` every 300ms for up to 6 seconds (20 attempts).
5. If ACK is received within that window, the UI shows a green confirmation.
6. If no ACK is received, the UI shows a red timeout warning.

---

## ACK Peer Registration on Band

Before the band can send back to the host, the host must be registered as an ESP-NOW peer on the band. This is done dynamically at receive time:

```cpp
esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, hostMAC, 6);
peerInfo.channel = 0;
peerInfo.encrypt = false;

if (!esp_now_is_peer_exist(hostMAC)) {
  esp_now_add_peer(&peerInfo);
}

esp_now_send(hostMAC, (uint8_t*)&ack, sizeof(ack));
```

---

## Channel and Range

| Parameter | Value |
|---|---|
| Frequency | 2.4 GHz |
| ESP-NOW Channel | 0 (auto, matches SoftAP channel) |
| Max Payload | 250 bytes |
| Typical Indoor Range | 30 to 50 meters |
| Line of Sight Range | Up to 200 meters |
| Latency | Under 10ms typical |
| Encryption | Disabled (not required for this use case) |

---

## Error Conditions

| Condition | Behaviour |
|---|---|
| Band out of range | No ACK received, UI shows timeout after 6 seconds |
| Band powered off | Same as out of range |
| Wrong band ID in packet | Band silently drops the packet, no response |
| Host send fails | `onSent` callback logs FAIL to Serial, UI still polls for ACK |
| Duplicate trigger | Band re-triggers if a second valid packet arrives; ackReceived resets on each new trigger |
