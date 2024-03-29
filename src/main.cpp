
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

#include <OneWire.h>
#include <DallasTemperature.h>
// GPIOs
#define WiFi_LED_PIN 4
#define PROCESSING_LED_PIN 0
#define STATUS_OK_LED_PIN 2
#define ERROR_LED_PIN 15

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi Constants
const char* ssid     = "WiFi-14FE";
const char* password = "205.easternStar";

enum WIFI_STATUS { CONNECTING, CONNECTED };
WIFI_STATUS wifiStatus = CONNECTING;
enum MODULE_STATUS { GOOD, KO };
MODULE_STATUS moduleStatus = GOOD;
enum MODULE_PROCESSING { PROCESSING, INACTIVE };
MODULE_PROCESSING moduleProcessing = INACTIVE;

// AsyncWebServer object on port 80
AsyncWebServer server(80);

// Asynchronous processes
using namespace reactesp;
ReactESP app;

// setting PWM properties
int dutyCycle = 0;
const int redLedPin = 12;  // RED
const int greenLedPin = 13;  // GREEN
const int blueLedPin = 14;  // BLUE
const int freq = 1000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8;

// Decode HTTP GET value
String redString = "0";
String greenString = "0";
String blueString = "0";
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;

// Variable to store the HTTP req  uest
String header;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
const char* PARAM_MESSAGE = "rgb";


////////////////////////////////////////// INTERNET TIME
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
  //Serial.println("getEpochTime - " + timeClient.getEpochTime());

  timeClient.forceUpdate();
  //format hh:mm:ss
  formattedTime = timeClient.getFormattedTime();
  //Serial.println(formattedTime);

  return;
}
////////////////////////////////////////// INTERNET TIME END

////////////////////////////////////////// FILE SAVE
void writeInFile(String filename, String value) {

  String time = formattedTime.substring(0, 5);

  Serial.print("Opening file: ");
  Serial.println(filename);
  File file = SPIFFS.open(filename, FILE_APPEND); //FILE_WRITE
  if(!file){
      Serial.println("There was an error opening the file for writing");
      return;
  }
  if(file.println(time + " - " + value)) {
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
////////////////////////////////////////// FILE SAVE END




////////////////////////////////////////// TEMPERATURE
// Temperature calculation constants
// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
float temperatureC = 0;
String temperatureString = "";
////////////////////////////////////////// TEMPERATURE END

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

  /*app.onRepeat(3, [] () {

    // increase the LED brightness
    if((dutyCycle >= 0) && (dutyCycle <= 255)){   
      // changing the LED brightness with PWM
      ledcWrite(ledChannel, dutyCycle);
      dutyCycle++;
      return;
    }

    // decrease the LED brightness
    if((dutyCycle >= 255) && (dutyCycle <= 512)){
      // changing the LED brightness with PWM
      ledcWrite(ledChannel, (512 - dutyCycle));
      dutyCycle++;
      if(dutyCycle > 512) dutyCycle = 0;
      return;
    }

  });*/

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

  /////////////////////////////////////////// FILE READING
  initialiseTime();
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



