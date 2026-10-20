#include <BleMouse.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// BLE Mouse & Sensor instances
BleMouse bleMouse("ESP32 BLE Mouse", "Naman Pahariya", 100);
Adafruit_MPU6050 mpu;

// --- Pin Definitions ---
#define LEFT_CLICK_PIN  18
#define RIGHT_CLICK_PIN 19
#define SCROLL_UP_PIN   14
#define SCROLL_DOWN_PIN 27

// --- Debounce Helper Structure ---
struct DebouncedButton {
  uint8_t pin;
  bool state;
  bool lastReading;
  unsigned long lastDebounceTime;

  void init(uint8_t p) {
    pin = p;
    state = false;
    lastReading = false;
    lastDebounceTime = 0;
    pinMode(pin, INPUT_PULLUP);
  }

  bool update() {
    bool reading = (digitalRead(pin) == LOW); // Active LOW
    if (reading != lastReading) {
      lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > 20) { // 20ms debounce
      state = reading;
    }
    lastReading = reading;
    return state;
  }
};

DebouncedButton btnLeft;
DebouncedButton btnRight;
DebouncedButton btnScrollUp;
DebouncedButton btnScrollDown;

// --- Gyro Offsets & Calibration ---
float gyroOffsetX = 0.0;
float gyroOffsetY = 0.0;
float gyroOffsetZ = 0.0;

// --- DPI Settings ---
enum DPIMode { DPI_LOW, DPI_MEDIUM, DPI_HIGH };
DPIMode currentDPI = DPI_MEDIUM;
float baseSensitivity = 10.0;
float threshold       = 0.015; // Deadzone threshold

// --- Smoothing (EMA Low-Pass Filter) ---
float smoothX = 0.0;
float smoothY = 0.0;
const float alpha = 0.65; // Smoothing factor (0.0 to 1.0)

// --- Timers & State Flags ---
unsigned long stabilizeUntil    = 0;
const unsigned long STABILIZE_MS = 75; // Freeze movement briefly on click to prevent hand tremor deflection

unsigned long lastScrollTime    = 0;
const unsigned long SCROLL_INTERVAL = 130; // Non-blocking scroll rate

unsigned long lastActivityTime  = 0;
const unsigned long INACTIVITY_TIMEOUT = 120000; // 2 minutes

bool wasLeftDown       = false;
bool wasRightDown      = false;
bool middleClickActive = false;
bool clutchActive      = false;
bool dpiChangeHandled  = false;

// --- Gyroscope Auto-Calibration ---
void calibrateGyro() {
  Serial.println("Calibrating Gyroscope... Keep the device still!");
  const int samples = 200;
  float sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sumX += g.gyro.x;
    sumY += g.gyro.y;
    sumZ += g.gyro.z;
    delay(5);
  }

  gyroOffsetX = sumX / samples;
  gyroOffsetY = sumY / samples;
  gyroOffsetZ = sumZ / samples;

  Serial.println("Calibration complete!");
  Serial.print("Offsets -> X: "); Serial.print(gyroOffsetX, 4);
  Serial.print(" | Y: "); Serial.print(gyroOffsetY, 4);
  Serial.print(" | Z: "); Serial.println(gyroOffsetZ, 4);
}

// --- DPI Switcher ---
void cycleDPI() {
  if (currentDPI == DPI_LOW) {
    currentDPI = DPI_MEDIUM;
    baseSensitivity = 10.0;
    Serial.println("[DPI] Medium (Standard - 10.0)");
  } else if (currentDPI == DPI_MEDIUM) {
    currentDPI = DPI_HIGH;
    baseSensitivity = 16.0;
    Serial.println("[DPI] High (Fast - 16.0)");
  } else {
    currentDPI = DPI_LOW;
    baseSensitivity = 5.0;
    Serial.println("[DPI] Low (Precision / Sniper - 5.0)");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Enhanced ESP32 BLE Air Mouse...");

  // Start BLE
  bleMouse.begin();

  // Start MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found, check wiring!");
    while (1) delay(10);
  }

  // Sensor configurations
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize Debounced Buttons
  btnLeft.init(LEFT_CLICK_PIN);
  btnRight.init(RIGHT_CLICK_PIN);
  btnScrollUp.init(SCROLL_UP_PIN);
  btnScrollDown.init(SCROLL_DOWN_PIN);

  // Calibrate Gyro to eliminate zero-motion drift
  calibrateGyro();

  lastActivityTime = millis();
  Serial.println("Setup complete, ready for BLE connection!");
}

void loop() {
  if (bleMouse.isConnected()) {
    // 1. Update Debounced Button States
    bool leftState       = btnLeft.update();
    bool rightState      = btnRight.update();
    bool scrollUpState   = btnScrollUp.update();
    bool scrollDownState = btnScrollDown.update();

    if (leftState || rightState || scrollUpState || scrollDownState) {
      lastActivityTime = millis();
    }

    // --- COMBO 1: Clutch Mode (Scroll Up + Scroll Down held together) ---
    // Pauses cursor tracking so you can reposition your hand without moving the pointer
    if (scrollUpState && scrollDownState) {
      clutchActive = true;
    } else {
      clutchActive = false;
    }

    // --- COMBO 2: DPI Toggle (Hold Scroll Up + Click Left) ---
    if (scrollUpState && !scrollDownState) {
      if (leftState && !wasLeftDown) {
        cycleDPI();
        dpiChangeHandled = true;
      }
    }
    if (!leftState) {
      dpiChangeHandled = false;
    }

    // --- COMBO 3: Middle Click (Left + Right Click together) ---
    if (leftState && rightState && !dpiChangeHandled) {
      if (!middleClickActive) {
        bleMouse.press(MOUSE_MIDDLE);
        middleClickActive = true;
        bleMouse.release(MOUSE_LEFT);
        bleMouse.release(MOUSE_RIGHT);
      }
    } else {
      if (middleClickActive) {
        bleMouse.release(MOUSE_MIDDLE);
        middleClickActive = false;
      }
    }

    // --- Standard Clicks (with Click Stabilization) ---
    if (!middleClickActive && !dpiChangeHandled) {
      // Left Click
      if (leftState) {
        if (!wasLeftDown) {
          stabilizeUntil = millis() + STABILIZE_MS; // Freeze cursor jitter on click
        }
        bleMouse.press(MOUSE_LEFT);
      } else {
        bleMouse.release(MOUSE_LEFT);
      }

      // Right Click
      if (rightState) {
        if (!wasRightDown) {
          stabilizeUntil = millis() + STABILIZE_MS; // Freeze cursor jitter on click
        }
        bleMouse.press(MOUSE_RIGHT);
      } else {
        bleMouse.release(MOUSE_RIGHT);
      }
    }

    wasLeftDown  = leftState;
    wasRightDown = rightState;

    // --- Non-Blocking Scrolling ---
    if (!clutchActive && !dpiChangeHandled) {
      if (scrollUpState && !leftState && (millis() - lastScrollTime >= SCROLL_INTERVAL)) {
        bleMouse.move(0, 0, 1);
        lastScrollTime = millis();
      } else if (scrollDownState && (millis() - lastScrollTime >= SCROLL_INTERVAL)) {
        bleMouse.move(0, 0, -1);
        lastScrollTime = millis();
      }
    }

    // --- MPU6050 Gyroscope Cursor Tracking ---
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Subtract calibrated gyro offsets
    float gyroY = g.gyro.y - gyroOffsetY; // Pitch -> Vertical
    float gyroZ = g.gyro.z - gyroOffsetZ; // Yaw   -> Horizontal

    float moveX = gyroZ;
    float moveY = gyroY;

    // Deadzone filter
    if (fabs(moveX) < threshold) moveX = 0;
    if (fabs(moveY) < threshold) moveY = 0;

    if (moveX != 0 || moveY != 0) {
      lastActivityTime = millis();
    }

    // Dynamic sensitivity scaling (faster hand movement = faster cursor)
    float speed = sqrt(moveX * moveX + moveY * moveY);
    float dynamicSensitivity = baseSensitivity * (1.0 + speed * 0.2);

    moveX *= dynamicSensitivity;
    moveY *= (dynamicSensitivity + 2.0);

    // Exponential Moving Average (EMA) low-pass smoothing
    smoothX = (alpha * moveX) + ((1.0 - alpha) * smoothX);
    smoothY = (alpha * moveY) + ((1.0 - alpha) * smoothY);

    // Suppress cursor if Stabilizing, in Clutch mode, or changing DPI
    if (millis() < stabilizeUntil || clutchActive || dpiChangeHandled) {
      smoothX = 0;
      smoothY = 0;
    }

    // Send cursor movement
    int finalX = (int)-smoothX; // Invert horizontal for natural pointer feeling
    int finalY = (int)smoothY;

    if (finalX != 0 || finalY != 0) {
      bleMouse.move(finalX, finalY);
    }
  }

  // Power save / polling rate adjuster:
  // If no motion or clicks for 2 minutes, slow down polling to save power
  if (millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    delay(40); // Idle standby mode
  } else {
    delay(6);  // High responsiveness (~125Hz report rate)
  }
}
