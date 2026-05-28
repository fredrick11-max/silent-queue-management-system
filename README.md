# Silent Queue Management System

A wireless, wearable-based patient alerting system designed to replace conventional token number announcements in hospital waiting areas.

---

## Overview

Hospitals are high-stress environments where patients wait anxiously for their turn. Traditional token systems rely on loud verbal announcements or number displays, which often cause confusion, missed turns, and increased anxiety — especially for elderly or differently-abled patients.

The **Silent Queue Management System** eliminates this problem by assigning each patient a wearable ESP32-C3 Super Mini band. When a patient's turn arrives, hospital staff trigger their band remotely through a web interface, activating a silent vibration and LED indicator on the wearable — no announcements, no confusion.

---

## Key Features

- Fully wireless, no internet required
- Silent haptic and visual alerts via wearable band
- Web interface for hospital staff to control bands
- ESP-NOW broadcast with ID-based filtering for precise targeting
- Acknowledgement feedback confirming successful band trigger
- No smartphone needed by the patient
- Scalable and reusable across hospital settings
- Cost-effective — each band under Rs. 500 in bulk

---

## System Components

| Unit | Hardware |
|---|---|
| Host Controller | ESP32 Development Board (USB powered) |
| Wearable Band x2 | ESP32-C3 Super Mini + LiPo 3.7V 500mAh + AMS1117-3.3V |
| Alert Hardware | Coin vibration motor + LED + 2N2222A transistor + 1N4007 diode |

---

## How It Works

1. Staff opens the web interface hosted by the ESP32 host over a local WiFi access point.
2. Staff selects a Patient ID (mapped to Band 1 or Band 2) and clicks Trigger.
3. The host broadcasts an ESP-NOW packet containing the target band ID.
4. Each band receives the broadcast and filters it — only the addressed band activates.
5. The addressed band vibrates and flashes its LED to alert the patient.
6. The band sends an acknowledgement back to the host.
7. The web interface confirms the band was successfully triggered.

---

## Repository Structure

```
silent-queue-management/
│
├── host/
│   └── host.ino                  # ESP32 host firmware with web interface
│
├── receiver/
│   └── receiver.ino              # ESP32-C3 band firmware (set MY_BAND_ID per unit)
│
├── docs/
│   ├── README.md                 # This file
│   ├── Architecture.md           # System architecture overview
│   ├── communication-flow.md     # ESP-NOW communication protocol details
│   ├── components.md             # Detailed component descriptions and circuit notes
│   └── components-list.txt       # Quick bill of materials
```

---

## Getting Started

### Prerequisites

- Arduino IDE with ESP32 board package version 2.0.x installed
- Board: `ESP32C3 Dev Module` selected for the receiver
- Board: `ESP32 Dev Module` selected for the host

### Flashing the Host

1. Open `host/host.ino` in Arduino IDE.
2. Select your ESP32 board and COM port.
3. Upload the firmware.
4. Connect to WiFi AP: `BandControl` / Password: `12345678`
5. Open browser and navigate to `192.168.4.1`

### Flashing the Receiver Bands

1. Open `receiver/receiver.ino` in Arduino IDE.
2. Set `MY_BAND_ID 1` for Band 1, `MY_BAND_ID 2` for Band 2.
3. Select `ESP32C3 Dev Module` and the correct COM port.
4. Upload separately to each band unit.

---

## Circuit Notes

- Motor is powered from the battery rail directly, not from the ESP32 3V3 pin.
- 2N2222A NPN transistor switches the motor ground side via GPIO 4.
- 1N4007 flyback diode is placed across motor terminals to suppress back EMF.
- LED on GPIO 8 uses a 220 ohm series resistor.
- Common GND must be shared between battery and ESP32.

---

## Project Context

Developed as a college project at the School of Computing — CSE, this system demonstrates practical application of embedded systems, wireless communication protocols, and IoT hardware design in a real-world healthcare scenario.

---

## License

MIT License. Free to use, modify, and distribute with attribution.
