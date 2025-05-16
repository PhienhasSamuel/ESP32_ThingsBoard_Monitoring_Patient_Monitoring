#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// OLED display configuration
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

// Sensor and MQTT objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "demo.thingsboard.io";
const int mqttPort = 1883;
const char* token = "4TCQ2h0zijCm5TMKZS7K";  // ThingsBoard device token

WiFiClient espClient;
PubSubClient client(espClient);

// Movement detection variables
unsigned long lastMovementTime = 0;
const unsigned long movementTimeout = 15000;  // 15 seconds
static float lastAcc = 9.8;  // Approximate gravity baseline

// Connect to Wi-Fi
void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}

// Connect to MQTT broker
void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32Client", token, "")) break;
    delay(2000);
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();         // Initialize temperature sensor
  mpu.begin();         // Initialize MPU6050 motion sensor
  Wire.begin();        // I2C communication for OLED and MPU

  pinMode(PULSE_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize OLED
  display.clearDisplay();

  setupWiFi();
  client.setServer(mqttServer, mqttPort);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Simulated pulse value (adjusted to 20–130 BPM range)
  int pulseRaw = analogRead(PULSE_PIN);
  int pulse = map(pulseRaw, 0, 4095, 20, 130);

  // Read motion data
  sensors_event_t a, g, tempSensor;
  mpu.getEvent(&a, &g, &tempSensor);

  // Calculate total acceleration (vector magnitude)
  float acc = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  // Detect real movement based on acceleration change
  if (abs(acc - lastAcc) > 1.0) {
    lastMovementTime = millis();
  }
  lastAcc = acc;

  // Set movement flag: 1 if recent movement, else 0
  int movement = (millis() - lastMovementTime > movementTimeout) ? 0 : 1;

  // Check if panic button is pressed
  int panicalert = digitalRead(BUTTON_PIN) == LOW ? 1 : 0;

  // Check alert condition: abnormal vitals or no movement or panic
  int alert = (temperature > 38 || pulse < 50 || pulse > 120 || movement == 1 || panicalert == 1) ? 1 : 0;

  // Trigger buzzer and LED alert
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

  // Display data on OLED
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

  // Create MQTT JSON payload
  String payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"pulse\":" + String(pulse) + ",";
  payload += "\"movement\":" + String(movement) + ",";
  payload += "\"panicalert\":" + String(panicalert) + ",";
  payload += "\"alert\":" + String(alert);
  payload += "}";

  // Send data to ThingsBoard
  client.publish("v1/devices/me/telemetry", payload.c_str());

  // Debug info in serial monitor
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
