# System Architecture

## Overview

The Silent Queue Management System is built on a three-tier wireless architecture: a staff-facing web interface, a host ESP32 controller, and two wearable ESP32-C3 patient bands. All communication happens locally over ESP-NOW and a softAP WiFi network — no internet, no cloud, no external dependencies.

---

## Architectural Diagram

```
+---------------------------+
|     Hospital Staff        |
|   (Browser / Mobile)      |
+---------------------------+
            |
            | HTTP over WiFi (192.168.4.1)
            |
+---------------------------+
|    HOST UNIT              |
|    ESP32 Dev Board        |
|                           |
|  - Hosts Web Interface    |
|  - Manages Patient IDs    |
|  - Sends ESP-NOW Broadcast|
|  - Receives ACK from bands|
|  - Powered via USB        |
+---------------------------+
            |
            | ESP-NOW Broadcast (2.4 GHz)
            | Packet contains: {band_id, cmd}
            |
     +------+-------+
     |               |
+----------+    +----------+
| BAND 1   |    | BAND 2   |
| ESP32-C3 |    | ESP32-C3 |
| Super    |    | Super    |
| Mini     |    | Mini     |
|          |    |          |
| ID = 1   |    | ID = 2   |
| Motor    |    | Motor    |
| LED      |    | LED      |
| LiPo     |    | LiPo     |
+----------+    +----------+
     |               |
     +------+-------+
            |
            | ESP-NOW Unicast ACK back to host
            | Packet contains: {band_id, status}
            |
+---------------------------+
|    HOST UNIT              |
|  - Receives ACK           |
|  - Updates Web UI         |
+---------------------------+
```

---

## Layers

### Layer 1 — Presentation (Web Interface)

- Runs as a single-page HTML application served directly from the ESP32 host.
- Staff access it through any browser on a device connected to the ESP32 softAP.
- Displays Patient ID 1 and Patient ID 2 trigger buttons with live ACK status.
- Polls the host every 300ms for acknowledgement after a trigger is sent.

### Layer 2 — Host Controller (ESP32)

- Creates a WiFi SoftAP named `BandControl` for staff devices to connect.
- Runs a lightweight HTTP WebServer on port 80.
- Handles three routes: `/` (UI), `/trigger?id=N` (send command), `/ack?id=N` (check ACK).
- Registers a broadcast ESP-NOW peer at `FF:FF:FF:FF:FF:FF`.
- Transmits a `struct_message {band_id, cmd}` packet via broadcast.
- Listens for incoming `struct_ack {band_id, status}` packets from bands.
- Stores ACK state per band in a volatile boolean array.

### Layer 3 — Wearable Band (ESP32-C3 Super Mini)

- Each band has a hardcoded `MY_BAND_ID` (1 or 2) compiled into firmware.
- Listens for ESP-NOW broadcast packets.
- Filters by `band_id` field — ignores packets not addressed to it.
- On match, activates the motor and LED in a pulse pattern (2 cycles, 300ms on / 200ms off).
- Captures sender MAC from the incoming packet and sends ACK back via unicast.
- Powered by a 3.7V 500mAh LiPo through an AMS1117-3.3V regulator.

---

## Communication Protocol

- **Transport:** ESP-NOW (IEEE 802.11 based, peer-to-peer, no router needed)
- **Direction Host to Band:** Broadcast to `FF:FF:FF:FF:FF:FF`
- **Direction Band to Host:** Unicast to host MAC captured at receive time
- **Payload Host to Band:** `{int band_id, int cmd}` — 8 bytes
- **Payload Band to Host:** `{int band_id, int status}` — 8 bytes
- **Range:** Up to 200m line of sight, ~50m indoors typical

---

## Network Topology

```
SoftAP Network (192.168.4.x)     ESP-NOW Network (2.4 GHz channel 0)
+--------------------+            +------------------------------------+
| Staff Device       |            | Host ESP32                         |
| 192.168.4.2        |<--HTTP---->| 192.168.4.1 (SoftAP)              |
+--------------------+            | MAC: XX:XX:XX:XX:XX:XX            |
                                  |              |                     |
                                  |    ESP-NOW Broadcast               |
                                  |         /        \                 |
                                  |    Band 1       Band 2            |
                                  +------------------------------------+
```

---

## Design Decisions

| Decision | Reason |
|---|---|
| ESP-NOW over standard WiFi | No router dependency, lower latency, works offline |
| Broadcast + ID filter over unicast pairing | Simpler to add more bands, no complex pairing needed |
| ACK via unicast back to host | Host MAC captured dynamically, no hardcoding |
| AMS1117 regulator on band | Stable 3.3V for ESP32-C3, protects against LiPo voltage variation |
| Transistor motor driver | GPIO cannot supply enough current to drive motor directly |
| Flyback diode on motor | Suppresses back EMF spike that would otherwise damage the transistor |
