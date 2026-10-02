#include "BluetoothSerial.h"
#include "string.h"

#define LED 2

BluetoothSerial SerialBT;
char buffer[60];
static int count = 0;

void setup() {
  pinMode(LED, OUTPUT);
  SerialBT.begin("ESP32test"); // Bluetooth device name
  Serial.begin(115200);
  Serial.println("\nThe device started, now you can pair it with Bluetooth!");
}

void loop() {
  Serial.print("hi markt \n");

  while (SerialBT.available()) {
    Serial.print("write bluethot1 \n");
    uint8_t message[] = "get_id()";
    Serial.print("write bluethot2 \n");
    SerialBT.write(message, sizeof(message) - 1);
    Serial.print("write bluethot \n");
    delay(1000);
  }
  
  while (SerialBT.available()) {
    buffer[count] = SerialBT.read();
    count++;
  }
  
  if (count > 0) {
    Serial.print(buffer);
    
    if (strncmp(buffer, "led_on", 6) == 0) {
      digitalWrite(LED, HIGH);
    }
    
    if (strncmp(buffer, "led_off", 7) == 0) {
      digitalWrite(LED, LOW);
    }
    
    count = 0;
    memset(buffer, 0, sizeof(buffer));
  }
}
