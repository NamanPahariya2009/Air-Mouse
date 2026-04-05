# 🖱️ ESP32 BLE Air Mouse

Transform your ESP32 and MPU6050 into a sleek, motion-controlled Bluetooth mouse. This project uses the built-in Bluetooth Low Energy (BLE) capabilities of the ESP32 to act as a standard HID (Human Interface Device), allowing you to control your computer cursor simply by moving your hand in the air.

![Air Mouse Circuit](Air_Mouse.png)

## ✨ Features

- **🎮 Motion Control**: High-precision cursor movement using the MPU6050 gyroscope.
- **⚡ Dynamic Sensitivity**: Cursor speed scales naturally with your hand movement (faster movement = faster cruise).
- **🎯 Deadzone Filtering**: Intelligent thresholding to prevent cursor "creep" or jitter when stationary.
- **🖱️ Full Mouse Functionality**:
  - Left & Right Click buttons.
  - Dedicated Scroll Up & Scroll Down buttons.
- **📡 Wireless & Driverless**: Uses standard BLE HID profiles. No software installation required on the host computer.
- **🔋 Energy Efficient**: Built using Bluetooth Low Energy for longer battery life in portable setups.

## 🛠️ How to Build Your Own Air Mouse

Follow these step-by-step instructions to assemble and start using your ESP32 Air Mouse.

### Step 1: Gathering Materials
You will need:
- **ESP32 Development Board** (30-pin or 38-pin version works fine)
- **MPU6050 Sensor Module** (Accelerometer + Gyroscope)
- **4x Push Buttons** (for Left, Right, and Scroll Up/Down)
- **A Small Breadboard** or a **Prototype Board** (if soldering)
- **Jumper Wires**

### Step 2: Wiring the Circuit
Looking at the [Air_Mouse.png](Air_Mouse.png) diagram, connect the components as follows:

1. **Connect the MPU6050**:
   - VCC -> ESP32 **3V3**
   - GND -> ESP32 **GND**
   - SCL -> ESP32 **GPIO 22**
   - SDA -> ESP32 **GPIO 21**
2. **Connect the Click Buttons**:
   - Connect one side of all buttons to **GND**.
   - Connect the other side of each button to its specific GPIO:
     - **Left Click** -> GPIO **18**
     - **Right Click** -> GPIO **19**
     - **Scroll Up** -> GPIO **14**
     - **Scroll Down** -> GPIO **27**

### Step 3: Setting Up the Software
1. **Install Arduino IDE**: [Download it here](https://www.arduino.cc/en/software).
2. **Add ESP32 Support**: 
   - Go to `File > Preferences`.
   - In "Additional Boards Manager URLs," paste: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Go to `Tools > Board > Boards Manager`, search for **"esp32"**, and install the package by Espressif Systems.
3. **Install Libraries**:
   - Go to `Sketch > Include Library > Manage Libraries`.
   - Search for and install **"ESP32 BLE Mouse"** (by T-vK).
   - Search for and install **"Adafruit MPU6050"** (this will ask you to install "Adafruit Unified Sensor" and "Adafruit BusIO"—say **Yes to all**).

### Step 4: Flashing the Code
1. Connect your ESP32 to your computer via USB.
2. Open `Air_Mouse.ino` in your Arduino IDE.
3. Go to `Tools > Board` and select your board (usually **"DOIT ESP32 DEVKIT V1"**).
4. Go to `Tools > Port` and select the COM port of your ESP32.
5. Click the **Upload** (➡️) button.

### Step 5: Pairing and Using
1. Once the upload is successful, your ESP32 will start broadcasting its Bluetooth signal.
2. Open your computer/phone's Bluetooth settings and look for **"ESP32 BLE Mouse"**.
3. Pair with it.
4. Move your Air Mouse! Tilt it forward/backward to move vertically, and pan it side-to-side to move horizontally. Use your buttons for clicks and scrolling.

---

## 🧪 Technical Logic

The project uses the **Gyroscope (degrees/sec)** instead of the Accelerometer to map movement. This provides a "laser pointer" feel.
- **Yaw (Z-axis)** -> Horizontal Movement (X)
- **Pitch (Y-axis)** -> Vertical Movement (Y)

```cpp
// Dynamic sensitivity formula
float dynamicSensitivity = baseSensitivity * (1 + speed * 0.2);
```

## 📜 License
This project is open-source and available under the [MIT License](LICENSE).

---
*Developed by [Naman Pahariya](https://github.com/NamanPahariya) with ❤️ for the Microcontroller community.*
