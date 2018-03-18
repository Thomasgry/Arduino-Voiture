#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <FS.h>
#define LEDUP 14
#define PWMRANGE 1023

float VERSION = 0.1;
ESP8266WebServer server(80);
int left  = 0;
int top   = 0;
int speed =0;
int speed2 =0;

String realSize = String(ESP.getFlashChipRealSize());
String ideSize = String(ESP.getFlashChipSize());
bool flashCorrectlyConfigured = realSize.equals(ideSize);

const int IN1 = 14;
const int IN2 = 12;
const int IN3 = 13;
const int IN4 = 15;

const int ENA = 5;
const int ENB = 4;

int _timeKeepAlive  = 0;
int KeepAlive  = 2 * 1000;
int time_delay =200;

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
}

void handleSetPosition(){
  _timeKeepAlive  = round(millis()/1000);
  // get position
  left  = server.arg("left").toInt();
  Serial.println("left = " + String(left));
  top  = server.arg("top").toInt(); 
  Serial.println("top = " + String(top));
  
  //set speed
  speed = int(abs(top)*512/100) + 512;
  speed2 = int(speed - speed * (abs(left))/100);
  Serial.print("ENA = " + String(speed));
  Serial.println(" ENB = " + String(speed2));
  
  String txt="";
  if (top == 0){
    stopMotor();
    //forward
  } else if (top > 0){
    // motor A
    digitalWrite(IN1,HIGH);
    digitalWrite(IN2,LOW); 
    // motor B
    digitalWrite(IN3,HIGH);
    digitalWrite(IN4,LOW);
    
    if ( abs(left) < 20){
      //speed
      analogWrite(ENA,speed);
      analogWrite(ENB,speed);
    } else if ( left <= -20 && left >= -90 ){
      analogWrite(ENA,speed);
      analogWrite(ENB,speed2);
    } else if ( left >= 20 && left <= 90){
      analogWrite(ENA,speed2);
      analogWrite(ENB,speed);
    } else if ( left <= -90){
      analogWrite(ENA,speed);
      analogWrite(ENB,LOW);
    } else {
      analogWrite(ENB,speed);
      analogWrite(ENA,LOW);
    }
    //backward
  } else {
    // motor A
    digitalWrite(IN1,LOW); 
    digitalWrite(IN2,HIGH); 
    // motor B
    digitalWrite(IN3,LOW); 
    digitalWrite(IN4,HIGH); 
    
    if ( left > -20 && left < 20){
      analogWrite(ENA,speed);
      analogWrite(ENB,speed);
      //turn left
    } else if ( left <= -20 && left >= -90 ){
      analogWrite(ENA,speed2);
      analogWrite(ENB,speed);
      //turn right
    } else if ( left >= 20 && left <= 90){
      analogWrite(ENA,speed);
      analogWrite(ENB,speed2);
    } else if ( left <= -90){
      digitalWrite(ENA,LOW);
      analogWrite(ENB,speed);
    } else {
      analogWrite(ENA,speed);
      digitalWrite(ENB,LOW);
    }
  }
   server.send ( 200, "text/plain", "done speed:" + String(speed) +"/"+String(speed2) + " gpio: "+txt );
}

void handletest(){
  server.send ( 200, "text/plain", "test !" );
}
void setup() {
  Serial.begin(115200);
  Serial.println("Startup");

  pinMode(LEDUP,OUTPUT);
  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);

  pinMode(ENA,OUTPUT);
  pinMode(ENB,OUTPUT);
  
  digitalWrite(LEDUP,LOW);
  stopMotor();
  
   if(flashCorrectlyConfigured) SPIFFS.begin();
   else Serial.println("flash incorrectly configured, SPIFFS cannot start, IDE size: " + ideSize + ", real size: " + realSize);

  // Lecture de la memoire SPIFFS
  if (!SPIFFS.begin()){
    Serial.println("SPIFFS Mount failed");
  } else {
    Serial.println("SPIFFS Mount succesfull");
  }
  Serial.print("here1");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
  }

  WiFi.disconnect();
  WiFi.softAP("ESP8266Voiture", "");
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/jquery-1.11.3.min.js", SPIFFS, "/jquery-1.11.3.min.js");
  server.serveStatic("/jquery-ui.min.js", SPIFFS, "/jquery-ui.min.js");
  server.serveStatic("/jquery.ui.touch-punch.min.js", SPIFFS, "/jquery.ui.touch-punch.min.js");
  server.serveStatic("/cible.png", SPIFFS, "/cible.png");
  server.serveStatic("/voiture.png", SPIFFS, "/voiture.png");
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.on ( "/car", handleSetPosition );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println("Ready");
  //digitalWrite(LEDUP,HIGH);
}
void stopMotor(){
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
}
void loop() {
 server.handleClient();
 if ((millis()/1000 - _timeKeepAlive) >KeepAlive){
    _timeKeepAlive  = round(millis()/1000);
    stopMotor();
  }
}
