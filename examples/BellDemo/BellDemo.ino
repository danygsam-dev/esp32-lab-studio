/*
  ESP32 Lab Studio starter sketch
  Board target: ESP32-S3 Dev Module

  GPIO assignments are examples. Verify your actual board pinout before
  connecting hardware. This sketch demonstrates Wi-Fi station connection,
  a relay output, a push-button input and a status LED.

  Relay is configured ACTIVE-LOW by default. Change RELAY_ACTIVE_LOW if
  your relay module is active-high.
*/
#include <WiFi.h>
#include <time.h>

static constexpr int RELAY_PIN = 5;
static constexpr int BUTTON_PIN = 4;
static constexpr int LED_PIN = 2;
static constexpr bool RELAY_ACTIVE_LOW = true;

const char* WIFI_SSID = "BellLab";
const char* WIFI_PASSWORD = "belllab123";

bool relayOn = false;
unsigned long lastStatus = 0;

void setRelay(bool on) {
  relayOn = on;
  const int level = RELAY_ACTIVE_LOW
      ? (on ? LOW : HIGH)
      : (on ? HIGH : LOW);
  digitalWrite(RELAY_PIN, level);
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  Serial.printf("[RELAY] %s\n", on ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setRelay(false);

  Serial.println("\n[BOOT] ESP32 Lab Studio Bell Demo");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  const unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected. IP=%s\n", WiFi.localIP().toString().c_str());
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("\n[WiFi] Not connected; continuing offline.");
  }

  Serial.println("[READY] Press the simulated GPIO button (GPIO4) low to toggle relay.");
}

void loop() {
  static int previousButton = HIGH;
  const int currentButton = digitalRead(BUTTON_PIN);

  if (previousButton == HIGH && currentButton == LOW) {
    setRelay(!relayOn);
  }
  previousButton = currentButton;

  if (millis() - lastStatus >= 5000) {
    lastStatus = millis();
    Serial.printf("[STATUS] relay=%s wifi=%s\n",
                  relayOn ? "ON" : "OFF",
                  WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    struct tm timeInfo;
    if (getLocalTime(&timeInfo, 10)) {
      char buffer[32];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
      Serial.printf("[CLOCK] %s UTC\n", buffer);
    }
  }
  delay(10);
}
