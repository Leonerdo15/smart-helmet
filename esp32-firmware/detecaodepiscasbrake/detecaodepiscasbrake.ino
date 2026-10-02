#include "helmet_config.h"
#include <MPU6050_tockn.h>
#include <Wire.h>
#include "esp_adc_cal.h"
#include <ESP32Servo.h>
#include <WiFi.h>
//#include <WiFiClient.h>
//#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "BluetoothSerial.h"
#include "string.h"
#define trigPin 18 // define TrigPin
#define echoPin 19 // define EchoPin.
#define MAX_DISTANCE 700 // Maximum sensor distance is rated at 400-500cm.
//timeOut= 2*MAX_DISTANCE /100 /340 *1000000 = MAX_DISTANCE*58.8
float timeOut = MAX_DISTANCE * 60; 
int soundVelocity = 340; // define sound speed=340m/s
String ip = "";
#define SDA 13
#define SCL 14
#define PIN_ANALOG_IN   6
 // A10, ADC2_CHANNEL_0
#define DEFAULT_VREF    1100        //Default vref
#define NO_OF_SAMPLES   64          //Multisampling

int id=0;
MPU6050 mpu6050(Wire);//Attach the IIC
int16_t ax,ay,az;//define acceleration values of 3 axes
int16_t gx,gy,gz;//define variables to save the values in 3 axes of gyroscope
int timesInsideLeft=0,temperature=5,timesInsideRight=0;
char buffer[60];
bool pisca=false,piscaL=false,piscaR=false;
long timer = 0;
int side=0,brake=0,timerBlink=0,hi=0,ledcar=0,timerLedCar=0,sendDataTime=0,timeorarieDist=0,timetimeerature=0;
Servo myservo;  // create servo object to control a servo
int posVal = 0;    // variable to store the servo position
int servoPin = 5; // Servo motor pin
// WiFi network info.
const char *ssid = HELMET_WIFI_SSID; // Enter your Wi-Fi Name
const char *pass = HELMET_WIFI_PASSWORD; // Enter your Wi-Fi Password
void send_event(const char *event);
const char *host = "maker.ifttt.com";
const char *privateKey = HELMET_IFTTT_KEY;
const int hallPin = 33; // GPIO4
int hallState = 1;
int address = 0;


adc_channel_t channel = ADC_CHANNEL_0;      // ADC1:GPIO36, ADC2:GPIO4
adc_unit_t unit = ADC_UNIT_2;               // ADC2
adc_atten_t atten = ADC_ATTEN_DB_11;        // Full scale 0-3.9V, precision range 150mV-2450mV

esp_adc_cal_characteristics_t *adc_chars;

esp_adc_cal_value_t val_type;


void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  
  WiFi.begin(ssid, pass);
  delay(2000);
  
  getIp();
  Wire.begin(SDA, SCL);          //attach the IIC pin
  mpu6050.begin();               //initialize the MPU6050
  mpu6050.calcGyroOffsets(true); //get the offsets value
  if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten((adc1_channel_t)channel, atten);
    }
    else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
  
  myservo.setPeriodHertz(50);           // standard 50 hz servo
  myservo.attach(servoPin, 500, 2500);  // attaches the servo on servoPin to the servo object


  uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }
        else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    //printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);

    double vol = voltage / 1000.0f;
    double Rt = 10 * vol / (3.3 - vol); //calculate resistance value of thermistor
    double timeK = 1 / (1 / (273.15 + 25) + log(Rt / 10) / 3950.0); //calculate timeerature (Kelvin)
    double timeC = timeK - 273.15;     //calculate timeerature (Celsius)
    Serial.printf("ADC value : %d,\tVoltage : %.2fV, \ttimeerature : %.2fC\n", adc_reading, vol, timeC);
    //ultrasonic.settimeerature(timeC);
  pinMode(hallPin, INPUT);
  pinMode(12,OUTPUT);
  digitalWrite(12,LOW); //esquerda
  pinMode(15,OUTPUT);
  digitalWrite(15,LOW); //direita
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW); //brake
  pinMode(0,OUTPUT);
  digitalWrite(0,LOW); //buzzer
  pinMode(23,OUTPUT);
  digitalWrite(23,LOW); //ledcarnearby
  pinMode(trigPin,OUTPUT);// set trigPin to output mode
  pinMode(echoPin,INPUT);

}


void loop() {
  while(hallState==0){
    hallState = digitalRead(hallPin);
    Serial.println(hallState);
  }

  mpu6050.update();            //update the MPU6050
  getMotion3(); 
  //Serial.print((ax)+(ay)+(az));
  //Serial.print("Gs de forca\n\n");
  float gforce=((fabs(ax)/ 16384)+(fabs(ay)/ 16384)+(fabs(az)/ 16384));
  if(gforce>30){
    digitalWrite(0,HIGH);
    Serial.print(gforce);
    Serial.print(" gforce\n");
    send_event("sms", gforce);
    delay(10000);
    //INsert send impact force to the api forca do impacto data e hora e se existe movimento aseguir ao impacto    
  }
  
  if(millis() - timer > 100){   //each 0.1 second printf the data
     mpu6050.update();            //update the MPU6050
    getMotion6(); 
    
    sendDataTime=sendDataTime+1;
    if(sendDataTime>=200){

      sendData(ax,ay,az,gx,gy,gz,mpu6050.getTemp(),timeorarieDist);
      sendDataTime=0;

    }


    //Serial.print("\na/g:\t");
    //Serial.print(ax); Serial.print("\t");
    //Serial.print(ay); Serial.print("\t");
    //Serial.print(az); Serial.print("\t");
    //Serial.print(gx); Serial.print(" ");
    //Serial.print(gy); Serial.print(" ");
    //Serial.println(gz);
    timerLedCar++;

    if(timerLedCar>2){
    timerLedCar=0;

      int distance = getSonar();
      Serial.print(distance); Serial.print(" distancia \n");
      timeorarieDist=distance;
      if(distance<50){
        digitalWrite(23,HIGH);
        Serial.print(distance); Serial.print(" distancia \n");
        ledcar=0;
      }else if(ledcar >10){
        digitalWrite(23,LOW);
      }else
        ledcar++;
    }
    

    timetimeerature=timetimeerature+1;
    if(timetimeerature>50){
      timetimeerature=0;
      uint32_t adc_reading = 0;
      //Multisampling
     for (int i = 0; i < NO_OF_SAMPLES; i++) {
         if (unit == ADC_UNIT_1) {
             adc_reading += adc1_get_raw((adc1_channel_t)channel);
         }
         else {
             int raw;
             adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
             adc_reading += raw;
          }
     }
     adc_reading /= NO_OF_SAMPLES;
      //Convert adc_reading to voltage in mV
      uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
      //printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);

      double vol = voltage / 1000.0f;
      double Rt = 10 * vol / (3.3 - vol); //calculate resistance value of thermistor
      double timeK = 1 / (1 / (273.15 + 25) + log(Rt / 10) / 3950.0); //calculate timeerature (Kelvin)
      double timeC = timeK - 273.15;     //calculate timeerature (Celsius)
      Serial.printf("ADC value : %d,\tVoltage : %.2fV, \ttimeerature : %.2fC\n", adc_reading, vol, timeC);
      
    }
    float timeC=mpu6050.getTemp();
    Serial.print(timeC);
      if (timeC>30){
         myservo.write(200);
      }else
         myservo.write(0);

    
    //Serial.print((ax/16384)+(ay/16384)+(az/16384));
    //Serial.print("Gs de forca\n\n");
    //Serial.print((ax/16384));
    //Serial.print("Gs de forca\n");
    //Serial.print((ay/16384));
    //Serial.print("Gs de forca\n");
    //Serial.print((az/16384));
    //Serial.print("Gs de forca\n");

   

  


    if(ay>3000){
      digitalWrite(2,HIGH); //brake
      brake=0;
    }else{
      brake++;
    }
    if(brake==4){
      digitalWrite(2,LOW); //brake

    }
    

    if(gz>20000 ){
      Serial.println(timesInsideLeft);
      timesInsideLeft=timesInsideLeft+1;
      temperature=10;
      side=1;
      Serial.println("WE are innnnn 1");
      Serial.println(timesInsideLeft);
      Serial.println(temperature);
    }else if(gz<-20000 ){
      Serial.println(timesInsideLeft);
      timesInsideRight=timesInsideRight+1;
      temperature=10;
      side=2;
      Serial.println("WE are innnnn 2");
      Serial.println(timesInsideRight);
      Serial.println(temperature);
    }else{
      temperature=temperature-1;
      
    }
    //Serial.println("timer blink: ");
    //Serial.println(timerBlink);
    //Serial.println("side: ");
    //Serial.println(side);
    //Serial.println("pisca: ");
    //Serial.println(pisca);
    
    timerBlink++;
    if(timerBlink==4){
      if(piscaL){
        if(digitalRead(12)==1){
          digitalWrite(12,LOW);
          timerBlink=0;
        }else if(digitalRead(12)==0){
          digitalWrite(12,HIGH);
          timerBlink=0;
        }
      }

      if(piscaR){
        if(digitalRead(15)==1){
          digitalWrite(15,LOW);
          timerBlink=0;
        }else if(digitalRead(15)==0){
          digitalWrite(15,HIGH);
          timerBlink=0;
        }
      }
    }
      

    if((timesInsideLeft==2 && temperature>0) || (temperature>0 && timesInsideRight==2)){
      if(pisca && side==1){
        pisca=false;
        digitalWrite(12,LOW);
        Serial.println("Led left down");
        side=0;
        piscaL=false;
      }else if (side == 1){
        digitalWrite(12,HIGH);
        Serial.println("Led left up");
        pisca=true;
        side=0;
        timerBlink=0;
        piscaL=true;              
      }else if(pisca && side==2){
        pisca=false;
        digitalWrite(15,LOW);
        Serial.println("Led right down");
        side=0;
        piscaR=false;
      }else if (side == 2){
        Serial.println("Led right up");
        digitalWrite(15,HIGH);
        side=0;
        pisca=true;
        timerBlink=0;
        piscaR=true;              
      }
      //Serial.println("timesInsideLeft= o");
      timesInsideLeft=0;
      timesInsideRight=0;
    }
    
     else if(temperature<0){      
      //Serial.println("else if de baixo");
      timesInsideLeft=0;
      timesInsideRight=0;
    }
  //  Serial.print("a/g:\t");
  //  Serial.print((float)ax / 16384); Serial.print("g\t");
  //  Serial.print((float)ay / 16384); Serial.print("g\t");
   // Serial.print((float)az / 16384); Serial.print("g\t");
    //Serial.print((float)gx / 131); Serial.print("d/s \t");
    //Serial.print((float)gy / 131); Serial.print("d/s \t");
    //Serial.print((float)gz / 131); Serial.print("d/s \n");
    timer = millis();
    hallState = digitalRead(hallPin);

  }
}
void getMotion6(void){
  ax=mpu6050.getRawAccX();//gain the values of X axis acceleration raw data
  ay=mpu6050.getRawAccY();//gain the values of Y axis acceleration raw data
  az=mpu6050.getRawAccZ();//gain the values of Z axis acceleration raw data
  gx=mpu6050.getRawGyroX();//gain the values of X axis Gyroscope raw data
  gy=mpu6050.getRawGyroY();//gain the values of Y axis Gyroscope raw data
  gz=mpu6050.getRawGyroZ();//gain the values of Z axis Gyroscope raw data
}

void getMotion3(void){
  ax=mpu6050.getRawAccX();//gain the values of X axis acceleration raw data
  ay=mpu6050.getRawAccY();//gain the values of Y axis acceleration raw data
  az=mpu6050.getRawAccZ();//gain the values of Z axis acceleration raw data
}

void send_event(const char *event,float gforce)
{
  Serial.print("Connecting to "); 
  Serial.println(host);
    // Use WiFiClient class to create TCP connections
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("wifi connected.....");
    // We now create a URI for the request
    String url = "https://maker.ifttt.com/trigger/";
    url += event;
    url += "/with/key/";
    url += privateKey;
    Serial.print("Requesting URL: ");
    Serial.println(url);
    // This will send the request to the server
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<64> doc;
    doc["value1"] = HELMET_ALERT_NAME;
    doc["value2"] = HELMET_ALERT_PHONE;
    doc["value3"] = HELMET_ALERT_LOCATION;
    String postData;
    serializeJson(doc, postData);
    Serial.print(postData);

    int httpResponseCode = http.POST(postData);
    Serial.print(httpResponseCode);

      if (httpResponseCode > 0) {
        String response = http.getString();
        
        Serial.println(httpResponseCode);
        Serial.println(response);
      }
      else {
        Serial.print("Error on HTTP request: ");
        Serial.println(httpResponseCode);
      }
    http.end();  
  }
    

  //while(client.connected())
  //{
  //  if(client.available())
  //  {
  //    String line = client.readStringUntil('\r');
    //  Serial.print(line);
    //} else {
      // No data yet, wait a bit
     // delay(50);
    //};
  //}
  Serial.println();
  Serial.println("closing connection");
  
}


float checkIfRegister(void){
  float readParam;
  EEPROM.get(address, readParam);
  if(readParam > 0){
    return readParam;
    Serial.println(readParam);
  }else{
    EEPROM.put(address, 1);
    Serial.println("insert id");
    checkIfRegister();
  }
  
  
 
}

float getSonar1() {
  unsigned long pingTime;
  float distance;
  // make trigPin output high level lasting for 10μs to triger HC_SR04
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Wait HC-SR04 returning to the high level and measure out this waitting temperature
  pingTime = pulseIn(echoPin, HIGH, timeOut); 
  // calculate the distance according to the temperature
  distance = (float)pingTime * soundVelocity / 2 / 10000; 
  return distance; // return the distance value
}

float getSonar() {
   float distances[10];
  float sum = 0;
  float avg = 0;
  float variance = 0;
  float sd = 0;

  // Get 10 distances
  for (int i = 0; i < 10; i++) {
    distances[i] = getSonar1();
    sum += distances[i];
    delay(5);
  }

  // Calculate the average
  avg = sum / 10;

  // Calculate the variance
  for (int i = 0; i < 10; i++) {
    variance += pow(distances[i] - avg, 2);
  }
  variance /= 10;

  // Calculate the standard deviation
  sd = sqrt(variance);

  // Calculate the min and max distances
  float minDist = avg - sd;
  float maxDist = avg + sd;

  // Calculate the average of the distances between 1 and -1 sigma
  sum = 0;
  int count = 0;
  for (int i = 0; i < 10; i++) {
    if (distances[i] >= minDist && distances[i] <= maxDist) {
      sum += distances[i];
      count++;
    }
  }

  if (count > 0) {
    return sum / count;
  } else {
    return 0;
  }
}



void getIp() {
  
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected...");
    // We now create a URI for the request
    String url = "http://";
    url += "ulideparty.ddns.net:8080/api/user/1/ip";
    url += "";
    Serial.print("Requesting URL: ");
    Serial.println(url);
  
    HTTPClient http;
    http.begin(url);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      
      String payload = http.getString();
      Serial.println("Response data:");
      Serial.println(payload);
      ip=payload;
      
      // Process the received data as needed
      
    } else {
      Serial.print("Error in HTTP request. Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }

  Serial.println();
  Serial.println("Closing connection");
}



void sendData(float ax, float ay, float az, float gx, float gy, float gz,float temperature,float distance)
{
    StaticJsonDocument<256> doc;
    doc["ax"] = ax;
    doc["ay"] = ay;
    doc["az"] = az;
    doc["gx"] = gx;
    doc["gy"] = gy;
    doc["gz"] = gz;
    doc["temperature"] = temperature;
    doc["distance"] = distance;
    
    String postData;
    Serial.print("1 ");
    serializeJson(doc, postData);
   Serial.print(postData);
    Serial.print("\n doc \n");
  WiFiClient client;

  Serial.print("host: ");
  Serial.println(host);

  Serial.print("client.connect: ");
  //Serial.println(client.connect(host, 8080));

  if (!client.connect(ip.c_str(), 8080)) {
    Serial.println("Connection failed");
    return;
  }
 String path="/test";

  // Construct the HTTP POST request
  String httpRequest = "POST " + path + " HTTP/1.1\r\n" +
                       "Host: " + String(host) + "\r\n" +
                       "Content-Type: application/json\r\n" +
                       "Content-Length: " + String(postData.length()) + "\r\n\r\n" +
                       String(postData);
  Serial.println("httpRequest: "+httpRequest);
  const char* httpRequestChar = httpRequest.c_str();
  // Send the HTTP request
  client.print(httpRequestChar);

  // Wait for the server to respond
  delay(100);

  // Read and print the server response
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  // Close the connection
  client.stop();
}
