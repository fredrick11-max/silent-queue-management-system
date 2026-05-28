# Components

Detailed description of every hardware component used in the Silent Queue Management System, including its role, specifications, and circuit notes.

---

## Host Unit

### ESP32 Development Board

- **Role:** Main controller for the host unit. Hosts the web interface and manages ESP-NOW communication.
- **Microcontroller:** Xtensa LX6 dual-core, 240 MHz
- **Wireless:** WiFi 802.11 b/g/n + Bluetooth 4.2
- **Flash:** 4MB
- **GPIO:** 34 usable pins
- **Power:** USB 5V via onboard regulator
- **Key feature used:** SoftAP mode to create local WiFi network, ESP-NOW for wireless band control
- **Notes:** Runs in `WIFI_AP_STA` mode simultaneously — SoftAP for staff device connection, STA mode for ESP-NOW operation.

---

## Wearable Band Units (x2)

### ESP32-C3 Super Mini

- **Role:** Main controller for each wearable band. Receives ESP-NOW commands, drives motor and LED, sends ACK.
- **Microcontroller:** RISC-V single-core, 160 MHz
- **Wireless:** WiFi 802.11 b/g/n + Bluetooth 5.0 LE
- **Flash:** 4MB
- **GPIO:** 13 usable pins
- **Power:** 3.3V regulated input
- **Operating current:** ~80mA active, ~5uA deep sleep
- **Key GPIOs used:** GPIO 4 (motor transistor base drive), GPIO 8 (LED output)
- **Notes:** Compact form factor suited for wearable use. Runs in `WIFI_STA` mode for ESP-NOW.

---

### LiPo Battery — 3.7V 500mAh

- **Role:** Portable power source for the wearable band.
- **Chemistry:** Lithium Polymer
- **Nominal voltage:** 3.7V
- **Capacity:** 500mAh
- **Estimated runtime:** Approximately 4 to 6 hours under intermittent use
- **Notes:** Battery output feeds the AMS1117 regulator for the ESP32-C3 and directly powers the motor rail through the transistor switch.

---

### AMS1117-3.3V Linear Voltage Regulator

- **Role:** Regulates 3.7V LiPo output down to stable 3.3V for the ESP32-C3.
- **Input voltage range:** 4.75V to 15V (minimum ~4.75V recommended, though 3.7V LiPo functions at startup)
- **Output voltage:** 3.3V fixed
- **Maximum output current:** 1A
- **Dropout voltage:** ~1.1V typical
- **Package:** SOT-223
- **Notes:** At 3.7V LiPo nominal, dropout is tight. Ensure decoupling capacitors (10uF + 100nF) on both input and output pins for stability. As the LiPo discharges below ~4.4V, output may drop slightly — this is acceptable for ESP32-C3 operation down to 3.0V.

---

### Coin Vibration Motor (DC)

- **Role:** Provides haptic (vibration) alert to the patient when their band is triggered.
- **Operating voltage:** 3V to 5V
- **No-load current:** 80mA to 120mA at 3.7V
- **Startup spike current:** Up to 500mA
- **Diameter:** Typically 10mm or 8mm coin form factor
- **Notes:** Must not be connected directly to ESP32-C3 GPIO. GPIO current limit is 40mA per pin. Motor is powered from the battery rail and switched via the 2N2222A transistor. A flyback diode is mandatory.

---

### LED (Standard 3mm or 5mm)

- **Role:** Visual alert indicator — flashes in sync with the motor when the band is triggered.
- **Forward voltage:** ~2.0V to 2.2V (red), ~3.0V to 3.2V (blue/white)
- **Recommended current:** 8mA to 10mA
- **Connected to:** GPIO 8 via a 220 ohm series resistor
- **Notes:** At 3.3V GPIO output with 220 ohm resistor, current through a red LED is approximately (3.3 - 2.0) / 220 = 5.9mA, which is safe and visible.

---

### 220 Ohm Resistor

- **Role:** Current limiting resistor for the LED.
- **Value:** 220 ohm
- **Power rating:** 0.25W (1/4W) — sufficient for LED current
- **Placement:** In series between GPIO 8 and LED anode

---

### 2N2222A NPN Transistor

- **Role:** Motor switching. Acts as a low-side switch controlled by GPIO 4.
- **Type:** NPN BJT
- **Collector current (IC max):** 600mA continuous
- **Collector-emitter voltage (VCEO):** 40V
- **DC current gain (hFE):** 100 to 300 at 150mA
- **Base resistor:** 1k ohm from GPIO 4 to base
- **Circuit configuration:**
  - Base: GPIO 4 via 1k ohm resistor
  - Collector: Motor negative terminal
  - Emitter: GND
  - Motor positive: Battery positive rail
- **Notes:** With GPIO 4 HIGH (3.3V), base current = (3.3 - 0.7) / 1000 = 2.6mA. For motor load of ~120mA, required base current = 120 / hFE = ~0.4mA minimum. 2.6mA provides sufficient overdrive for reliable saturation switching.

---

### 1N4007 Flyback Diode

- **Role:** Suppresses back EMF (inductive voltage spike) generated when the motor is switched off.
- **Type:** Rectifier diode
- **Reverse voltage rating:** 1000V
- **Forward current:** 1A
- **Placement:** Across motor terminals — anode to motor negative, cathode to motor positive (reverse biased during normal operation, conducts only during turn-off spike)
- **Notes:** Without this diode, the voltage spike when the transistor turns off can exceed 40V and destroy the 2N2222A transistor instantly.

---

### 1k Ohm Resistor (Base Resistor)

- **Role:** Limits base current into the 2N2222A transistor from GPIO 4.
- **Value:** 1k ohm
- **Placement:** Between GPIO 4 and transistor base
- **Notes:** Prevents excessive current draw from the GPIO pin while still providing enough base drive to saturate the transistor.

---

## Full Circuit Summary Per Band

```
LiPo (+) ──────────────────────────── Motor (+)
                                        Motor (-)
                                           |
                                       Collector
GPIO 4 ── 1k ohm ── Base (2N2222A)
                                       Emitter ── GND

Flyback diode: Anode at Motor(-), Cathode at Motor(+)

GPIO 8 ── 220 ohm ── LED Anode ── LED Cathode ── GND

LiPo (+) ── AMS1117-3.3V ── ESP32-C3 3V3 pin
LiPo (-) ── GND (common with ESP32 GND)
```

---

## Power Budget Per Band

| Component | Current Draw |
|---|---|
| ESP32-C3 Super Mini (active WiFi) | ~80mA |
| Coin motor (running) | ~100mA |
| LED (during alert) | ~6mA |
| AMS1117 quiescent current | ~5mA |
| **Total during alert** | **~191mA** |
| **Total idle (no alert)** | **~85mA** |

With a 500mAh LiPo:
- Idle runtime: ~5.8 hours
- Continuous alerting: ~2.6 hours (not a realistic use case)
- Practical runtime in a hospital queue context (intermittent alerts): 5 to 6 hours per charge
