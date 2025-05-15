#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin definitions
#define DHTPIN 15
#define DHTTYPE DHT22
#define PULSE_PIN 34
#define BUTTON_PIN 4
#define LED_PIN 2
#define BUZZER_PIN 13

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

// Wi-Fi & MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "demo.thingsboard.io";
const int mqttPort = 1883;
const char* token = "4TCQ2h0zijCm5TMKZS7K"; // Replace with your device token

WiFiClient espClient;
PubSubClient client(espClient);

// Movement detection
unsigned long lastMovementTime = 0;
const unsigned long movementTimeout = 15000;  // 15 seconds
static float lastAcc = 9.8; // Initialize to gravity

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32Client", token, "")) break;
    delay(2000);
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  mpu.begin();
  pinMode(PULSE_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  setupWiFi();
  client.setServer(mqttServer, mqttPort);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int pulseRaw = analogRead(PULSE_PIN);
  int pulse = map(pulseRaw, 0, 4095, 20, 130);  // Updated range 20–130 BPM

  sensors_event_t a, g, tempSensor;
  mpu.getEvent(&a, &g, &tempSensor);

  // Calculate total acceleration magnitude
  float acc = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  // Detect real movement (not jitter)
  if (abs(acc - lastAcc) > 1.0) {  // 1.0 m/s² threshold
    lastMovementTime = millis();
  }
  lastAcc = acc;

  int movement = (millis() - lastMovementTime > movementTimeout) ? 0 : 1;

  // Panic button check
  int panicalert = digitalRead(BUTTON_PIN) == LOW ? 1 : 0;

  // Alert condition
  int alert = (temperature > 38 || pulse < 50 || pulse > 120 || movement == 1 || panicalert == 1) ? 1 : 0;

  // Alert indicators
  if (alert) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
    delay(500);
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // OLED Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.printf("Temp: %.1f C\n", temperature);
  display.printf("Pulse: %d bpm\n", pulse);
  display.printf("Movement: %s\n", movement == 1 ? "YES" : "NO");
  display.printf("Panic Button: %s\n", panicalert == 1 ? "PRESSED" : "NOT PRESSED");
  display.printf("Alert: %s\n", alert == 1 ? "YES" : "NO");
  display.display();

  // MQTT Payload for ThingsBoard
  String payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"pulse\":" + String(pulse) + ",";
  payload += "\"movement\":" + String(movement) + ",";
  payload += "\"panicalert\":" + String(panicalert) + ",";
  payload += "\"alert\":" + String(alert);
  payload += "}";

  client.publish("v1/devices/me/telemetry", payload.c_str());

  // Debug (optional)
  Serial.print("Acc: ");
  Serial.print(acc);
  Serial.print(" | Movement: ");
  Serial.print(movement);
  Serial.print(" | PulseRaw: ");
  Serial.print(pulseRaw);
  Serial.print(" | Pulse: ");
  Serial.println(pulse);

  delay(1000);
}
