/* ==============================================================================
   SMART IRRIGATION SYSTEM - 3-LAYER IoT ARCHITECTURE
   
   Author: IoT Agriculture Team
   Version: 2.0
   Date: 1 Nov 2025
   
   ARCHITECTURE:
   1. PERCEPTION LAYER    : Sensors (DHT11, DS18B20, Soil, BH1750) + Actuator (Pump)
   2. SECURITY LAYER      : TLS/SSL Encryption with CA Certificate
   3. NETWORK LAYER       : MQTT (ThingsBoard) + HTTPS (Telegram)
   4. APPLICATION LAYER   : Dashboard, Alerts, Remote Control, Data Logging
   
   FEATURES:
   - Multi-sensor monitoring (temperature, humidity, soil moisture, light)
   - Hysteresis-based irrigation control
   - TLS/SSL secure MQTT communication
   - JSON-RPC remote control via ThingsBoard
   - Telegram bot alerts and notifications
   - Google Sheets data logging
   - Automatic and manual operation modes
   - Evapotranspiration estimation
   - Edge computing and statistics
============================================================================== */
/* ==============================================================================
   SMART IRRIGATION SYSTEM - 3-LAYER IoT ARCHITECTURE (Modified)
   Additions:
   1) Pump max runtime & cooldown protection
   2) Explicit Telegram critical alerts
   3) I2C LCD display with "ERR" for unread sensors

   Author: IoT Agriculture Team (modified)
   Date: 5 Nov 2025
============================================================================== */

#define MQTT_MAX_PACKET_SIZE 1024

// ========== LIBRARIES ==========
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <BH1750.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>   // I2C LCD 16x2

// ========== CONFIGURATION ==========
// WiFi Credentials
const char* WIFI_SSID = "kesz";
const char* WIFI_PASS = "kaz3912ry!";

// ThingsBoard Configuration (MQTT over TLS)
const char* THINGSBOARD_SERVER = "mqtt.thingsboard.cloud";
const uint16_t THINGSBOARD_PORT = 8883;  // Secure MQTT port
const char* THINGSBOARD_TOKEN = "lEJVww7vgJl8HtJJQvUJ";

// Telegram Bot Configuration
const char* TELEGRAM_BOT_TOKEN = "8244334452:AAHJ1aNiydrob-Bo6pj_NOhl5_P60rMH3Ks";
const long TELEGRAM_CHAT_ID = 2143545396;

// Google Apps Script for Logging
const char* GAS_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbzv_x3YAFVsPCCtrNjIYChivPaSLfvgnZktrOayPDcBBGJnzbpnlmf1SMLN54Ugc2jLfw/exec";

// Security: Use TLS certificate validation
const bool USE_SECURE_MQTT = true;

// Root CA certificate
const char ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIID0zCCArugAwIBAgIQVmcdBOpPmUxvEIFHWdJ1lDANBgkqhkiG9w0BAQwFADB7
MQswCQYDVQQGEwJHQjEbMBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYD
VQQHDAdTYWxmb3JkMRowGAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UE
AwwYQUFBIENlcnRpZmljYXRlIFNlcnZpY2VzMB4XDTE5MDMxMjAwMDAwMFoXDTI4
MTIzMTIzNTk1OVowgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpOZXcgSmVyc2V5
MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhlIFVTRVJUUlVTVCBO
ZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgRUNDIENlcnRpZmljYXRpb24gQXV0
aG9yaXR5MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEGqxUWqn5aCPnetUkb1PGWthL
q8bVttHmc3Gu3ZzWDGH926CJA7gFFOxXzu5dP+Ihs8731Ip54KODfi2X0GHE8Znc
JZFjq38wo7Rw4sehM5zzvy5cU7Ffs30yf4o043l5o4HyMIHvMB8GA1UdIwQYMBaA
FKARCiM+lvEH7OKvKe+CpX/QMKS0MB0GA1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1
xmNjmjAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zARBgNVHSAECjAI
MAYGBFUdIAAwQwYDVR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5j
b20vQUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNAYIKwYBBQUHAQEEKDAmMCQG
CCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wDQYJKoZIhvcNAQEM
BQADggEBABns652JLCALBIAdGN5CmXKZFjK9Dpx1WywV4ilAbe7/ctvbq5AfjJXy
ij0IckKJUAfiORVsAYfZFhr1wHUrxeZWEQff2Ji8fJ8ZOd+LygBkc7xGEJuTI42+
FsMuCIKchjN0djsoTI0DQoWz4rIjQtUfenVqGtF8qmchxDM6OW1TyaLtYiKou+JV
bJlsQ2uRl9EMC5MCHdK8aXdJ5htN978UeAOwproLtOGFfy/cQjutdAFI3tZs4RmY
CV4Ks2dH/hzg1cEo70qLRDEmBDeNiXQ2Lu+lIg+DdEmSx/cQwgwp+7e9un/jX9Wf
8qn0dNW44bOwgeThpWOjzOoEeJBuv/c=
-----END CERTIFICATE-----
)EOF";

// ========== TIMING CONFIGURATION ==========
const unsigned long SENSOR_READ_INTERVAL_MS = 10000;    // 10 seconds
const unsigned long TELEMETRY_INTERVAL_MS = 30000;      // 30 seconds
const unsigned long TELEGRAM_INTERVAL_MS = 300000;      // 5 minutes
const unsigned long GOOGLE_LOG_INTERVAL_MS = 300000;    // 5 minutes

// ========== IRRIGATION THRESHOLDS ==========
const float SOIL_CRITICAL_LOW = 30.0;      // Turn pump ON
const float SOIL_WARNING_HIGH = 45.0;      // Turn pump OFF
const float SOIL_OPTIMAL_MIN = 40.0;
const float SOIL_OPTIMAL_MAX = 60.0;

// Extra critical alert threshold
const float SOIL_CRITICAL_ALERT_THRESHOLD = 20.0; // If soil < 20% -> critical alert

// ========== PUMP PROTECTION CONFIG ==========
const unsigned long PUMP_MAX_RUNTIME_MS = 10UL * 60UL * 1000UL; // 10 minutes max continuous runtime (tweakable)
const unsigned long PUMP_MIN_OFF_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 minutes minimum off between cycles

// ========== HARDWARE PIN DEFINITIONS ==========
#define DHTPIN 5                    // DHT11 sensor pin
#define DHTTYPE DHT11               // DHT sensor type
const int SOIL_SENSOR_PIN = 34;     // Analog input for capacitive soil sensor
const int ONE_WIRE_BUS = 4;        // DS18B20 soil temperature sensor
const int PUMP_PIN = 14;            // Water pump relay control
const bool PUMP_ACTIVE_HIGH = false; // Relay logic level

// ========== SENSOR CALIBRATION ==========
const int SOIL_DRY_VALUE = 3000;   // ADC value in dry air
const int SOIL_WET_VALUE = 1600;   // ADC value in water

// ========== LCD (I2C 16x2) ==========
LiquidCrystal_I2C lcd(0x27, 16, 2); // common address 0x27; change if needed

// ========== GLOBAL OBJECTS ==========
WiFiClientSecure wifiClientSecure;
PubSubClient mqttClient(wifiClientSecure);

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18Sensor(&oneWire);
BH1750 lightMeter;

// ========== SYSTEM STATE VARIABLES ==========
bool pumpState = false;           // Current pump status
bool autoMode = true;             // Automatic control mode
bool alertSent = false;           // Flag to prevent alert spam
String lastAlertType = "";        // Last alert type sent

// Sensor data structure
struct SensorData {
  float soilMoisture;
  float airTemperature;
  float airHumidity;
  float soilTemperature;
  float lightLevel;
  unsigned long timestamp;
};

SensorData currentData;
SensorData lastData;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastTelemetryPublish = 0;
unsigned long lastTelegramAlert = 0;
unsigned long lastGoogleLog = 0;
unsigned long pumpStartTime = 0;
unsigned long totalPumpRuntime = 0;
unsigned long lastPumpStopTime = 0; // time when pump last stopped

// Statistics
int telemetryCount = 0;
int errorCount = 0;

// Forward declarations
void connectWiFi();
void connectMQTT();
void publishTelemetry();
void logToGoogleSheets();
void sendTelegramAlert(String message);
String urlencode(String str);
void irrigationControl();
bool readAllSensors();
void setPump(bool state);
float readSoilMoisture();
float calculateET();
void initSecurity();
void sendStatusReport();
void updateLCD();
void checkPumpRuntimeSafety();

// ========== LAYER 1: PERCEPTION LAYER FUNCTIONS ==========
float readSoilMoisture() {
  int rawValue = analogRead(SOIL_SENSOR_PIN);
  float percentage = 100.0 - ((rawValue - SOIL_WET_VALUE) /
                     (float)(SOIL_DRY_VALUE - SOIL_WET_VALUE) * 100.0);
  percentage = constrain(percentage, 0, 100);
  Serial.printf("  [SOIL] Raw: %d, Moisture: %.1f%%\n", rawValue, percentage);
  return percentage;
}

bool readAllSensors() {
  Serial.println("\n========== READING SENSORS ==========");
  currentData.airTemperature = dht.readTemperature();
  currentData.airHumidity = dht.readHumidity();
  ds18Sensor.requestTemperatures();
  currentData.soilTemperature = ds18Sensor.getTempCByIndex(0);
  currentData.lightLevel = lightMeter.readLightLevel();
  currentData.soilMoisture = readSoilMoisture();
  currentData.timestamp = millis();
  bool valid = true;
  // Validate individually; allow reporting of partial failures
  if (isnan(currentData.airTemperature) || isnan(currentData.airHumidity)) {
    Serial.println("  [ERROR] DHT reading invalid!");
    valid = false;
    errorCount++;
  }
  if (currentData.soilTemperature == DEVICE_DISCONNECTED_C) {
    Serial.println("  [ERROR] DS18B20 disconnected!");
    valid = false;
    errorCount++;
  }
  if (currentData.lightLevel < 0) {
    Serial.println("  [ERROR] BH1750 reading invalid!");
    valid = false;
    errorCount++;
  }
  // Soil moisture analog read always returns a value but we can sanity-check
  if (currentData.soilMoisture < 0 || currentData.soilMoisture > 100) {
    Serial.println("  [ERROR] Soil moisture out of range!");
    valid = false;
    errorCount++;
  }

  Serial.print("  [AIR]   Temp: ");
  Serial.print(isnan(currentData.airTemperature) ? -999.0 : currentData.airTemperature, 1);
  Serial.print("°C, RH: ");
  Serial.print(isnan(currentData.airHumidity) ? -999.0 : currentData.airHumidity, 1);
  Serial.println("%");

  Serial.print("  [SOIL]  Temp: ");
  Serial.print(currentData.soilTemperature == DEVICE_DISCONNECTED_C ? -999.0 : currentData.soilTemperature, 1);
  Serial.print("°C, Moisture: ");
  Serial.print(currentData.soilMoisture, 1);
  Serial.println("%");

  Serial.print("  [LIGHT] ");
  Serial.print(currentData.lightLevel, 0);
  Serial.println(" lux");

  // update LCD even when some readings are invalid
  updateLCD();

  return valid;
}

void setPump(bool state) {
  if (pumpState == state) return;
  pumpState = state;
  digitalWrite(PUMP_PIN, PUMP_ACTIVE_HIGH ? (state ? HIGH : LOW) : (state ? LOW : HIGH));
  Serial.println("\n========== PUMP CONTROL ==========");
  Serial.printf("  Pump %s\n", state ? "ACTIVATED" : "DEACTIVATED");
  if (state) {
    pumpStartTime = millis();
  } else {
    if (pumpStartTime > 0) {
      totalPumpRuntime += (millis() - pumpStartTime);
      pumpStartTime = 0;
    }
    lastPumpStopTime = millis();
  }
  // update LCD so user sees pump state quickly
  updateLCD();
}

// Ensure pump is not abused and send alert if auto-shutdown occurs
void checkPumpRuntimeSafety() {
  if (pumpState && pumpStartTime > 0) {
    unsigned long runtime = millis() - pumpStartTime;
    if (runtime >= PUMP_MAX_RUNTIME_MS) {
      // Auto-shutdown
      setPump(false);
      sendTelegramAlert(String("⚠️ PUMP AUTO-SHUTDOWN\n\nPump ran for ") +
                        String(runtime / 1000) + " s (max allowed: " +
                        String(PUMP_MAX_RUNTIME_MS / 60000) + " min). " +
                        "Auto-shutdown to protect equipment.");
      lastAlertType = "pump_autoshutdown";
    }
  }
}

void irrigationControl() {
  if (!autoMode) return;
  float soilMoisture = currentData.soilMoisture;

  // If sensor reading is invalid, don't change pump state automatically
  if (isnan(currentData.airTemperature) || isnan(currentData.airHumidity) ||
      currentData.soilTemperature == DEVICE_DISCONNECTED_C) {
    Serial.println("  [AUTO] Some sensors invalid - skipping auto control");
    return;
  }

  // If soil critically dry enough to trigger pump
  if (!pumpState && soilMoisture < SOIL_CRITICAL_LOW) {
    // Respect minimum off cooldown
    if (millis() - lastPumpStopTime < PUMP_MIN_OFF_INTERVAL_MS) {
      Serial.println("  [AUTO] Pump cooldown active - delaying restart");
      return;
    }
    Serial.println("  [AUTO] Soil critically dry, activating pump");
    setPump(true);
    if (!alertSent || lastAlertType != "pump_on") {
      sendTelegramAlert("🚰 PUMP ACTIVATED\n\nSoil moisture critically low: " +
                       String(soilMoisture, 1) + "%\nIrrigation started automatically.");
      alertSent = true;
      lastAlertType = "pump_on";
    }
  }
  // Deactivate when moisture recovers past warning high
  else if (pumpState && soilMoisture > SOIL_WARNING_HIGH) {
    Serial.println("  [AUTO] Soil moisture sufficient, deactivating pump");
    setPump(false);
    if (!alertSent || lastAlertType != "pump_off") {
      sendTelegramAlert("✅ PUMP DEACTIVATED\n\nSoil moisture restored: " +
                       String(soilMoisture, 1) + "%\nIrrigation completed.");
      alertSent = true;
      lastAlertType = "pump_off";
    }
  }

  // Additional critical-level alert (even if pump already on/attempting to start)
  if (soilMoisture < SOIL_CRITICAL_ALERT_THRESHOLD) {
    if (lastAlertType != "soil_critical") {
      sendTelegramAlert("🚨 CRITICAL SOIL MOISTURE\n\nSoil moisture extremely low: " +
                       String(soilMoisture, 1) + "%\nImmediate attention required!");
      lastAlertType = "soil_critical";
      alertSent = true;
    }
  }
}

// ========== LAYER 2: SECURITY LAYER FUNCTIONS ==========
void initSecurity() {
  if (USE_SECURE_MQTT) {
    Serial.println("\n========== SECURITY LAYER ==========");
    Serial.println("  Setting up TLS/SSL with CA certificate...");
    wifiClientSecure.setCACert(ROOT_CA);
    Serial.println("  ✓ CA certificate loaded");
  } else {
    Serial.println("  ⚠ WARNING: Running in INSECURE mode!");
    wifiClientSecure.setInsecure();
  }
}

// ========== LAYER 3: NETWORK LAYER FUNCTIONS ==========
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("\n========== NETWORK LAYER ==========");
  Serial.print("  Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("  ✓ WiFi connected");
    Serial.print("  IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("  ✗ WiFi connection failed!");
    errorCount++;
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;
  Serial.println("\n========== RPC COMMAND RECEIVED ==========");
  Serial.println("  Topic: " + String(topic));
  Serial.println("  Payload: " + message);
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println("  ✗ JSON parsing failed: " + String(error.c_str()));
    return;
  }
  const char* method = doc["method"];
  if (!method) {
    Serial.println("  ✗ No method specified");
    return;
  }
  if (strcmp(method, "setPump") == 0) {
    bool value = doc["params"];
    Serial.printf("  → setPump(%s)\n", value ? "ON" : "OFF");
    if (!autoMode) {
      // Respect pump runtime safety even for manual commands
      if (value && (millis() - lastPumpStopTime < PUMP_MIN_OFF_INTERVAL_MS)) {
        Serial.println("  ⚠ Manual pump ON blocked by cooldown");
        sendTelegramAlert("⚠ Manual pump ON blocked: pump cooldown active.");
      } else {
        setPump(value);
        sendTelegramAlert("🎮 MANUAL CONTROL\n\nPump " + String(value ? "activated" : "deactivated") +
                         " via remote command.");
      }
    } else {
      Serial.println("  ⚠ Command ignored (auto mode active)");
    }
  }
  else if (strcmp(method, "setMode") == 0) {
    const char* mode = doc["params"];
    Serial.printf("  → setMode(%s)\n", mode);
    if (strcmp(mode, "auto") == 0) {
      autoMode = true;
      Serial.println("  ✓ Switched to AUTOMATIC mode");
      sendTelegramAlert("🤖 MODE CHANGED\n\nSwitched to automatic irrigation control.");
    }
    else if (strcmp(mode, "manual") == 0) {
      autoMode = false;
      Serial.println("  ✓ Switched to MANUAL mode");
      sendTelegramAlert("🎮 MODE CHANGED\n\nSwitched to manual control. Use remote commands to control pump.");
    }
  }
  else if (strcmp(method, "getStatus") == 0) {
    Serial.println("  → getStatus()");
    // telemetry will carry the status
  }
  else {
    Serial.println("  ✗ Unknown method: " + String(method));
  }
  String responseTopic = String("v1/devices/me/rpc/response/1");
  StaticJsonDocument<128> response;
  response["method"] = method;
  response["results"]["success"] = true;
  response["results"]["mode"] = autoMode ? "auto" : "manual";
  response["results"]["pump"] = pumpState;
  String responsePayload;
  serializeJson(response, responsePayload);
  mqttClient.publish(responseTopic.c_str(), responsePayload.c_str());
}

void connectMQTT() {
  if (mqttClient.connected()) return;
  connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.println("\n========== MQTT CONNECTION ==========");
  mqttClient.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  mqttClient.setCallback(mqttCallback);
  String clientId = "ESP32_Irrigation_" + WiFi.macAddress();
  clientId.replace(":", "");
  Serial.println("  Connecting to ThingsBoard...");
  Serial.println("  Server: " + String(THINGSBOARD_SERVER) + ":" + String(THINGSBOARD_PORT));
  if (mqttClient.connect(clientId.c_str(), THINGSBOARD_TOKEN, NULL)) {
    Serial.println("  ✓ MQTT connected successfully");
    String rpcTopic = "v1/devices/me/rpc/request/+";
    if (mqttClient.subscribe(rpcTopic.c_str())) {
      Serial.println("  ✓ Subscribed to RPC topic: " + rpcTopic);
    }
  } else {
    Serial.print("  ✗ MQTT connection failed, state: ");
    Serial.println(mqttClient.state());
    errorCount++;
  }
}

// ========== LAYER 4: APPLICATION LAYER FUNCTIONS ==========
void publishTelemetry() {
  if (!mqttClient.connected()) return;
  Serial.println("\n========== PUBLISHING TELEMETRY ==========");
  DynamicJsonDocument doc(1024);
  doc["soil_moisture"] = round(currentData.soilMoisture * 10) / 10.0;
  doc["air_temperature"] = round(currentData.airTemperature * 10) / 10.0;
  doc["air_humidity"] = round(currentData.airHumidity * 10) / 10.0;
  doc["soil_temperature"] = round(currentData.soilTemperature * 10) / 10.0;
  doc["light_level"] = round(currentData.lightLevel);
  doc["pump_state"] = pumpState;
  doc["auto_mode"] = autoMode;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["uptime_minutes"] = millis() / 60000;
  doc["telemetry_count"] = telemetryCount;
  doc["error_count"] = errorCount;
  doc["pump_runtime_minutes"] = totalPumpRuntime / 60000;
  doc["et_index"] = round(calculateET() * 100) / 100.0;
  if (currentData.soilMoisture < SOIL_CRITICAL_LOW) {
    doc["soil_status"] = "critical";
  } else if (currentData.soilMoisture < SOIL_OPTIMAL_MIN) {
    doc["soil_status"] = "low";
  } else if (currentData.soilMoisture > SOIL_OPTIMAL_MAX) {
    doc["soil_status"] = "high";
  } else {
    doc["soil_status"] = "optimal";
  }
  String payload;
  serializeJson(doc, payload);
  bool success = mqttClient.publish("v1/devices/me/telemetry", payload.c_str());
  if (success) {
    Serial.println("  ✓ Telemetry published successfully");
    Serial.println("  Size: " + String(payload.length()) + " bytes");
    telemetryCount++;
  } else {
    Serial.println("  ✗ Telemetry publish failed");
    errorCount++;
  }
}

void sendTelegramAlert(String message) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastTelegramAlert < 60000) {
    Serial.println("  ⚠ Telegram rate limited, skipping alert");
    return;
  }
  Serial.println("\n========== SENDING TELEGRAM ALERT ==========");
  Serial.println("  Message: " + message);
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) +
               "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
               "&text=" + urlencode(message);
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == 200) {
    Serial.println("  ✓ Telegram alert sent successfully");
    lastTelegramAlert = millis();
  } else {
    Serial.println("  ✗ Telegram alert failed, HTTP code: " + String(httpCode));
    errorCount++;
  }
  http.end();
}

String urlencode(String str) {
  String encoded = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += '+';
    } else if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      if ((uint8_t)c < 16) encoded += '0';
      encoded += String((uint8_t)c, HEX);
    }
  }
  return encoded;
}

void logToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.println("\n========== LOGGING TO GOOGLE SHEETS ==========");
  HTTPClient http;
  http.begin(GAS_SCRIPT_URL);
  http.addHeader("Content-Type", "application/json");
  DynamicJsonDocument doc(512);
  doc["soil_moisture"] = currentData.soilMoisture;
  doc["air_temp"] = currentData.airTemperature;
  doc["air_humidity"] = currentData.airHumidity;
  doc["soil_temp"] = currentData.soilTemperature;
  doc["light"] = currentData.lightLevel;
  doc["pump_state"] = pumpState;
  String payload;
  serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  if (httpCode == 200 || httpCode == 302) {
    Serial.println("  ✓ Data logged to Google Sheets");
  } else {
    Serial.println("  ✗ Google Sheets logging failed, HTTP code: " + String(httpCode));
    errorCount++;
  }
  http.end();
}

void sendStatusReport() {
  // Rate-limit this to TELEGRAM_INTERVAL_MS
  if (millis() - lastTelegramAlert < TELEGRAM_INTERVAL_MS) return;
  String report = "📊 IRRIGATION SYSTEM STATUS\n\n";
  report += "🌡️ Air: " + String(currentData.airTemperature, 1) + "°C, " +
            String(currentData.airHumidity, 1) + "%\n";
  report += "🌱 Soil: " + String(currentData.soilMoisture, 1) + "% moisture, " +
            String(currentData.soilTemperature, 1) + "°C\n";
  report += "☀️ Light: " + String(currentData.lightLevel, 0) + " lux\n";
  report += "💧 Pump: " + String(pumpState ? "ON" : "OFF") + "\n";
  report += "🤖 Mode: " + String(autoMode ? "AUTO" : "MANUAL") + "\n";
  report += "⏱️ Uptime: " + String(millis() / 60000) + " min\n";
  sendTelegramAlert(report);
}

// ========== LCD UPDATE ==========
void updateLCD() {
  // Show a concise summary on 16x2 LCD:
  // Line1: Air T xx.xC RH yy%
  // Line2: Soil xx% Pump:ON/OFF (or ERR where appropriate)
  lcd.clear();
  // Line 1
  String line1 = "";
  if (!isnan(currentData.airTemperature)) {
    line1 += "T:";
    line1 += String(currentData.airTemperature, 1);
    line1 += "C ";
  } else {
    line1 += "T:ERR ";
  }
  if (!isnan(currentData.airHumidity)) {
    line1 += "RH:";
    line1 += String(currentData.airHumidity, 0);
    line1 += "%";
  } else {
    line1 += "RH:ERR";
  }
  // truncate/pad to 16
  if (line1.length() > 16) line1 = line1.substring(0, 16);
  lcd.setCursor(0, 0);
  lcd.print(line1);

  // Line 2
  String line2 = "";
  if (currentData.soilMoisture >= 0 && currentData.soilMoisture <= 100) {
    line2 += "Soil:";
    line2 += String(currentData.soilMoisture, 0);
    line2 += "% ";
  } else {
    line2 += "Soil:ERR ";
  }
  if (currentData.soilTemperature != DEVICE_DISCONNECTED_C) {
    line2 += "S:";
    line2 += String(currentData.soilTemperature, 0);
    line2 += "C ";
  } else {
    line2 += "S:ERR ";
  }
  line2 += (pumpState ? "P:ON" : "P:OFF");
  if (line2.length() > 16) line2 = line2.substring(0, 16);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ========== MISC ==========
float calculateET() {
  float tempFactor = max(0.0f, currentData.airTemperature - 20.0f) / 10.0f;
  float lightFactor = currentData.lightLevel / 50000.0f;
  float humidityFactor = (100.0f - currentData.airHumidity) / 100.0f;
  float et = (tempFactor * 0.4 + lightFactor * 0.4 + humidityFactor * 0.2);
  return constrain(et, 0.0, 1.0);
}

// ========== MAIN: setup & loop ==========
void setup() {
  Serial.begin(115200);
  delay(100);

  // Pins
  pinMode(PUMP_PIN, OUTPUT);
  // Initialize pump as off
  digitalWrite(PUMP_PIN, PUMP_ACTIVE_HIGH ? LOW : HIGH);
  pumpState = false;

  // Initialize sensors
  dht.begin();
  ds18Sensor.begin();
  Wire.begin();
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 initialized");
  } else {
    Serial.println("BH1750 init failed");
  }

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Smart Irrigation");
  delay(1000);
  lcd.clear();

  // WiFi & Security & MQTT
  initSecurity();
  mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);

  // Attempt initial connections
  connectWiFi();
  connectMQTT();

  // Read sensors once at startup
  if (!readAllSensors()) {
    Serial.println("Initial sensor read failed; continuing...");
  } else {
    publishTelemetry();
  }
}

unsigned long lastMqttReconnectAttempt = 0;
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 10000; // try every 10s

void loop() {
  // Ensure WiFi and MQTT connection
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    if (millis() - lastMqttReconnectAttempt > MQTT_RECONNECT_INTERVAL_MS) {
      lastMqttReconnectAttempt = millis();
      connectMQTT();
    }
  } else {
    mqttClient.loop();
  }

  // Sensor reading schedule
  if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = millis();
    bool ok = readAllSensors();
    if (ok) {
      irrigationControl(); // apply control if in auto mode
    } else {
      Serial.println("Skipping control/publish due to invalid readings");
    }
  }

  // Safety check for pump runtime (auto-shutdown if exceeded)
  checkPumpRuntimeSafety();

  // Telemetry schedule
  if (millis() - lastTelemetryPublish >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryPublish = millis();
    if (readAllSensors()) { // fresh data for telemetry
      publishTelemetry();
    }
  }

  // Google Sheets logging schedule
  if (millis() - lastGoogleLog >= GOOGLE_LOG_INTERVAL_MS) {
    lastGoogleLog = millis();
    if (readAllSensors()) {
      logToGoogleSheets();
    }
  }

  // Periodic status report via Telegram (rate-limited)
  if (millis() - lastTelegramAlert >= TELEGRAM_INTERVAL_MS) {
    sendStatusReport();
  }

  // small yield to lower CPU usage
  delay(10);
}
