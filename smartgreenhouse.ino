#define BLYNK_TEMPLATE_ID 
#define BLYNK_TEMPLATE_NAME "SmartGreenhouse"
#define BLYNK_AUTH_TOKEN 

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// Pin definitions
#define DHTPIN        D4
#define DHTTYPE       DHT11
#define SOIL_PIN      A0
#define LED_PIN       D5
#define RELAY_FAN     D6
#define RELAY_PUMP    D7

// WiFi credentials
const char* ssid     = ;
const char* password = ;

// Thresholds (adjustable from Blynk)
int tempThreshold  = 30;   // °C — fan turns on above this
int soilThreshold  = 500;  // raw ADC — pump turns on below this (drier soil)
bool autoMode      = true; // manual override from Blynk

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// Virtual Pins
// V0 — Temperature (display)
// V1 — Humidity (display)
// V2 — Soil Moisture % (display)
// V3 — Fan relay (button)
// V4 — Pump relay (button)
// V5 — LED (button)
// V6 — Temp threshold (slider)
// V7 — Soil threshold (slider)
// V8 — Auto mode toggle
// V9 — Status label

// Read sensors and send to Blynk
void sendSensorData() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int soilRaw = analogRead(SOIL_PIN);
  int soilPct = map(soilRaw, 1023, 0, 0, 100); // invert: wet=high

  if (isnan(temp) || isnan(hum)) {
    Blynk.virtualWrite(V9, "DHT11 Error");
    return;
  }

  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, soilPct);

  // Auto control logic
  if (autoMode) {
    // Fan: ON if temp exceeds threshold
    bool fanOn = (temp >= tempThreshold);
    digitalWrite(RELAY_FAN, fanOn ? LOW : HIGH); // active LOW relay
    Blynk.virtualWrite(V3, fanOn ? 1 : 0);

    // Pump: ON if soil is dry (low moisture %)
    bool pumpOn = (soilPct < map(soilThreshold, 1023, 0, 0, 100));
    digitalWrite(RELAY_PUMP, pumpOn ? LOW : HIGH);
    Blynk.virtualWrite(V4, pumpOn ? 1 : 0);

    // LED: ON if either fan or pump is active
    digitalWrite(LED_PIN, (fanOn || pumpOn) ? HIGH : LOW);
    Blynk.virtualWrite(V5, (fanOn || pumpOn) ? 1 : 0);

    // Status message
    String status = "Auto | ";
    if (fanOn)  status += "Fan ON ";
    if (pumpOn) status += "Pump ON";
    if (!fanOn && !pumpOn) status += "All OK";
    Blynk.virtualWrite(V9, status);
  }

  Serial.printf("Temp: %.1f°C  Hum: %.1f%%  Soil: %d%%\n", temp, hum, soilPct);
}

// Manual control from Blynk buttons
BLYNK_WRITE(V3) { // Fan button (manual)
  if (!autoMode) {
    int v = param.asInt();
    digitalWrite(RELAY_FAN, v ? LOW : HIGH);
  }
}

BLYNK_WRITE(V4) { // Pump button (manual)
  if (!autoMode) {
    int v = param.asInt();
    digitalWrite(RELAY_PUMP, v ? LOW : HIGH);
  }
}

BLYNK_WRITE(V5) { // LED button
  if (!autoMode) {
    digitalWrite(LED_PIN, param.asInt());
  }
}

BLYNK_WRITE(V6) { tempThreshold = param.asInt(); }  // Temp threshold slider
BLYNK_WRITE(V7) { soilThreshold = param.asInt(); }  // Soil threshold slider

BLYNK_WRITE(V8) { // Auto/Manual toggle
  autoMode = param.asInt();
  Blynk.virtualWrite(V9, autoMode ? "Auto Mode" : "Manual Mode");
  if (!autoMode) {
    // Safe state when switching to manual
    digitalWrite(RELAY_FAN,  HIGH);
    digitalWrite(RELAY_PUMP, HIGH);
    digitalWrite(LED_PIN,    LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_FAN,  OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);

  // Safe defaults (relays OFF = HIGH for active-LOW modules)
  digitalWrite(RELAY_FAN,  HIGH);
  digitalWrite(RELAY_PUMP, HIGH);
  digitalWrite(LED_PIN,    LOW);

  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Send sensor data every 2 seconds
  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}