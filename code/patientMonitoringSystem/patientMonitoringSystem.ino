
// Set-up low-level interrupts for most accurate BPM match and enable DEBUG to show ongoing commands on serial monitor
#define USE_ARDUINO_INTERRUPTS true
#define DEBUG true

// Set WiFi name, password and IP of thingspeak.com
#define SSID "GiovasNewNet" // SSID-WiFiname
#define PASS "15089496" // WiFi Password
#define IP "184.106.153.149" // thingspeak.com IP

// include libraries
#include <SoftwareSerial.h>
#include "Timer.h"
#include <PulseSensorPlayground.h> // include the PulseSensorPlayground library

// instance for timer, SoftwareSerial and pulse sensor
Timer t;
PulseSensorPlayground pulseSensor;
SoftwareSerial esp8266(10,11); // Rx, Tx

// update information on ThingSpeak channel
String msg = "GET /update?key=F2ZACECNWPR0IXXD";

// variables
const int PulseWire = A0; // pulseSensor PURPLE wire connected to ANALOG pin 0
const int LED13 = 13; // the on-board Arduino LED close to pin 13
int Threshold = 550; // for heart rate sensor
float myTemp;
int myBPM;
String BPM;
int error;
int panic;
int raw_myTemp;
float Voltage;
float tempC;

void setup() {
  // set the baud rate for serial communication between Arduino serial monitor and esp8266
  Serial.begin(9600);
  esp8266.begin(115200);
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13); // auto blink Arduino's LED with heartbeat
  pulseSensor.setThreshold(Threshold);

  // double-check the pulseSensor object was created and began seeing a signal
  if (pulseSensor.begin()){
    Serial.println("pulseSensor object created!"); // this prints one time at Arduino power-up or on Arduino reset
  }
  
  // start the ESP communication by giving AT command to it and connect it by calling connectWiFi() function
  Serial.println("AT");
  esp8266.println("AT");
  delay(3000);

  if (esp8266.find("OK")) {
    connectWiFi();
  }
  
  // initialize timers by calling t.every(time_interval, do_this) which will take the readings of the sensors and update on the channel after every time_interval defined
  t.every(10000, getReadings);
  t.every(10000, updateInfo);
}

void loop() {
  // call panic_button() and timers 
  panic_button();
  start: //label
    error=0;
    t.update();
    // resend if transmission is not completed
    if (error == 1) {
      goto start; // go to label "start"
    }
  delay(4000);
}

// function for updating sensor information on the ThingSpeak channel
// AT command will establish TCP command over port 80
void updateInfo() {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  Serial.println(cmd);
  esp8266.println(cmd);
  delay(2000);
  if (esp8266.find("Error")) {
    return;
  }

  // attach the readings with the GET URL using "&field1="; for pulse readings 
  // and "&field2="; for temperature readings. 
  // Send this information using “AT+CIPSEND=” command.
  cmd = msg ;
  cmd += "&field1="; //field 1 for BPM
  cmd += BPM;
  cmd += "&field2="; //field 2 for temperature
  cmd += temp;
  cmd += "\r\n";
  Serial.print("AT+CIPSEND=");
  esp8266.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  esp8266.println(cmd.length());
  if (esp8266.find(">")) {
    Serial.print(cmd);
    esp8266.print(cmd);
  }
  else {
    Serial.println("AT+CIPCLOSE");
    esp8266.println("AT+CIPCLOSE");
    // resend
    error = 1;
  }
}

// function for connectWiFi() which will return True or False depending upon WiFi connected or not
// AT+CWMODE=1 will make ESP8266 work in station mode
// AT+CWJAP=\ used to connect to Access Point (WiFi router)
boolean connectWiFi() {
  Serial.println("AT+CWMODE=1");
  esp8266.println("AT+CWMODE=1");
  delay(2000);
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"";
  Serial.println(cmd);
  esp8266.println(cmd);
  delay(5000);
  if (esp8266.find("OK")) {
    return true;
  }
  else {
    return false;
  }
}

// function to take pulse sensor and LM35 readings and convert them to String using dtostrf() function
void getReadings() {
  raw_myTemp = analogRead(A1);
  Voltage = (raw_myTemp / 1023.0) * 5000; // 5000 to get millivolts
  tempC = Voltage * 0.1; // in degree Celsius
  myTemp = (tempC * 1.8) + 32; // convert to Fahrenheit
  Serial.println(myTemp);
  int myBPM = pulseSensor.getBeatsPerMinute(); // calls function on our pulseSensor object that returns BPM as int
  if (pulseSensor.sawStartOfBeat()) { // constantly test to see if a beat happened
    Serial.println(myBPM); // print the value inside of myBPM
  }
  delay(20);

  // define char array for BPM and temp and convert float value of these sensors to String
  char buffer1[10];
  char buffer2[10];
  BPM = dtostrf(myBPM, 4, 1, buffer1);
  temp = dtostrf(myTemp, 4, 1, buffer2);
}

// function for panic button
// when button goes to HIGH, esp8266 send the information to the server 
void panic_button() {
  panic = digitalRead(8);
  if(panic == HIGH){
    Serial.println(panic);
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += IP;
    cmd += "\",80";
    Serial.println(cmd);
    esp8266.println(cmd);
    delay(2000);
    if (esp8266.find("Error")) {
      return;
    }
    cmd = msg ;
    cmd += "&field3=";
     cmd += panic;
    cmd += "\r\n";
    Serial.print("AT+CIPSEND=");
    esp8266.print("AT+CIPSEND=");
    Serial.println(cmd.length());
    esp8266.println(cmd.length());
    if (esp8266.find(">")) {
      Serial.println(cmd);
      esp8266.print(cmd);
    }
    else {
      Serial.println("AT+CIPCLOSE");
      esp8266.println("AT+CIPCLOSE");
      // resend
      error=1;
    }
  }
}
