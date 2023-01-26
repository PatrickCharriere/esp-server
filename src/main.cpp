using namespace std;
#include <string>
#include <ReactESP.h>
#include <Wire.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <fstream>
#include <stdio.h>
#include "pitches.h"
#include <OneWire.h>
#include <DallasTemperature.h>



///////////////////////////////////////////////////////////////////////// GPIOs
#define WiFi_LED_PIN 4
#define PROCESSING_LED_PIN 0
#define STATUS_OK_LED_PIN 2
#define ERROR_LED_PIN 15
#define BUZZER_PIN 5


///////////////////////////////////////////////////////////////////////// OLED DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // Display width (px)
#define SCREEN_HEIGHT 64 // Display height (px)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

///////////////////////////////////////////////////////////////////////// WiFi
const char* ssid     = "WiFi-14FE";
const char* password = "205.easternStar";
enum WIFI_STATUS { CONNECTING, CONNECTED };
WIFI_STATUS wifiStatus = CONNECTING;
enum MODULE_STATUS { GOOD, KO };
MODULE_STATUS moduleStatus = GOOD;
enum MODULE_PROCESSING { PROCESSING, INACTIVE };
MODULE_PROCESSING moduleProcessing = INACTIVE;


///////////////////////////////////////////////////////////////////////// WEB SERVER
AsyncWebServer server(80);
// Asynchronous processes
using namespace reactesp;
ReactESP app;
// Variable to store the HTTP req  uest
String header;

///////////////////////////////////////////////////////////////////////// PWM
int dutyCycle = 0;
const int redLedPin = 12;  // RED
const int greenLedPin = 13;  // GREEN
const int blueLedPin = 14;  // BLUE
const int freq = 1000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8;
String redString = "0";
String greenString = "0";
String blueString = "0";
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;
const char* PARAM_MESSAGE = "rgb";


///////////////////////////////////////////////////////////////////////// INTERNET TIME
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
// Variables to save date and time
String formattedTime;
String dayStamp;
String timeStamp;
void initialiseTime(void) {
    // Initialize a NTPClient to get time
    timeClient.begin();
    // Set offset time in seconds to adjust for the timezone
    // GMT +1 = 3600
    // GMT +10 = 36000
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient.setTimeOffset(39600);
}
void getTime(void) {
  timeClient.forceUpdate();
  //format hh:mm:ss
  formattedTime = timeClient.getFormattedTime();
  //Serial.println(formattedTime);
  return;
}


///////////////////////////////////////////////////////////////////////// FILEs
void writeInFile(String filename, String value) {
  unsigned long timestamp = timeClient.getEpochTime();

  Serial.print("Opening file: ");
  Serial.println(filename);
  File file = SPIFFS.open(filename, FILE_APPEND); //FILE_WRITE
  if(!file){
      Serial.println("There was an error opening the file for writing");
      return;
  }
  file.print(timestamp); 
  if(file.println("-" + value)) {
    Serial.println("File was written");
  } else {
      Serial.println("File write failed");
  }
	
  file.close();
}
void listAllFiles(void) {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file) {
    Serial.println("FILE: ");
    Serial.println(file.name());
    Serial.println(file.readString());
    file = root.openNextFile();
  }
  Serial.println("END OF FILES READ");
  root.close();
  file.close();
}
void initFileSytem(void) {
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
  } else {
    Serial.println("SPIFFS mounted");
  }
  listAllFiles();
}



///////////////////////////////////////////////////////////////////////// TEMPERATURE
const float maxTempWarning = 25.5;
const int oneWireBus = 4;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
float temperatureC = 0;
String temperatureString = "";





void initDebug() {
  // Serial port for debugging purposes
  Serial.begin(115200);

}

bool wifiConnect() {
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  wifiStatus = CONNECTING;

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(String("Connected to WiFi ") + String(ssid));
  wifiStatus = CONNECTED;
  return 0;
}

void updateTemperature() {

  Serial.println("temp reading");
  sensors.requestTemperatures(); 

  temperatureC = sensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");

  temperatureString = String(temperatureC, 1);
  writeInFile("/temp.txt", temperatureString.c_str());

}

void startAsyncProcesses() {
  bool buzz = true;

  // LED blinking processes
  app.onRepeat(100, [] () {

    switch (wifiStatus) {
      case CONNECTING:
        static bool state = LOW;
        digitalWrite(WiFi_LED_PIN, state = !state);
        break;
      case CONNECTED:
        digitalWrite(WiFi_LED_PIN, HIGH);
        break;
      default:
        digitalWrite(WiFi_LED_PIN, LOW);
        break;
    }
    switch (moduleStatus) {
      case GOOD:
        digitalWrite(STATUS_OK_LED_PIN, HIGH);
        digitalWrite(ERROR_LED_PIN, LOW);
        break;
      default:
        digitalWrite(STATUS_OK_LED_PIN, LOW);
        digitalWrite(ERROR_LED_PIN, HIGH);
        break;
    }
    switch (moduleProcessing) {
      case PROCESSING:
        static bool processingState = LOW;
        digitalWrite(PROCESSING_LED_PIN, processingState = !processingState);
        break;
      default:
        digitalWrite(PROCESSING_LED_PIN, LOW);
        break;
    }

  });

  app.onRepeat(200, [] () {
    
    static bool buzzing = false;

    // Temperature monitoring process
    if(temperatureC >= maxTempWarning) {

      if(buzzing) {
        noTone(BUZZER_PIN);
      } else {
        tone(BUZZER_PIN, NOTE_A4);
      }
      buzzing = !buzzing;

    } else {
      noTone(BUZZER_PIN);
    }

  });

  app.onRepeat(1000, [] () {
    //get time every second
    getTime();

    //refresh screen every second
    display.setTextSize(5);
    display.setCursor(5, 5);
    display.clearDisplay();
    display.println(temperatureString);
    display.setTextSize(2);
    display.setCursor(15, 46);
    display.println(formattedTime);
    display.display();
  });

  app.onRepeat(10000, [] () {
    updateTemperature();
  });
}


void setup() {
  
  Wire.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(WiFi_LED_PIN, OUTPUT);
  pinMode(PROCESSING_LED_PIN, OUTPUT);
  pinMode(STATUS_OK_LED_PIN, OUTPUT);
  pinMode(ERROR_LED_PIN, OUTPUT);
  moduleStatus = GOOD;
  moduleProcessing = INACTIVE;

  // Start the async process
  startAsyncProcesses();

  bool status;
  initDebug();
  wifiConnect();

  initialiseTime();
  getTime();
  initFileSytem();

  // configure LED PWM functionalitites
  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(redLedPin, redChannel);
  ledcAttachPin(greenLedPin, greenChannel);
  ledcAttachPin(blueLedPin, blueChannel);

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/temp.txt");
  });
  server.on("/color", HTTP_GET, [](AsyncWebServerRequest *request){
    String colors[3];

      Serial.println("RGB request ____________________________________ ");

      if (request->hasParam(PARAM_MESSAGE)) {
        String paramString = request->getParam(PARAM_MESSAGE)->value();

        colors[0] = paramString.substring(0, paramString.indexOf(','));
        colors[1] = paramString.substring(paramString.indexOf(',')+1, paramString.lastIndexOf(','));
        colors[2] = paramString.substring(paramString.lastIndexOf(',')+1, paramString.length());
        
        Serial.println(colors[0]);
        Serial.println(colors[1]);
        Serial.println(colors[2]);
        ledcWrite(redChannel, 255-colors[0].toInt());
        ledcWrite(greenChannel, 255-colors[1].toInt());
        ledcWrite(blueChannel, 255-colors[2].toInt());

      } else {
          Serial.println("No params found");
      }
      request->send_P(200, "text/plain", "RGB request received by ESP32");

  });
    
  Serial.println("Start the DS18B20 sensor");
  sensors.begin();
  sensors.setResolution(0x7F);

  // Start server
  server.begin();


  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  updateTemperature();
}

void loop(){
  app.tick();
}



