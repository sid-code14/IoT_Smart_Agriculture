#include <SoftwareSerial.h>
#include <SPI.h>
#include <LoRa.h>

#define MAX485_DE_RE 7
#define RX_PIN 2
#define TX_PIN 3
#define SS 10
#define RST 9
#define DIO0 2

SoftwareSerial rs485Serial(RX_PIN, TX_PIN);

int counter = 0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Initialize RS485 Serial
  rs485Serial.begin(9600);
  
  // Initialize MAX485
  pinMode(MAX485_DE_RE, OUTPUT);
  digitalWrite(MAX485_DE_RE, LOW); // Receive mode
  
  // Initialize LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // Read NPK values
  int nitrogen = readNutrientValue(0x1E);
  int phosphorus = readNutrientValue(0x1F);
  int potassium = readNutrientValue(0x20);

  // Display values on Serial Monitor
  Serial.print("Nitrogen: ");
  Serial.print(nitrogen);
  Serial.println(" mg/kg");
  
  Serial.print("Phosphorus: ");
  Serial.print(phosphorus);
  Serial.println(" mg/kg");
  
  Serial.print("Potassium: ");
  Serial.print(potassium);
  Serial.println(" mg/kg");
  
  // Prepare LoRa message
  String LoRaMessage = String(counter) + "/" +
                       "N:" + String(nitrogen) + "," +
                       "P:" + String(phosphorus) + "," +
                       "K:" + String(potassium);
  
  // Send LoRa message
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
  
  counter++;
  
  delay(1500); // Delay between readings
}

int readNutrientValue(byte registerAddress) {
  byte requestFrame[] = {0x01, 0x03, 0x00, registerAddress, 0x00, 0x01};
  byte responseFrame[7];
  
  // Calculate CRC for the request frame
  unsigned int crc = calculateCRC(requestFrame, 6);
  requestFrame[6] = lowByte(crc);
  requestFrame[7] = highByte(crc);
  
  // Send request
  digitalWrite(MAX485_DE_RE, HIGH); // Transmit mode
  rs485Serial.write(requestFrame, sizeof(requestFrame));
  rs485Serial.flush();
  digitalWrite(MAX485_DE_RE, LOW); // Receive mode
  
  // Wait for response
  delay(100);
  
  // Read response
  if (rs485Serial.available() >= 7) {
    rs485Serial.readBytes(responseFrame, 7);
    
    // Verify CRC
    unsigned int responseCRC = word(responseFrame[6], responseFrame[5]);
    if (responseCRC == calculateCRC(responseFrame, 5)) {
      // Extract nutrient value
      int value = word(responseFrame[3], responseFrame[4]);
      return value;
    }
  }
  
  return -1; // Return -1 if reading failed
}

unsigned int calculateCRC(byte *frame, byte length) {
  unsigned int crc = 0xFFFF;
  for (byte i = 0; i < length; i++) {
    crc ^= frame[i];
    for (byte j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
