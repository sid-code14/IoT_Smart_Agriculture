#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BlynkEdgent.h"

// LoRa module pins
#define ss 15
#define rst 16
#define dio0 4

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define water pump relay pins (assumed active LOW)
#define WATER_PUMP_N_PIN 8  // Nitrogen pump
#define WATER_PUMP_P_PIN 9  // Phosphorus pump
#define WATER_PUMP_K_PIN 10 // Potassium pump

// Nutrient threshold values (adjust as required)
#define THRESHOLD_N 50    // Example threshold for Nitrogen (mg/kg)
#define THRESHOLD_P 20    // Example threshold for Phosphorus (mg/kg)
#define THRESHOLD_K 50    // Example threshold for Potassium (mg/kg)

// Variables to store received data
String counter;
String nitrogen;
String phosphorus;
String potassium;

void setup() {
  Serial.begin(115200);
  BlynkEdgent.begin();

  // Initialize water pump relay pins
  pinMode(WATER_PUMP_N_PIN, OUTPUT);
  pinMode(WATER_PUMP_P_PIN, OUTPUT);
  pinMode(WATER_PUMP_K_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_N_PIN, HIGH); // Pump OFF initially (assuming active LOW)
  digitalWrite(WATER_PUMP_P_PIN, HIGH); // Pump OFF initially (assuming active LOW)
  digitalWrite(WATER_PUMP_K_PIN, HIGH); // Pump OFF initially (assuming active LOW)

  // Initialize LoRa module
  while (!Serial);
  Serial.println("LoRa Receiver");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
}

void loop() {
  BlynkEdgent.run();

  // Try to parse incoming LoRa packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    String LoRaData = LoRa.readString();
    Serial.print("Received packet: ");
    Serial.println(LoRaData);

    // Expected format: counter/N:<nitrogen>,P:<phosphorus>,K:<potassium>
    int pos1 = LoRaData.indexOf('/');
    int comma1 = LoRaData.indexOf(',');
    int comma2 = LoRaData.indexOf(',', comma1 + 1);
    
    if (pos1 != -1 && comma1 != -1 && comma2 != -1) {
      counter = LoRaData.substring(0, pos1);
      // Extract nutrient values and remove the leading identifier if present
      nitrogen = LoRaData.substring(pos1 + 1, comma1);
      phosphorus = LoRaData.substring(comma1 + 1, comma2);
      potassium = LoRaData.substring(comma2 + 1);

      if (nitrogen.startsWith("N:")) nitrogen = nitrogen.substring(2);
      if (phosphorus.startsWith("P:")) phosphorus = phosphorus.substring(2);
      if (potassium.startsWith("K:")) potassium = potassium.substring(2);

      // Send nutrient data to Blynk
      Blynk.virtualWrite(V1, nitrogen);    // Nitrogen level
      Blynk.virtualWrite(V2, phosphorus);    // Phosphorus level
      Blynk.virtualWrite(V3, potassium);     // Potassium level

      // Display data on Serial Monitor
      Serial.print("Packet No: ");
      Serial.println(counter);
      Serial.print("Nitrogen Level: ");
      Serial.println(nitrogen);
      Serial.print("Phosphorus Level: ");
      Serial.println(phosphorus);
      Serial.print("Potassium Level: ");
      Serial.println(potassium);

      // Update OLED display
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print("Packet No: ");
      display.println(counter);
      display.setCursor(0, 15);
      display.print("Nitrogen: ");
      display.println(nitrogen);
      display.setCursor(0, 30);
      display.print("Phosphorus: ");
      display.println(phosphorus);
      display.setCursor(0, 45);
      display.print("Potassium: ");
      display.println(potassium);
      display.display();

      // Check if any nutrient value is below its threshold and activate corresponding pump
      if (nitrogen.toInt() < THRESHOLD_N) {
        digitalWrite(WATER_PUMP_N_PIN, LOW); // Turn on Nitrogen pump
        Serial.println("Nitrogen level is low. Activating Nitrogen pump.");
      } else {
        digitalWrite(WATER_PUMP_N_PIN, HIGH); // Turn off Nitrogen pump
      }

      if (phosphorus.toInt() < THRESHOLD_P) {
        digitalWrite(WATER_PUMP_P_PIN, LOW); // Turn on Phosphorus pump
        Serial.println("Phosphorus level is low. Activating Phosphorus pump.");
      } else {
        digitalWrite(WATER_PUMP_P_PIN, HIGH); // Turn off Phosphorus pump
      }

      if (potassium.toInt() < THRESHOLD_K) {
        digitalWrite(WATER_PUMP_K_PIN, LOW); // Turn on Potassium pump
        Serial.println("Potassium level is low. Activating Potassium pump.");
      } else {
        digitalWrite(WATER_PUMP_K_PIN, HIGH); // Turn off Potassium pump
      }
    } else {
      Serial.println("Error: Invalid data format received.");
    }
  }
}

// Blynk virtual pins are used to control the water pumps
BLYNK_WRITE(V4) {
    int pinValue = param.asInt();
    digitalWrite(WATER_PUMP_N_PIN, pinValue);
    Serial.println(pinValue == 1 ? "N pump turned ON" : "N pump turned OFF");
  }
  
  BLYNK_WRITE(V5) {
    int pinValue = param.asInt();
    digitalWrite(WATER_PUMP_P_PIN, pinValue);
    Serial.println(pinValue == 1 ? "P pump turned ON" : "P pump turned OFF");
  }
  
  BLYNK_WRITE(V6) {
    int pinValue = param.asInt();
    digitalWrite(WATER_PUMP_k_PIN, pinValue);
    Serial.println(pinValue == 1 ? "K pump turned ON" : "K pump turned OFF");
  }
