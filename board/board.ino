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
#include <LittleFS.h> // LittleFS is declared
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#ifndef STASSID
#define STASSID ""
#define STAPSK  ""
#endif

// Which pin on the Arduino is connected to the LEDs?
//#define LED_PIN    D1
// How many LEDs are attached to the Arduino?
//#define LED_COUNT 60



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


const char *defaultSsid = STASSID;
const char *defaultPassword = STAPSK;

ESP8266WebServer server(80);

const int led = 13;

void setup(void) {
  
  // SETUP YOUR OUTPUT PIN AND NUMBER OF PIXELS
  String ssid = "";

  Serial.println("");
  Serial.println("INITIALISE : LED strip");
  //Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, STRIPPIN, NEO_GRB + NEO_KHZ800);
  strip.updateLength(55);
  strip.setPin(D1);
  strip.updateType(NEO_RGB + NEO_KHZ800);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.println("- DONE");

  Serial.println(F("INITIALISE : FS"));
  if (LittleFS.begin()){
    Serial.println(F("- DONE"));
  }else{
    Serial.println(F("- FAIL"));
  }
  
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  boolean config = true;
  Serial.println("INITIALISE : WIFI");
  if(config){
    ssid = defaultSsid;
    Serial.println("- STA mode - config found");
    WiFi.mode(WIFI_STA);
    WiFi.hostname("TheBoard");
    WiFi.begin(ssid, defaultPassword);
    Serial.println("- Running");
  }else{
    ssid = "TheBoard";
    Serial.println("- AP mode - no config");
    WiFi.softAP(ssid, "");
    Serial.println("- Running");
  }

  

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("- Connected to ");
  Serial.println(ssid);
  Serial.print("- IP address: ");
  Serial.println(WiFi.localIP());

  //if (WiFi.status() == WL_CONNECTED) {
  //    WiFi.hostname("TheBoard");
  //}

  if (MDNS.begin("THEBOARD")) {
    Serial.println("- MDNS responder started");
  }
  
  Serial.println("- DONE");
  // Main page
  server.on("/", handleBoard);
  // Internal requests
  server.on("/led", handleLed);                       // Handle LED requests
  server.on("/map", handleEditMap);                   // Handle hold/LED mapping
  server.on("/updateProblems", handleUpdateProblems); // Internal update to save file
  server.on("/updateMap", handleUpdateMap);           // Internal update to save file
  server.on("/demo", handleDemo);                     // Internal demo mode
  server.on("/problems.json", []() {serveFile("/problems.json", "application/json; charset=utf-8");} );
  server.on("/holdMap.json",  []() {serveFile("/holdMap.json", "application/json; charset=utf-8");} );
  server.on("/css/bootstrap.min.css", []() {serveFile("/css/bootstrap.min.css", "text/css; charset=utf-8");} );
  server.on("/js/bootstrap.bundle.min.js", []() {serveFile("/js/bootstrap.bundle.min.js", "application/javascript; charset=utf-8");} );
  server.on("/css/tabulator_modern.min.css", []() {serveFile("/css/tabulator_modern.min.css", "text/css; charset=utf-8");} );
  server.on("/js/tabulator.min.js", []() {serveFile("/js/tabulator.min.js", "application/javascript; charset=utf-8");} );
  server.on("/favicon.ico", []() {serveFile("/favicon.ico", "image/x-icon");} );
  server.onNotFound(handleNotFound);
  Serial.println("INITIALISE : HTTP Server");
  server.begin();
  Serial.println("- DONE");

  // startup fun :o)
  Serial.println("INITIALISE - Startup Animation");
  for(int j=0; j<5; j++) {
    //heartThrob(20);
    heartBeat();
  }
  // Reset the LEDs to OFF
  Serial.println("- Blank the board");
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

// HANDLE DIFFERENT PAGES

void handleNotFound() {
  digitalWrite(led, 1);
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
  digitalWrite(led, 0);
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

//void handleProblems() {
//  //digitalWrite(led, 1);
//  File pageFile = LittleFS.open(F("/problems.json"), "r");
//  String page = pageFile.readString();
//  pageFile.close();
//  server.sendHeader("Access-Control-Allow-Origin", "*");
//  server.send(200, " application/json", page);
//  //digitalWrite(led, 0);
//}

//void handleMap() {
//  //digitalWrite(led, 1);
//  File pageFile = LittleFS.open(F("/holdMap.json"), "r");
//  String page = pageFile.readString();
//  pageFile.close();
//  server.sendHeader("Access-Control-Allow-Origin", "*");
//  server.send(200, " application/json", page);
//  //digitalWrite(led, 0);
//}

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
  Serial.println("Demo animation");
  for(int j=0; j<10; j++) {
    //heartThrob(20);
    heartBeat();
  }
  // Reset the LEDs to OFF
  Serial.println("Blank the board");
  for(int i=0; i<strip.numPixels(); i++) {             // For each pixel in strip...
    strip.setPixelColor(i, strip.Color(0,   0,   0));  //  Set pixel's color (in RAM)
  }
  strip.show();                                        // Turn OFF all pixels ASAP
}

// Handle requests to light up the LEDs
void handleLed() {
  Serial.println("Board requested");
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
  Serial.println("- STANDARD PROBLEM...");
  Serial.println("- Set LEDs...");
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
  Serial.println("- DONE...");

  // Extra logic for circuits
  if(circuit){
    Serial.println("- CIRCUIT...");
    Serial.println("- countdown - flash holds...");
    for (int i = 0; i < 5; i++) {
      Serial.println("- bright...");
      strip.setBrightness(255);
      strip.show(); 
      delay(1000);
      Serial.println("- dim...");
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
    Serial.println("- Light start holds...");
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
  Serial.println("- Animate LEDs");
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
