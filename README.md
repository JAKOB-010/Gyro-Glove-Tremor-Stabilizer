# Design & Development of a Gyroscope-Based Hand Tremor Stabilizing Glove

An open-source, IoT-enabled active assistive wearable device engineered to detect, analyze, and mechanically suppress pathological hand tremors (4–12 Hz) commonly seen in Parkinson's Disease and Essential Tremor patients. This system pairs hardware-level analog signal filtering with an asynchronous, non-blocking finite state machine (FSM) and an IoT telemetry network stack.

---

## 📌 Project Overview & Mechanical Principle
Pathological hand tremors introduce involuntary, rhythmic, oscillatory movements that severely disrupt activities of daily living (ADL) such as eating, drinking, or writing. 

This wearable glove addresses the issue by exploiting the **physical principle of Gyroscopic Precession** to actively counteract these oscillations. When a heavy flywheel rotates at a high velocity, it develops high angular momentum. Any sudden external torque attempting to tilt or shift the axis of rotation (such as a muscle tremor) experiences an immediate, perpendicular mechanical counter-torque (gyroscopic resistance). 

By dynamically spinning up a flywheel attached to a high-RPM Brushless DC (BLDC) motor directly on the dorsal side of the hand, the glove creates a localized mechanical stabilizer that dampens physical shaking while remaining transparent to the patient's slow, intentional intentional movements.

---

## ⚙️ Hardware Components & Bill of Materials

Here is a detailed breakdown of the physical components used to build the Gyro Glove prototype:

### 1. Microcontroller: ESP32 DevKit V1
<img width="459" alt="ESP32 Microcontroller" src="https://github.com/user-attachments/assets/19cbdaa7-6fce-4ed1-93ae-d01e27b2d3bf" />

The ESP32 serves as the "brain" of the glove. It is a powerful 32-bit microcontroller equipped with built-in Wi-Fi. It is responsible for reading the analog signals from the accelerometer, running the hysteresis cross-counting algorithm to calculate tremor frequency, outputting the PWM control signal to the motor, and handling all asynchronous cloud telemetry uploads to ThingSpeak.

### 2. Actuator: High-Speed Brushless DC (BLDC) Motor
<img width="614" alt="BLDC Motor" src="https://github.com/user-attachments/assets/ad98e686-11dc-4425-ba5b-b0a1d1aa7c2b" />

Unlike standard vibration motors, this high-RPM brushless motor is designed to spin a heavy external flywheel at incredible speeds (between 9,000 and 11,400 RPM). This rapid rotation generates the high **gyroscopic precession** (angular momentum) required to physically resist, dampen, and stabilize sudden pathological hand tremors.

### 3. Power Supply: Lithium-Polymer (LiPo) Battery
<img width="457" alt="LiPo Battery" src="https://github.com/user-attachments/assets/e6de2be5-9bbf-46cf-84a5-ddf8e31c90b7" />

A high-discharge LiPo battery is utilized to power the mechanical drivetrain. The BLDC motor demands high transient peak currents that a standard 5V USB power bank or AA batteries simply cannot provide. *(Note: The battery's ground line must be tied to the ESP32's ground plane for the ESC to read the PWM control signal accurately).*

### 4. Motor Driver: 30A Electronic Speed Controller (ESC)
<img width="554" alt="30A ESC" src="https://github.com/user-attachments/assets/b282b8f1-fdf9-494b-a734-9a9f6d7e8e7e" />

Microcontrollers cannot drive brushless motors directly. The 30A ESC acts as the heavy-duty intermediary. It receives the low-voltage 50Hz PWM control signal from the ESP32 (via GPIO 27) and converts the high-current DC power from the LiPo battery into the precise 3-phase AC power required to drive the motor smoothly.

### 5. Wake-Up Gatekeeper: SW-420 Vibration Sensor
<img width="500" alt="SW-420 Sensor" src="https://github.com/user-attachments/assets/5723a54e-ee75-4506-98d5-b5fb37aed61c" />

To conserve power and system processing overhead, the SW-420 acts as a hardware-level trigger. It is a digital mechanical switch that detects gross physical movement. If the hand is resting entirely still, this sensor keeps the system locked in `IDLE` mode. Once movement is detected, it wakes up the main accelerometer to begin analyzing the tremor.

### 6. Primary Measurement: ADXL335 Triple-Axis Accelerometer
<img width="554" alt="ADXL335 Accelerometer" src="https://github.com/user-attachments/assets/f21ccc72-f3e9-44a6-9679-f066a4f459db" />

The ADXL335 is a highly sensitive, low-noise analog sensor that measures acceleration across the X, Y, and Z planes. It continuously feeds raw analog voltage waves to the ESP32, which processes these physical waves to calculate whether the hand is experiencing a true pathological tremor (4-12 Hz) or just normal, intentional movement.

## 🔌 Circuit Diagram & Electrical Architecture

To ensure precise high-speed motor response and clean analog signal processing, the electrical system splits logic processing from heavy current actuation. Below is the complete circuit schematic and wiring matrix for the Gyro Glove.

### 🗺️ System Circuit Schematic
 <img width="1600" height="1078" alt="Final report pdf-image-025" src="https://github.com/user-attachments/assets/43e17997-2ecf-40bc-93ab-17adb63099e3" />


---

### 📋 Complete Wiring Matrix & Pin Definition

The physical interconnects between the Espressif ESP32 DevKit V1 and system modules are defined in the master connection schedule below:

| Source Component | Physical Pin | Target Board | Target GPIO Pin | Signal Profile | Functional Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **SW-420 Sensor** | VCC | ESP32 | Vin / 5V | Power (DC) | 5V Main Logic Supply Rail |
| **SW-420 Sensor** | GND | ESP32 | GND | Ground | Logic Ground Reference |
| **SW-420 Sensor** | DO (Digital Out) | ESP32 | **GPIO 4** | Digital Input | Interrupt-style mechanical wakeup switch |
| **ADXL335 Module** | VCC | ESP32 | 3V3 | Power (DC) | 3.3V Regulated Analog Supply Rail |
| **ADXL335 Module** | GND | ESP32 | GND | Ground | Clean Analog Ground Shield |
| **ADXL335 Module** | X-Axis Out | ESP32 | **GPIO 34** | Analog Input | Dynamic Acceleration Waveform (X-Plane) |
| **ADXL335 Module** | Y-Axis Out | ESP32 | **GPIO 35** | Analog Input | Dynamic Acceleration Waveform (Y-Plane) |
| **ADXL335 Module** | Z-Axis Out | ESP32 | **GPIO 32** | Analog Input | Dynamic Acceleration Waveform (Z-Plane) |
| **30A ESC** | PWM Signal (White) | ESP32 | **GPIO 27** | PWM Output | 50Hz Asynchronous Motor Speed Control |
| **30A ESC** | Signal Ground (Black)| ESP32 | GND | Ground | Common Reference Ground Plane |
| **30A ESC** | Power Input (+) | LiPo Battery | Positive (+) | High Current | Direct Raw Unregulated Battery Power |
| **30A ESC** | Power Input (-) | LiPo Battery | Negative (-) | High Current | Heavy Drivetrain Return Path |
| **BLDC Motor** | Phase Wires (A, B, C)| 30A ESC | Motor Output | 3-Phase AC | Switched Brushless Commutation Drive |

---

### ⚡ Power Isolation & Distribution Strategy

Operating a high-RPM Brushless DC motor adjacent to ultra-sensitive analog measurement sensors introduces profound electrical challenges, specifically high-frequency electromagnetic interference (EMI) and massive voltage sags. The system controls this using a dual-rail isolation layout:

1. **The Logic Rail (Low Current):** The ESP32, SW-420, and ADXL335 operate on a low-noise logic framework. The ADXL335 is explicitly tied to the ESP32’s onboard 3.3V low-drop regulator to shield the analog-to-digital converter channels from electrical transients.
2. **The Actuation Rail (High Current):** The Electronic Speed Controller (ESC) and Brushless Motor are powered directly by the high-discharge Lithium-Polymer (LiPo) battery. This keeps high-frequency switching noise from corrupting the sensor registers.

> ### ⚠️ Critical Assembly Warning: The Common Ground Rule
> For the ESP32 to command the motor, the black signal wire coming from the ESC **must** be tied directly into one of the `GND` pins on the ESP32 board alongside the battery's negative terminal. 
>
> Without this shared reference path, the 50Hz PWM signal leaving GPIO 27 will floatingly drift. This breaks communication, preventing the ESC from arming and causing the motor to enter an infinite emergency safeguard beeping loop.

---

### 🔍 Signal Path Diagnostics
* **Input Path:** Raw physical hand tremors are parsed as real-time voltage variations by the ADXL335, ranging from 0V to 3.3V, which are processed inside the ESP32's 12-bit successive-approximation register (SAR) Analog-to-Digital Converter.
* **Output Path:** The controller converts processed tremor metrics into precise hardware PWM signals generated at a frequency of 50Hz across a 14-bit resolution map to govern the structural speed profiles of the stabilizing flywheel smoothly.

## 🔄 System Workflow & Control Firmware Logic

The Gyro Glove operates on a highly optimized, non-blocking asynchronous **Finite State Machine (FSM)** managed by the ESP32. To conserve energy and prevent processor overhead, the firmware utilizes a dual-sensor "Gatekeeper" design pattern that isolates low-power tracking from power-intensive stabilization.

Below is the detailed step-by-step technical execution flow of the system:

```text
    [ STATE: IDLE ] 
           │
           ▼ (Vibration Detected by SW-420)
    [ STATE: SAMPLING & ANALYSIS ] ───► (If Frequency < 4Hz or > 12Hz) ──┐
           │                                                              │
           ▼ (Pathological Tremor Confirmed: 4–12 Hz)                     │
    [ STATE: MOTOR_ON (Active Suppression) ]                              │
           │                                                              │
           ▼ (Run Duration Timer Expires)                                 │
    [ STATE: COOLDOWN ] ◄─────────────────────────────────────────────────┘
           │
           ▼ (Cooldown Timer Expires)
     (Return to IDLE)
```
---

### 📑 Comprehensive Operational States Breakdown

#### 1. State 1: IDLE (Gatekeeper Monitoring Mode)
* **Action:** The firmware locks the processor loop into a low-overhead monitoring state. The **SW-420 Vibration Sensor** acts as the digital gatekeeper.
* **Logic:** The ESP32 continuously polls the digital output (`GPIO 4`) of the SW-420. If the user's hand is relaxed, stationary, or making slow intentional movements, the sensor remains untriggered.
* **Output:** The motor duty cycle is pinned to `STOP_DUTY`, keeping the BLDC motor completely powered off to eliminate background power draw and unnecessary mechanical wear.

#### 2. State 2: SAMPLING & ANALYSIS (Tremor Frequency Extraction)
* **Action:** Immediate wake-up state triggered when the SW-420 registers physical oscillations. The **ADXL335 Accelerometer** is engaged to capture high-fidelity physical data.
* **Logic:** The ESP32's 12-bit SAR ADC executes a high-speed continuous burst-sampling window, gathering a batch of 100 raw analog readings across the X, Y, and Z planes. 
* **Algorithmic Processing:** * The firmware calculates the moving arithmetic average across all channels to isolate and eliminate the static effects of gravity vector offsets.
  * It passes the filtered acceleration waveforms through a custom **Hysteresis Zero-Crossing Counting Algorithm**. By establishing a tight noise-threshold boundary ($\pm100$ raw ADC ticks), the controller filters out high-frequency muscle twitches and signal jitter.
  * The net zero-crossings are mathematically converted into a precise physical frequency value in Hertz ($Hz$).

#### 3. State 3: MOTOR_ON (Adaptive Inertial Counter-Torque)
* **Action:** Active mechanical mitigation. If the computed frequency falls squarely within the pathological tremor window (**4 Hz to 12 Hz**), the FSM transitions to active suppression.
* **Logic & Execution Profiles:** The system adaptively maps the calculated frequency to specific structural runtime and cooldown profiles to match tremor severity:
  * **Mild to Moderate Tremors (4.0 Hz to < 8.0 Hz):** The system spools up the BLDC motor to its calculated stabilization RPM. It holds this speed for a strict execution runtime of **5,000 milliseconds (5 seconds)**.
  * **Severe Tremors (8.0 Hz to 12.0 Hz):** The motor is driven to an ultra-high RPM tier. It maintains this intense stabilization window for an extended runtime of **7,000 milliseconds (7 seconds)** to fully neutralize severe oscillatory force.
* **Suppression Mechanism:** The high-speed rotation of the heavy CNC-machined brass flywheel develops high angular momentum. Any sudden, involuntary tremor attempting to shift the motor's axis of rotation encounters immediate, perpendicular mechanical gyroscopic resistance, dampening the tremor instantly.

#### 4. State 4: COOLDOWN (Thermal & Power Management)
* **Action:** Safe mechanical deceleration and system breathing room.
* **Logic:** Once the active runtime (`runDuration`) timer expires, the ESP32 commands the ESC to decelerate the motor. The system enters a forced, non-blocking cooldown state to prevent high-RPM component overheating, shield the battery from sustained deep-discharge sags, and prevent patient muscle habituation:
  * Following a **Mild/Moderate** suppression block, a **7,000 millisecond (7 seconds)** cooldown is enforced.
  * Following a **Severe** suppression block, a longer **14,000 millisecond (14 seconds)** cooldown is enforced to let the drivetrain dissipate heat safely.
* **Loop Reset:** The entire system relies strictly on non-blocking `millis()` timers rather than blocking `delay()` functions. This allows the telemetry loop to continuously function. Once the cooldown timer expires, the FSM smoothly cycles back to State 1 (`IDLE`) to await the next gatekeeper trigger.

---

### 📊 Algorithmic Parameters Quick Reference

| Detected Frequency ($Hz$) | Tremor Classification | Active Motor Status | Active Run Window | Required Cooldown |
| :--- | :--- | :--- | :--- | :--- |
| **< 4.0 Hz** | Intentional / Macro Movement | `DISABLED` (Idle) | 0 ms | 0 ms |
| **4.0 Hz – 7.9 Hz** | Mild / Moderate Pathological Tremor | `ENABLED` (Stabilizing) | **5,000 ms (5s)** | **7,000 ms (7s)** |
| **8.0 Hz – 12.0 Hz** | Severe Pathological Tremor | `ENABLED` (Max Suppression) | **7,000 ms (7s)** | **14,000 ms (14s)** |
| **> 12.0 Hz** | High Frequency Mechanical Noise | `DISABLED` (Idle) | 0 ms | 0 ms |
