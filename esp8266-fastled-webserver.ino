/*
   ESP8266 FastLED WebServer: https://github.com/jasoncoon/esp8266-fastled-webserver
   Copyright (C) 2015-2018 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//#define FASTLED_ALLOW_INTERRUPTS 0
//#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <FastLED.h>
FASTLED_USING_NAMESPACE

extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <EEPROM.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

WiFiManager wifiManager;
ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdateServer;

// Comment out to disable WS
WebSocketsServer webSocketsServer = WebSocketsServer(81);

#include "GradientPalettes.h"
#include "Field.h"
#include "FSBrowser.h"

// LED STRIP PARAMETERS
#define DATA_PIN      D4
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      300

// IMPORTANT: set the max milli-amps of your power supply (4A = 4000mA)
#define MILLI_AMPS         18000
// Animation speed. With the Access Point / Web Server the animations run a bit slower.
#define FRAMES_PER_SECOND  120

//////////////////////////////////////////////////
//////////// DEFAULT SETTINGS & STUFF ////////////
//////////////////////////////////////////////////

CRGB leds[NUM_LEDS];
const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
uint8_t brightnessIndex = 0;
uint8_t secondsPerPalette = 10;
uint8_t cooling = 50;
uint8_t sparking = 60;
uint8_t speed = 30;
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
uint8_t gCurrentPaletteNumber = 0;
uint8_t currentGradientPaletteIndex = 0;
CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );
CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);
uint8_t currentPatternIndex = 0; // Index number of which pattern is current
uint8_t autoplay = 0;
uint8_t autoplayDuration = 10;
unsigned long autoPlayTimeout = 0;
uint8_t currentPaletteIndex = 0;
uint8_t gHue = 0;
int meteorCounter = 0;
CRGB solidColor = CRGB::Blue;
String nameString;

/////////////////////////////////////////////
//////////// PATTERN DEFINITIONS ////////////
/////////////////////////////////////////////

// Needs at least one dummy function prototype here or it wont compile at all
void null() {};

typedef void (*Pattern)();
typedef Pattern PatternList[];

typedef struct {
  Pattern pattern;
  String name;
} PatternAndName;

typedef PatternAndName PatternAndNameList[];

// Include data for patterns & functions
#include "Twinkles.h"
#include "TwinkleFOX.h"

// List of available patterns (effects)
PatternAndNameList patterns = {
  { pride,                  "Pride" },
  { colorWaves,             "Color Waves" },
  { gradientSweeper,        "Gradient Sweep" },
  { gradientShowcase,       "Gradient Showcase" },

  // Twinkle patterns
  { rainbowTwinkles,        "Rainbow Twinkles" },
  { snowTwinkles,           "Snow Twinkles" },
  { cloudTwinkles,          "Cloud Twinkles" },
  { incandescentTwinkles,   "Incandescent Twinkles" },

  // TwinkleFOX patterns
  { retroC9Twinkles,        "Retro C9 Twinkles" },
  { redWhiteTwinkles,       "Red & White Twinkles" },
  { blueWhiteTwinkles,      "Blue & White Twinkles" },
  { blueIncandescentTwinkles,      "Blue & Incandescent Twinkles" },
  { redGreenBlueTwinkles,   "Red & Green & Blue Twinkles" },
  { redGreenWhiteTwinkles,  "Red, Green & White Twinkles" },
  { fairyLightTwinkles,     "Fairy Light Twinkles" },
  { easterTwinkles,         "Easter Twinkles" },
  { snow2Twinkles,          "Snow 2 Twinkles" },
  { hollyTwinkles,          "Holly Twinkles" },
  { iceTwinkles,            "Ice Twinkles" },
  { partyTwinkles,          "Party Twinkles" },
  { forestTwinkles,         "Forest Twinkles" },
  { lavaTwinkles,           "Lava Twinkles" },
  { fireTwinkles,           "Fire Twinkles" },
  { cloud2Twinkles,         "Cloud 2 Twinkles" },
  { oceanTwinkles,          "Ocean Twinkles" },

  { rainbow,                "Rainbow" },
  { rainbowWithGlitter,     "Rainbow With Glitter" },
  { sparkles,               "White Sparkles" },
  { rainbowSolid,           "Solid Rainbow" },
  { confetti,               "Confetti" },
  { sinelon,                "Sinelon" },
  { bpm,                    "Beat" },
  { juggle,                 "Juggle" },
  { fire,                   "Fire" },
  { water,                  "Water" },
  { pacifica_loop,          "Pacifica Loop" },
  { night_lake,             "Midnight Lake" },
  { whiteMeteor,			"Meteor Rain" },
  { colorfulMeteor,         "Colorful Meteor Rain" },

  { showSolidColor,         "Solid Color" }
};

// Get pattern count from array size
const uint8_t patternCount = ARRAY_SIZE(patterns);

///////////////////////////////////////////////////
//////////// COLOR PALETTE DEFINITIONS ////////////
///////////////////////////////////////////////////

typedef struct {
  CRGBPalette16 palette;
  String name;
} PaletteAndName;

typedef PaletteAndName PaletteAndNameList[];

// Color palette function names
const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};

// Get color palette count
const uint8_t paletteCount = ARRAY_SIZE(palettes);

// Define color palette names
const String paletteNames[paletteCount] = {
  "Rainbow",
  "Rainbow Stripe",
  "Cloud",
  "Lava",
  "Ocean",
  "Forest",
  "Party",
  "Heat"
};

// Include fields
#include "Fields.h"

//////////////////////////////////////////////////////////////////
/////////////////////// MAIN SETUP FUNCTION //////////////////////
//////////////////////////////////////////////////////////////////

void setup() {
  WiFi.mode(WIFI_STA);
  // Do not allow Wi-Fi module to sleep
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.begin(115200);
  Serial.setDebugOutput(false);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); // for WS2812 (Neopixel)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);

  // Set color to black
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Show strip
  FastLED.show();

  // Initialize EEPROM and load saved settings from it
  EEPROM.begin(512);
  loadSettings();

  // Set brightness again after obtaining it from EEPROM
  FastLED.setBrightness(brightness);

  // Print useful debug info to console
  Serial.println();
  Serial.print( F("Heap: ") ); Serial.println(system_get_free_heap_size());
  Serial.print( F("Boot Vers: ") ); Serial.println(system_get_boot_version());
  Serial.print( F("CPU: ") ); Serial.println(system_get_cpu_freq());
  Serial.print( F("SDK: ") ); Serial.println(system_get_sdk_version());
  Serial.print( F("Chip ID: ") ); Serial.println(system_get_chip_id());
  Serial.print( F("Flash ID: ") ); Serial.println(spi_flash_get_id());
  Serial.print( F("Flash Size: ") ); Serial.println(ESP.getFlashChipRealSize());
  Serial.print( F("Vcc: ") ); Serial.println(ESP.getVcc());
  Serial.println();

  // Initialize file system and print its contents to console
  SPIFFS.begin();
  {
    Serial.println("SPIFFS contents:");

    Dir dir = SPIFFS.openDir("/");
    
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }
    
    Serial.printf("\n");
  }

  // Do a little work to get a unique-ish name. Get the last two bytes of the MAC (HEX'd)
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();

  nameString = "ESP8266-" + macID;

  char nameChar[nameString.length() + 1];
  memset(nameChar, 0, nameString.length() + 1);

  for (int i = 0; i < nameString.length(); i++) {
    nameChar[i] = nameString.charAt(i);
  }

  Serial.printf("Name: %s\n", nameChar );

  // Uncomment to wipe WiFi credentials for testing purpose
  // wifiManager.resetSettings();

  wifiManager.setConfigPortalBlocking(false);

  // Automatically connect to WiFi using saved credentials if they exist
  // If connection fails then start an access point with the specified name
  if(wifiManager.autoConnect(nameChar)) {
    Serial.println("Wi-Fi connected");
  }
  else {
    Serial.println("Wi-Fi manager portal running");
  }

  ////////////////////////////////////////////////////////////////////
  ///////////////////// WEB SERVER RELATED STUFF /////////////////////
  ////////////////////////////////////////////////////////////////////

  httpUpdateServer.setup(&webServer);

  webServer.on("/all", HTTP_GET, []() {
    String json = getFieldsJson(fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", json);
  });

  webServer.on("/fieldValue", HTTP_GET, []() {
    String name = webServer.arg("name");
    String value = getFieldValue(name, fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", value);
  });

  webServer.on("/fieldValue", HTTP_POST, []() {
    String name = webServer.arg("name");
    String value = webServer.arg("value");
    String newValue = setFieldValue(name, value, fields, fieldCount);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/json", newValue);
  });

  webServer.on("/power", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPower(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(power);
  });

  webServer.on("/cooling", HTTP_POST, []() {
    String value = webServer.arg("value");
    cooling = value.toInt();
    broadcastInt("cooling", cooling);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(cooling);
  });

  webServer.on("/sparking", HTTP_POST, []() {
    String value = webServer.arg("value");
    sparking = value.toInt();
    broadcastInt("sparking", sparking);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(sparking);
  });

  webServer.on("/speed", HTTP_POST, []() {
    String value = webServer.arg("value");
    speed = value.toInt();
    broadcastInt("speed", speed);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(speed);
  });

  webServer.on("/twinkleSpeed", HTTP_POST, []() {
    String value = webServer.arg("value");
    twinkleSpeed = value.toInt();
    
    if (twinkleSpeed < 2) {
      twinkleSpeed = 2;
    }
    else if (twinkleSpeed > 7) {
      twinkleSpeed = 7;
    }
    
    setTwinkleSpeed(twinkleSpeed);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(twinkleSpeed);
  });

  webServer.on("/twinkleDensity", HTTP_POST, []() {
    String value = webServer.arg("value");
    twinkleDensity = value.toInt();
    
    if (twinkleDensity < 1) {
      twinkleDensity = 1;
    }
    else if (twinkleDensity > 8) {
      twinkleDensity = 8;
    }
    
    setTwinkleDensity(twinkleDensity);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(twinkleDensity);
  });

  webServer.on("/fadeInSpeed", HTTP_POST, []() {
    String value = webServer.arg("value");
    fadeInSpeed = value.toInt();
    
    if (fadeInSpeed < 16) {
      fadeInSpeed = 16;
    }
    else if (fadeInSpeed > 128) {
      fadeInSpeed = 128;
    }
    
    setFadeInSpeed(fadeInSpeed);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(fadeInSpeed);
  });

  webServer.on("/fadeOutSpeed", HTTP_POST, []() {
    String value = webServer.arg("value");
    fadeOutSpeed = value.toInt();
    
    if (fadeOutSpeed < 16) {
      fadeOutSpeed = 16;
    }
    else if (fadeOutSpeed > 128) {
      fadeOutSpeed = 128;
    }
    
    setFadeOutSpeed(fadeOutSpeed);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(fadeOutSpeed);
  });

  webServer.on("/solidColor", HTTP_POST, []() {
    String r = webServer.arg("r");
    String g = webServer.arg("g");
    String b = webServer.arg("b");
    setSolidColor(r.toInt(), g.toInt(), b.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendString(String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
  });

  webServer.on("/pattern", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPattern(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(currentPatternIndex);
  });

  webServer.on("/patternName", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPatternName(value);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(currentPatternIndex);
  });

  webServer.on("/palette", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPalette(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(currentPaletteIndex);
  });

  webServer.on("/paletteName", HTTP_POST, []() {
    String value = webServer.arg("value");
    setPaletteName(value);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(currentPaletteIndex);
  });

  webServer.on("/gradientPalette", HTTP_POST, []() {
    String value = webServer.arg("value");
    setGradientPalette(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(currentGradientPaletteIndex);
  });

  webServer.on("/gradientPaletteName", HTTP_POST, []() {
    String value = webServer.arg("value");
    setGradientPaletteName(value);
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(currentGradientPaletteIndex);
  });

  webServer.on("/brightness", HTTP_POST, []() {
    String value = webServer.arg("value");
    setBrightness(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(brightness);
  });

  webServer.on("/autoplay", HTTP_POST, []() {
    String value = webServer.arg("value");
    setAutoplay(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(autoplay);
  });

  webServer.on("/autoplayDuration", HTTP_POST, []() {
    String value = webServer.arg("value");
    setAutoplayDuration(value.toInt());
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    sendInt(autoplayDuration);
  });

  webServer.on("/wifi", HTTP_GET, []() {
    String wjson = getWiFiJson();
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "application/json", wjson);
  });

  // List directory contents
  webServer.on("/list", HTTP_GET, handleFileList);
  
  // Load SPIFFS editor
  webServer.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      webServer.sendHeader("Access-Control-Allow-Origin", "*");
      webServer.send(404, "text/plain", "FileNotFound");
    }
  });
  
  // Create file
  webServer.on("/edit", HTTP_PUT, handleFileCreate);
  
  // Delete file
  webServer.on("/edit", HTTP_DELETE, handleFileDelete);

  // Handle uploads in editor
  webServer.on("/edit", HTTP_POST, []() {
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/plain", "");
  }, handleFileUpload);

  webServer.serveStatic("/", SPIFFS, "/", "max-age=86400");

  // Start mDNS
  MDNS.begin(nameChar);
  MDNS.setHostname(nameChar);

  // Start HTTP web server
  webServer.begin();
  Serial.println("HTTP web server started");

  // Start WS server. Comment out to disable WS!
  webSocketsServer.begin();
  webSocketsServer.onEvent(webSocketEvent);
  Serial.println("Web socket server started");

  autoPlayTimeout = millis() + (autoplayDuration * 1000);
}

//////////////////////////////////////////////////////////////////////////
/////////////////////// WEB SERVER HELPER FUNCTIONS //////////////////////
//////////////////////////////////////////////////////////////////////////

void sendInt(uint8_t value) {
  sendString(String(value));
}

void sendString(String value) {
  webServer.send(200, "text/plain", value);
}

void broadcastInt(String name, uint8_t value) {
  String json = "{\"name\":\"" + name + "\",\"value\":" + String(value) + "}";
  
  // Comment out to disable WS
  webSocketsServer.broadcastTXT(json);
}

void broadcastString(String name, String value) {
  String json = "{\"name\":\"" + name + "\",\"value\":\"" + String(value) + "\"}";

  // Comment out to disable WS
  webSocketsServer.broadcastTXT(json);
}

//////////////////////////////////////////////////////////////////////////
/////////////////////// MAIN LOOP ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void loop() {
  static bool hasConnected = false;
  
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  // Comment out to disable WS
  webSocketsServer.loop();

  wifiManager.process();
  webServer.handleClient();
  MDNS.update();

  // Set color to black if power is turned off
  if (power == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }

  // Check if Wi-Fi is connected and attempt reconnect if it is not
  EVERY_N_SECONDS(1) {
    if (WiFi.status() != WL_CONNECTED) {
      hasConnected = false;
    }
    else if (!hasConnected) {
      hasConnected = true;
      MDNS.begin(nameString);
      MDNS.setHostname(nameString);
      webServer.begin();
      Serial.println("HTTP web server started");
      Serial.print("Connected! Open http://");
      Serial.print(WiFi.localIP());
      Serial.print(" or http://");
      Serial.print(nameString);
      Serial.println(".local in your browser");
    }
  }

  // Change to a new cpt-city gradient palette
  EVERY_N_SECONDS( secondsPerPalette ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  // Slowly blend the current palette to the next palette
  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 8);
    gHue++;
  }

  // Switch pattern if autoplay mode is active
  if (autoplay && (millis() > autoPlayTimeout)) {
    adjustPattern(true);
    autoPlayTimeout = millis() + (autoplayDuration * 1000);
  }

  // Call the current pattern function once, updating the 'leds' array
  patterns[currentPatternIndex].pattern();

  // Display the LED strip
  FastLED.show();

  // Insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

//////////////////////////////////////////////////////////////////////////
/////////////////////// WEBSOCKET FUNCTIONALITY //////////////////////////
//////////////////////////////////////////////////////////////////////////

// WebSockets provide feedback - change value on one screen and it changes on the other screen (browser window or device) too. 
// Comment out to disable WS.
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      Serial.printf("[%u] Disconnected!\n", num);
    }
    break;

    case WStype_CONNECTED: {
      IPAddress ip = webSocketsServer.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // Send message to client
        // webSocketsServer.sendTXT(num, "Connected");
    }
    break;

    case WStype_TEXT: {
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // Send message to client
      // webSocketsServer.sendTXT(num, "message here");

      // Send data to all connected clients
      // webSocketsServer.broadcastTXT("message here");
    }
    break;

    case WStype_BIN: {
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);

      // Send message to client
      // webSocketsServer.sendBIN(num, payload, length);
    }
    break;
  }
}

///////////////////////////////////////////////////////////////////////
/////////////////////// WIFI STATUS MONITORING ////////////////////////
///////////////////////////////////////////////////////////////////////

String getWiFiJson() {
  String json = "{";
  json += "\"status\":\"" + String(WiFi.status()) + "\"";
  json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
  json += ",\"password\":\"" + String(WiFi.psk()) + "\"";
  json += ",\"channel\":\"" + String(WiFi.channel()) + "\"";
  json += ",\"rssi\":\"" + String(WiFi.RSSI()) + "\"";
  json += ",\"bssid\":\"" + WiFi.BSSIDstr() + "\"";
  json += ",\"mac\":\"" + String(WiFi.macAddress()) + "\"";
  json += ",\"localIP\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"netmask\":\"" + WiFi.subnetMask().toString() + "\"";
  json += ",\"gateway\":\"" + WiFi.gatewayIP().toString() + "\"";
  json += ",\"hostname\":\"" + String(WiFi.hostname()) + "\"";
  json += ",\"dns1\":\"" + WiFi.dnsIP(0).toString() + "\"";
  json += ",\"dns2\":\"" + WiFi.dnsIP(1).toString() + "\"";
  json += ",\"softAPIP\":\"" + WiFi.softAPIP().toString() + "\"";
  json += "}";

  return json;
} 

//////////////////////////////////////////////////////////////////////////
/////////////////////// READ SETTINGS FROM EEPROM ////////////////////////
//////////////////////////////////////////////////////////////////////////

// Read settings (previous values) saved to EEPROM. If some values go out of bounds, they will be reset on startup.
// Reasons for reset can be no values set (after reflash) or invalid values set (via REST API)
void loadSettings() {
  brightness = EEPROM.read(0);

  currentPatternIndex = EEPROM.read(1);
  if (currentPatternIndex < 0) {
    currentPatternIndex = 0;
  }
  else if (currentPatternIndex >= patternCount) {
    currentPatternIndex = 0; //Load 'Pride' effect if out of bounds
  }
  
  byte r = EEPROM.read(2);
  byte g = EEPROM.read(3);
  byte b = EEPROM.read(4);

  if (!(r == 0 && g == 0 && b == 0)) {
    solidColor = CRGB(r, g, b);
  }

  power = EEPROM.read(5);
  autoplay = EEPROM.read(6);

  autoplayDuration = EEPROM.read(7);
  if (autoplayDuration < 10) {
    autoplayDuration = 10;
  }
  else if (autoplayDuration > 250) {
    autoplayDuration = 120; //Load default autoplay duration as 120 (2 minutes) if out of bounds
  }

  currentPaletteIndex = EEPROM.read(8);
  if (currentPaletteIndex < 0) {
    currentPaletteIndex = 0;
  }
  else if (currentPaletteIndex >= paletteCount) {
    currentPaletteIndex = 0; //Load 'Rainbow' palette if out of bounds
  }

  currentGradientPaletteIndex = EEPROM.read(9);
  if (currentGradientPaletteIndex < 0) {
    currentGradientPaletteIndex = 0;
  }
  else if (currentGradientPaletteIndex >= gGradientPaletteCount) {
    currentGradientPaletteIndex = 0; //Load 'Sunset' gradient if out of bounds
  }

  fadeInSpeed = EEPROM.read(10);
  if (fadeInSpeed < 16) {
    fadeInSpeed = 16;
  }
  else if (fadeInSpeed > 128) {
    fadeInSpeed = 75; //Set 'Twinkles' fade-in speed to 75 if out of bounds
  }
  
  fadeOutSpeed = EEPROM.read(11);
  if (fadeOutSpeed < 16) {
    fadeOutSpeed = 16;
  }
  else if (fadeOutSpeed > 128) {
    fadeOutSpeed = 65; //Set 'Twinkles' fade-out speed to 65 if out of bounds
  }

  twinkleSpeed = EEPROM.read(12);
  if (twinkleSpeed < 2) {
    twinkleSpeed = 2;
  }
  else if (twinkleSpeed > 7) {
    twinkleSpeed = 4; //Set 'Twinkles' speed to 4 if out of bounds
  }

  twinkleDensity = EEPROM.read(13);
  if (twinkleDensity < 1) {
    twinkleDensity = 1;
  }
  else if (twinkleDensity > 8) {
    twinkleDensity = 5; //Set 'Twinkles' density to 5 if out of bounds
  }
}

//////////////////////////////////////////////////////////////////////////
/////////////////////// SETTING / EEPROM FUNCTIONS ///////////////////////
//////////////////////////////////////////////////////////////////////////

// Set brightness, write to EEPROM
void setBrightness(uint8_t value) {
  if (value > 255) {
    value = 255;
  }
  else if (value < 0) {
    value = 0;
  }

  brightness = value;

  FastLED.setBrightness(brightness);

  EEPROM.write(0, brightness);
  EEPROM.commit();

  broadcastInt("brightness", brightness);
}

// Increase or decrease the current pattern number, and wrap around at the ends
void adjustPattern(bool up) {
  if (up) {
    currentPatternIndex++;
  }
  else {
    currentPatternIndex--;
  }
  
  // Wrap around at the ends
  if (currentPatternIndex < 0) {
    currentPatternIndex = patternCount - 1;
  }
  
  if (currentPatternIndex >= patternCount) {
    currentPatternIndex = 0;
  }

  // Write to EEPROM if autoplay is not active. Saving every pattern change will wear EEPROM!
  if (autoplay == 0) {
    EEPROM.write(1, currentPatternIndex);
    EEPROM.commit();
  }

  broadcastInt("pattern", currentPatternIndex);
}

// Set pattern, write to EEPROM
void setPattern(uint8_t value) {
  if (value >= patternCount) {
    value = patternCount - 1;
  }
  
  currentPatternIndex = value;

  // Write to EEPROM if autoplay is not active. Saving every pattern change will wear EEPROM!
  if (autoplay == 0) {
    EEPROM.write(1, currentPatternIndex);
    EEPROM.commit();
  }

  broadcastInt("pattern", currentPatternIndex);
}

// Set pattern name
void setPatternName(String name) {
  for (uint8_t i = 0; i < patternCount; i++) {
    if (patterns[i].name == name) {
      setPattern(i);
      break;
    }
  }
}

// Function to call setSolidColor
void setSolidColor(CRGB color) {
  setSolidColor(color.r, color.g, color.b);
}

// Set color values and write them to EEPROM
void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
  solidColor = CRGB(r, g, b);

  EEPROM.write(2, r);
  EEPROM.write(3, g);
  EEPROM.write(4, b);
  EEPROM.commit();

  // Switch to solod color mode
  setPattern(patternCount - 1);

  broadcastString("color", String(solidColor.r) + "," + String(solidColor.g) + "," + String(solidColor.b));
}

// Set power status, write to EEPROM
void setPower(uint8_t value) {
  power = value == 0 ? 0 : 1;

  EEPROM.write(5, power);
  EEPROM.commit();

  broadcastInt("power", power);
}

// Set autoplay status, write to EEPROM
void setAutoplay(uint8_t value) {
  autoplay = value == 0 ? 0 : 1;

  EEPROM.write(6, autoplay);
  EEPROM.commit();

  broadcastInt("autoplay", autoplay);
}

// Set autoplay duration, write to EEPROM
void setAutoplayDuration(uint8_t value) {
  if (value > 250) {
    value = 250;
  }
  else if (value < 3) {
    value = 3;
  }

  autoplayDuration = value;

  EEPROM.write(7, autoplayDuration);
  EEPROM.commit();

  autoPlayTimeout = millis() + (autoplayDuration * 1000);

  broadcastInt("autoplayDuration", autoplayDuration);
}

// Set color palette and write to EEPROM
void setPalette(uint8_t value) {
  if (value >= paletteCount) {
    value = paletteCount - 1;
  }

  currentPaletteIndex = value;

  EEPROM.write(8, currentPaletteIndex);
  EEPROM.commit();

  broadcastInt("palette", currentPaletteIndex);
}

// Set palette names
void setPaletteName(String name) {
  for (uint8_t i = 0; i < paletteCount; i++) {
    if (paletteNames[i] == name) {
      setPalette(i);
      break;
    }
  }
}

// Set gradient palette and write to EEPROM
void setGradientPalette(uint8_t value) {
  if (value >= gGradientPaletteCount) {
    value = gGradientPaletteCount - 1;
  }
  
  currentGradientPaletteIndex = value;

  EEPROM.write(9, currentGradientPaletteIndex);
  EEPROM.commit();

  broadcastInt("gradientPalette", currentGradientPaletteIndex);
}

// Set gradient palette names
void setGradientPaletteName(String name) {
  for (uint8_t i = 0; i < gGradientPaletteCount; i++) {
    if (gradientPaletteNames[i] == name) {
      setGradientPalette(i);
      break;
    }
  }
}

// Set fade-in speed, write to EEPROM
void setFadeInSpeed(uint8_t value) {
  fadeInSpeed = value;

  EEPROM.write(10, fadeInSpeed);
  EEPROM.commit();

  broadcastInt("fadeInSpeed", fadeInSpeed);
}

// Set fade-out speed, write to EEPROM
void setFadeOutSpeed(uint8_t value) {
  fadeOutSpeed = value;

  EEPROM.write(11, fadeOutSpeed);
  EEPROM.commit();

  broadcastInt("fadeOutSpeed", fadeOutSpeed);
}

// Set twinkle speed, write to EEPROM
void setTwinkleSpeed(uint8_t value) {
  twinkleSpeed = value;

  EEPROM.write(12, twinkleSpeed);
  EEPROM.commit();

  broadcastInt("twinkleSpeed", twinkleSpeed);
}

// Set twinkle density, write to EEPROM
void setTwinkleDensity(uint8_t value) {
  twinkleDensity = value;

  EEPROM.write(13, twinkleDensity);
  EEPROM.commit();

  broadcastInt("twinkleDensity", twinkleDensity);
}

////////////////////////////////////////////////////////////////
/////////////////////// EFFECT FUNCTIONS ///////////////////////
////////////////////////////////////////////////////////////////

// Function to set solid color for whole LED strip
void showSolidColor() {
  fill_solid(leds, NUM_LEDS, solidColor);
}

// FastLED's built-in rainbow generator
void rainbow() {
  uint8_t rainbowhue = beat8(speed);
  fill_rainbow( leds, NUM_LEDS, rainbowhue, 3);
}

// Built-in FastLED rainbow, plus some random sparkly glitter
void rainbowWithGlitter() {
  rainbow();
  addGlitter(80);
}

// Solid rainbow effect, like standart non-addressable LED strips
void rainbowSolid() {
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

// Random colored speckles that blink in and fade out smoothly, uses color palettes
void confetti() {
  fadeToBlackBy( leds, NUM_LEDS, speed);
  int pos = random16(NUM_LEDS);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

// White sparkles that blink in and fade quickly
void sparkles() {
  fadeToBlackBy( leds, NUM_LEDS, speed);
  int pos = random16(NUM_LEDS);
  leds[pos] += CRGB::White;
}

// A colored dot sweeping back and forth, with fading trails
void sinelon() {
  fadeToBlackBy( leds, NUM_LEDS, 20); // Change '20' to 'cooling' to control trail length with 'Cooling' slider
  int pos = beatsin16(speed, 0, NUM_LEDS);
  static int prevpos = 0;
  CRGB color = ColorFromPalette(palettes[currentPaletteIndex], gHue, 255);
  
  if ( pos < prevpos ) {
    fill_solid( leds + pos, (prevpos - pos) + 1, color);
  } 
  else {
    fill_solid( leds + prevpos, (pos - prevpos) + 1, color);
  }
  
  prevpos = pos;
}

// Effect with colored stripes pulsing at a defined Beats-Per-Minute (BPM)
void bpm() {
  uint8_t beat = beatsin8( speed, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

// Jugge effect - multiple colorful sweeping dots with trails
void juggle() {
  static uint8_t    numdots =   4; // Number of dots in use
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot
  static uint8_t    thishue =   0; // Starting hue
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour
  static uint8_t thisbright = 255; // How bright should the LED be
  static uint8_t   basebeat =   5; // Higher = faster movement

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
  
  switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  
  for ( int i = 0; i < numdots; i++) {
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

// Function to call heatMap with fire elements
void fire() {
  heatMap(HeatColors_p, true);
}

// Function to call heatMap with water elements
void water() {
  heatMap(IceColors_p, false);
}

// Function to call palettetest with current palette selected
void gradientSweeper() {
  palettetest( leds, NUM_LEDS, gGradientPalettes[currentGradientPaletteIndex]);
}

// Functon to call palettetest with changing palettes
void gradientShowcase() {
  palettetest( leds, NUM_LEDS, gCurrentPalette);
}

// This function draws rainbows with an ever-changing, widely-varying set of parameters.
void pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

// Fire effect with sparkling and cooling
void heatMap(CRGBPalette16 palette, bool up) {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(NUM_LEDS));

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( uint16_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( uint16_t j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240 for best results with color palettes
    colorindex = scale8(heat[j], 190);

    CRGB color = ColorFromPalette(palette, colorindex);

    if (up) {
      leds[j] = color;
    }
    else {
      leds[(NUM_LEDS - 1) - j] = color;
    }
  }
}

// Add white glitter based on a chance
void addGlitter( uint8_t chanceOfGlitter) {
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

// Forward declarations of an array of cpt-city gradient palettes, and a count of how many there are.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

uint8_t beatsaw8( accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0) {
  uint8_t beat = beat8( beats_per_minute, timebase);
  uint8_t beatsaw = beat + phase_offset;
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8( beatsaw, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

// Function to call colorwaves
void colorWaves() {
  colorwaves( leds, NUM_LEDS, gCurrentPalette);
}

// This function draws color waves with an ever-changing, widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } 
    else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16 ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds - 1) - pixelnumber;

    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

// Palette definitions for Pacifica effect
CRGBPalette16 pacifica_palette_1 = { 
  0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
  0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50
};

CRGBPalette16 pacifica_palette_2 = { 
  0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
  0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F
};

CRGBPalette16 pacifica_palette_3 = { 
  0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
  0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF
};

// Pacifica loop effect
void pacifica_loop() {
  // Increment the four "color index start" counters, one for each wave layer. Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
  sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
  sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
  sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0 - beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10, 38), 0 - beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10, 28), beat16(601));

  // Add brighter whitecaps where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff) {
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra white to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps() {
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );

  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    
    if ( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors() {
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145);
    leds[i].green = scale8( leds[i].green, 200);
    leds[i] |= CRGB( 2, 5, 7);
  }
}

// Calm effect, like a lake at night, uses color palettes
void night_lake() {
  uint8_t effectSpeed = 15;
  int wave1 = beatsin8(effectSpeed + 2, -64, 64);
  int wave2 = beatsin8(effectSpeed + 1, -64, 64);
  uint8_t wave3 = beatsin8(effectSpeed + 2, 0, 80);
  uint8_t lum = 0;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int index = cos8((i * 15) + wave1) / 2 + cubicwave8((i * 23) + wave2) / 2;
    
    if(index > wave3) {
      lum = index - wave3;
    }
    else {
      lum = 0;
    }

    leds[i] = ColorFromPalette(palettes[currentPaletteIndex], map(index, 0, 255, 0, 240), lum, LINEARBLEND);
  }
}

// Used by meteorRain
void colorfulMeteor() {
  meteorRain(true);
}

// Used by meteorRain
void whiteMeteor() {
  meteorRain(false);
}

// Falling meteor effect with decaying tail - must reduce size and trail for short strips
void meteorRain(bool useColorPalettes) {
  // Meteor size in pixels, without tail
  uint8_t meteorSize = 32;
  // Meteor tail decay, lower value means longer tail (slower decay)
  uint8_t meteorTrailDecay = 48;
  // Tail decays randomly, like a real meteor is not perfect and leaves some glowing dust behind
  bool meteorRandomDecay = true;
  // Default meteor color definitions (0-255)
  uint8_t redComponent = 255;
  uint8_t greenComponent = 255;
  uint8_t blueComponent = 255;

  if (meteorCounter < NUM_LEDS) { // Can replace 'NUM_LEDS' with 'NUM_LEDS + NUM_LEDS' in this IF statement if LED strip is very short
    // Add decay
    for (int j = 0; j < NUM_LEDS; j++) {
      if ( (!meteorRandomDecay) || (random(10) > 5) ) {
        leds[j].fadeToBlackBy(meteorTrailDecay);
      }
    }

    // Draw meteor
    for (int j = 0; j < meteorSize; j++) {
      if ( ( meteorCounter - j < NUM_LEDS) && (meteorCounter - j >= 0) ) {
        if (!useColorPalettes) {
          leds[meteorCounter - j] = CRGB(redComponent, greenComponent, blueComponent);
        }
        else {
          leds[meteorCounter - j] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64), 255, LINEARBLEND);
        }
      }
    }
    meteorCounter++;
  }
  else {
    meteorCounter = 0;
    return;
  }
}

// Alternate rendering function just scrolls the current palette across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette) {
  static uint8_t startindex = 0;
  startindex--;
  
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}
