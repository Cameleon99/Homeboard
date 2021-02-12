

/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>


// hardcoded default wifi values, used for AP mode
#ifndef WIFISSID
#define WIFISSID "THEBOARD"
#define WIFIPSK  ""
#define WIFIMD "AP"
#endif

// Which pin on the Arduino is connected to the LEDs?
#define LED_PIN    D1 //fallback LED pin
// How many LEDs are attached to the Arduino?
#define LED_COUNT 0 // fallback LED count

// Declare our NeoPixel strip object:
//Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

Adafruit_NeoPixel strip = Adafruit_NeoPixel() ;

const char* conf = "/config.json"; // config file
const char* defaultSsid = WIFISSID; // fallback SSID
const char* defaultPassword = WIFIPSK; // fallback password
const char* defaultWifiMode = WIFIMD;
const char* defaultLedType = "NEO_GRB"; 
const char* defaultLedFreq = "NEO_KHZ800";
char usrSsid[64]; // user SSID
char usrPass[64]; // user password
char usrMode[10]; // user wifi mode
char usrLedType[10]; // user LED type
char usrLedFreq[15]; // user LED freq
int  usrLeds;     // user LED count
int  usrLedPin;     // user LED count
boolean config = false;

ESP8266WebServer server(80);

void setup(void) {
  // SETUP YOUR OUTPUT PIN AND NUMBER OF PIXELS
  String ssid = "";
  Serial.begin(115200);
  Serial.println("");
  initFS();     // initialise the filesystem
  initConfig(); // initialise the config
  initLEDs();   // initialise the LEDs
  initWifi();   // initialise the WiFi
  initHttp();   // initialuse the HTTP server
  
  // startup fun :o)
  Serial.println("INITIALISE - Startup Animation");
  for(int j=0; j<5; j++) {
    //heartThrob(20);
    heartBeat();
  }
  // Reset the LEDs to OFF
  Serial.println(F("- Blank the board"));
  for(int i=0; i<strip.numPixels(); i++) {             // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0,   0,   0));  //  Set pixel's color (in RAM)
  }
  strip.show();                                        // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void loop(void) {
  //Serial.println("In the loop");
  server.handleClient();
  MDNS.update();
}


// INIT functions

void initHttp(){
  // Main page
  server.on("/", handleBoard);
  // Internal requests
  server.on("/led", handleLed);                       // Handle LED requests
  server.on("/map", handleEditMap);                   // Handle hold/LED mapping
  server.on("/updateProblems", handleUpdateProblems); // Internal update to save file
  server.on("/updateMap", handleUpdateMap);           // Internal update to save file
  server.on("/updateConfig", handleUpdateConfig);           // Internal update to save file
  server.on("/demo", handleDemo);                     // Internal demo mode
  server.on("/problems.json", []() {serveFile("/problems.json", "application/json; charset=utf-8");} );
  server.on("/holdMap.json",  []() {serveFile("/holdMap.json", "application/json; charset=utf-8");} );
  server.on("/config.json",  []() {serveFile("/config.json", "application/json; charset=utf-8");} );
  server.on("/css/bootstrap.min.css", []() {serveFile("/css/bootstrap.min.css", "text/css; charset=utf-8");} );
  server.on("/js/bootstrap.bundle.min.js", []() {serveFile("/js/bootstrap.bundle.min.js", "application/javascript; charset=utf-8");} );
  server.on("/css/tabulator_modern.min.css", []() {serveFile("/css/tabulator_modern.min.css", "text/css; charset=utf-8");} );
  server.on("/js/tabulator.min.js", []() {serveFile("/js/tabulator.min.js", "application/javascript; charset=utf-8");} );
  server.on("/favicon.ico", []() {serveFile("/favicon.ico", "image/x-icon");} );
  server.onNotFound(handleNotFound);
  Serial.println(F("INITIALISE : HTTP Server"));
  server.begin();
  Serial.println(F("- DONE"));
}

void initLEDs(){
  // Setup LEDs - setup and blank them ASAP! 
  Serial.println(F("INITIALISE : LED strip"));
  Serial.println(F(" - LEDs : "));
  Serial.print(F("--LED count : "));
  Serial.println(usrLeds);
  Serial.print(F("--LED pin   : "));
  Serial.println(usrLedPin);
  Serial.print(F("--LED opts  : "));
  Serial.print(usrLedType);
  Serial.print("+");
  Serial.println(usrLedFreq);
  //Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, STRIPPIN, NEO_GRB + NEO_KHZ800);
  strip.updateLength(usrLeds);
  strip.setPin(usrLedPin);
  int ledType;
  int ledFreq;
  ledType = (strcmp(usrLedType, "NEO_GRB"))    ? NEO_GRB    : NEO_RGB;
  ledFreq = (strcmp(usrLedFreq, "NEO_KHZ800")) ? NEO_KHZ800 : NEO_KHZ400;
  strip.updateType(ledType + ledFreq);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.println(F("- DONE"));
}

void initConfig(){
  // Initialise Config
  Serial.println(F("INITIALISE : CONFIG"));
  // open and parse json config file
  Serial.println(F("- OPEN config file"));
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("- Failed to open config file"));
  }
  size_t confSize = configFile.size();
  Serial.print(F("- Config size: "));
  Serial.println(confSize);
  if (confSize > 1024) {
    Serial.println(F("- Config file size is too large"));
  }
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[confSize]);

  configFile.readBytes(buf.get(), confSize);

  Serial.print("JSON: ");
  Serial.println(buf.get());

  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, buf.get());
  switch (err.code()) {
    case DeserializationError::Ok:
        Serial.print(F("--Deserialization succeeded"));
        usrLeds = doc["leds"];
        usrLedPin = doc["ledPin"];
        // copy to globals
        strcpy(usrMode, doc["mode"]);
        strcpy(usrSsid, doc["ssid"]);
        strcpy(usrPass, doc["pass"]);  
        strcpy(usrLedType, doc["ledType"]);
        strcpy(usrLedFreq, doc["ledFreq"]);  
        config=true;
        break;
    case DeserializationError::InvalidInput:
        Serial.print(F("-- Invalid input!"));
        break;
    case DeserializationError::NoMemory:
        Serial.print(F("-- Not enough memory"));
        break;
    default:
        Serial.print(F("-- Deserialization failed"));
        break;
  }
  if(!config){
    Serial.print(F("- Config failed to load, use defaults"));
      strcpy(usrMode, defaultWifiMode);
      strcpy(usrSsid, defaultSsid);
      strcpy(usrPass, defaultPassword);  
      strcpy(usrLedType, defaultLedType);
      strcpy(usrLedFreq, defaultLedFreq);  
      //usrLeds = LED_COUNT;
      //usrLedPin= LED_PIN;
      
  }
  // debug
  Serial.print(F("-- LEDs: "));
  Serial.println(usrLeds);
  Serial.print(F("-- LED pin: "));
  Serial.println(usrLedPin);
  Serial.print(F("-- LED Type: "));
  Serial.println(usrLedType);
  Serial.print(F("-- LED Freq: "));
  Serial.println(usrLedFreq);
  Serial.print(F("-- SSID: "));
  Serial.println(usrSsid);
  Serial.print(F("-- Password: "));
  Serial.println(usrPass);
  Serial.print(F("-- Mode: "));
  Serial.println(usrMode);
}

void initFS(){
  // Initialise the file system 
  Serial.println(F("INITIALISE : FS"));
  if (LittleFS.begin()){
    Serial.println(F("- DONE"));
  }else{
    Serial.println(F("- FAIL"));
  }
}

void initWifi(){
  Serial.println(F("INITIALISE : WIFI"));
  boolean running = false;
  // force reset of wifi config
  WiFi.disconnect(true);
  Serial.println(F("- USER Config:"));
  Serial.print(F("-- LEDs: "));
  Serial.println(usrLeds);
  Serial.print(F("-- SSID: "));
  Serial.println(usrSsid);
  Serial.print(F("-- Password: "));
  Serial.println(usrPass);
  Serial.print(F("-- Mode: "));
  Serial.println(usrMode);
  
  if(config){
    if(strcmp(usrMode, "STA") == 0){
      Serial.println(F("- USER STA mode"));
      WiFi.mode(WIFI_STA);
      WiFi.hostname("TheBoard");
      running = WiFi.begin(usrSsid, usrPass);
      // Wait for connection
      Serial.print(F("- Connecting"));
      int retries = 20;
      while (WiFi.status() != WL_CONNECTED && retries > 0) {
        delay(1000);
        Serial.print(".");
        retries -= 1;
      }
      Serial.println(".");
      if(WiFi.status() != WL_CONNECTED){
        Serial.print(F("- WiFi connection failed : "));
        Serial.println(WiFi.status());
      }
    }else{
      Serial.println(F("- USER AP mode"));
      running = WiFi.softAP(usrSsid, usrPass);
    }
  }else{
    strcpy(usrMode,"AP");
    Serial.println(F("- DEFAULT AP mode - no config"));
    running = WiFi.softAP(defaultSsid, defaultSsid);
  }
  if(running == true){
      Serial.println(F("- Running"));
  }else{
      Serial.println(F("- WiFi FAILED!"));
  } 

  Serial.println("");
  Serial.print(usrMode=="AP" ? "- Broadcasting on: " : "- Connected to: ");
  Serial.println(usrSsid);
  Serial.print(F("- IP address: "));
  Serial.println(usrMode=="AP" ? WiFi.softAPIP() : WiFi.localIP() );

  if (MDNS.begin("THEBOARD")) {
    Serial.println(F("- MDNS responder started"));
  }
  
  Serial.println(F("- DONE"));
}


// HANDLE PAGE REQUESTS

// 404 - NOT FOUND
void handleNotFound() {
//  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/plain", message);
//  digitalWrite(led, 0);
}

void handleUpdateProblems() { //Handler for the body path
 
  if (server.hasArg("plain")== false){ //Check if body received
     server.sendHeader("Access-Control-Allow-Origin", "*");
     server.sendHeader("Access-Control-Allow-Headers", "*");
     server.send(200, "text/plain", "Body not received");
    return;
  }
 
  String message = "Body received:\n";
         message += server.arg("plain");
         message += "\n";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  File problemFile = LittleFS.open(F("/problems.json"), "w");
  problemFile.print(server.arg("plain"));
  problemFile.close();
    
  server.send(200, "text/plain", message);
  Serial.println(message);
}

void handleUpdateMap() { //Handler for the body path
 
  if (server.hasArg("plain")== false){ //Check if body received
     server.sendHeader("Access-Control-Allow-Origin", "*");
     server.sendHeader("Access-Control-Allow-Headers", "*");
     server.send(200, "text/plain", "Body not received");
    return;
  }
 
  String message = "Body received:\n";
         message += server.arg("plain");
         message += "\n";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  File mapFile = LittleFS.open(F("/holdMap.json"), "w");
  mapFile.print(server.arg("plain"));
  mapFile.close();
    
  server.send(200, "text/plain", message);
  Serial.println(message);
}

void handleUpdateConfig() { //Handler for the body path
 
  if (server.hasArg("plain")== false){ //Check if body received
     server.sendHeader("Access-Control-Allow-Origin", "*");
     server.sendHeader("Access-Control-Allow-Headers", "*");
     server.send(200, "text/plain", "Body not received");
    return;
  }
 
  String message = "Body received:\n";
         message += server.arg("plain");
         message += "\n";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  File mapFile = LittleFS.open(F("/config.json"), "w");
  mapFile.print(server.arg("plain"));
  mapFile.close();
    
  server.send(200, "text/plain", message);
  Serial.println(message);
}

void handleBoard() {
  //digitalWrite(led, 1);
  File pageFile = LittleFS.open(F("/board.html"), "r");
  String page = pageFile.readString();
  pageFile.close();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html; charset=UTF-8", page);
  //digitalWrite(led, 0);
}

void handleEditMap() {
  //digitalWrite(led, 1);
  File pageFile = LittleFS.open(F("/ledmap.html"), "r");
  String page = pageFile.readString();
  pageFile.close();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html; charset=UTF-8", page);
  //digitalWrite(led, 0);
}

void serveFile(const char* file, const char* type) {
  File pageFile = LittleFS.open(file, "r");
  //String page = pageFile.readString();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  if (server.streamFile(pageFile, type) != pageFile.size()) {};
  pageFile.close();
}

void handleDemo(){
  // startup fun :o)
  Serial.println(F("Demo animation"));
  for(int j=0; j<10; j++) {
    //heartThrob(20);
    heartBeat();
  }
  // Reset the LEDs to OFF
  Serial.println(F("Blank the board"));
  for(int i=0; i<strip.numPixels(); i++) {             // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0,   0,   0));  //  Set pixel's color (in RAM)
  }
  strip.show();                                        // Turn OFF all pixels ASAP
}

// Handle requests to light up the LEDs
void handleLed() {
  Serial.println(F("Board requested"));
  //digitalWrite(led, 1);
  //String message = "Updating board...\n\n";
  // Reset the LEDs to OFF
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0,   0,   0));         //  Set pixel's color (in RAM)
  }
  strip.show();            // Turn OFF all pixels ASAP

  int circuit = 0;
  int timeDelay = 1000;
  
  // get optional args
  for (uint8_t i = 0; i < server.args(); i++) {  
    char arg = server.argName(i).charAt(0);
    // args - test for optional arguements
    // c circuit
    // d delay (for circuits)
    switch(arg){
      case 'c':
        circuit = server.arg(i).toInt();
        break;
      case 'd':
        timeDelay = server.arg(i).toInt();
        break;
    }
  }

  Serial.println("- Circuit      : " + circuit);
  Serial.println("- Circuit delay:" + timeDelay);

  // half the default brightness for circuits
  int saturation = circuit != 1 ? 255 : 128;
  Serial.println(F("- STANDARD PROBLEM..."));
  Serial.println(F("- Set LEDs..."));
  for (uint8_t i = 0; i < server.args(); i++) {  
    char arg = server.arg(i).charAt(0);
    switch(arg){
      case 's':
        strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(0,   saturation,   0)); 
        break;
      case 'h':
        strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(0,   0,   saturation));
        break;
      case 't':
        strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(saturation,   0,   0));
        break;
    }
    strip.show();
  } 
  Serial.println(F("- DONE..."));

  // Extra logic for circuits
  if(circuit){
    Serial.println(F("- CIRCUIT..."));
    Serial.println(F("- countdown - flash holds..."));
    for (int i = 0; i < 5; i++) {
      Serial.println(F("- bright..."));
      strip.setBrightness(255);
      strip.show(); 
      delay(1000);
      Serial.println(F("- dim..."));
      strip.setBrightness(50);
      strip.show(); 
      delay(1000);
    }
    //Serial.println("- dim all...");
    //strip.setBrightness(50);
    //delay(1000);

    // get start holds and light them bright (already been dimly lit above)
    int startHoldsCount=0;
    // loop args and find start holds
    Serial.println(F("- Light start holds..."));
    for (uint8_t i = 0; i < server.args(); i++) {  
      char arg = server.arg(i).charAt(0);
      if(arg == 's'){
        // light up start holds
        strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(0,   255,   0)); 
        startHoldsCount+=1;
      }
    }
    strip.show();
    Serial.println("- wait...");
    delay(1000);

    // Loop through holds and light them up as we go
    for (uint8_t i = 0; i < server.args(); i++) {  
      int red, green, blue = 0;
      if(i > 2) strip.setPixelColor(server.argName(i-3).toInt()-1, strip.Color(0,   0,   50));   // change second last hold - dim
      char arg = server.arg(i).charAt(0);
      // skip start holds - already handled
      if(arg=='h' || arg=='t'){
        if(arg=='h'){
          // blue hand holds
          red   = 0;
          green = 0;
          blue  = 255;
        }else if(arg=='t'){
          // red end holds
          red   = 255;
          green = 0;
          blue  = 0;
        }
        // flash next hold
        for (int j=0; j<5; j++){
          strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(red, green, blue)); // turn off hold
          strip.show();
          delay(300);
          strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(red/5, green/5, blue/5)); // light up new LED
          strip.show();
          delay(300);
          strip.setPixelColor(server.argName(i).toInt()-1, strip.Color(red, green, blue)); // turn off hold
          strip.show();
        }     
        delay(timeDelay);
      }
    }
  }  // turn off other holds and leave just finish holds

  
  String message = "Board updated\n\n";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
  //digitalWrite(led, 0);
}




void heartBeat() {  
  Serial.println(F("- Animate LEDs"));
  // all pixels show the same color:
  int red =random(255);
  int green = random(255);
  int blue = random (255);
   for(int i=0; i<strip.numPixels(); i++) {             // For each pixel in strip...
     strip.setPixelColor(i, strip.Color(red, green, blue));  //  Set pixel's color (in RAM)
   }
   strip.show();
   delay (20);
      
   int x = 3;
   for (int ii = 1 ; ii <252 ; ii = ii = ii + x){
     strip.setBrightness(ii);
     strip.show();              
     delay(5);
    }
    
    x = 3;
   for (int ii = 252 ; ii > 3 ; ii = ii - x){
     strip.setBrightness(ii);
     strip.show();              
     delay(3);
     }
   delay(10);
   
   x = 6;
  for (int ii = 1 ; ii <255 ; ii = ii = ii + x){
     strip.setBrightness(ii);
     strip.show();              
     delay(2);  
     }
   x = 6;
   for (int ii = 255 ; ii > 1 ; ii = ii - x){
     strip.setBrightness(ii);
     strip.show();              
     delay(3);
   }
  delay (50);   
}
