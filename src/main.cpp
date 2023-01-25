
#include "main.h"
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

/*#include "../lib/inet_time/internet_time.h"
#include "../lib/temperature/temperature.h"
#include "../lib/file_save/file_save.h"*/


// GPIOs
#define WiFi_LED_PIN 4
#define PROCESSING_LED_PIN 0
#define STATUS_OK_LED_PIN 2
#define ERROR_LED_PIN 15

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


////////////////////////////////////////// FILE SAVE
void writeInFile(String filename, String value) {

  //File file = SPIFFS.open(filename, FILE_WRITE);
  File file = SPIFFS.open(filename, FILE_APPEND);
  if(!file){
      Serial.println("There was an error opening the file for writing");
      return;
  }
  if(file.print(value + ",")) {
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

    timeClient.forceUpdate();

    //format hh:mm:ss
    formattedTime = timeClient.getFormattedTime();
    Serial.println(formattedTime);

    return;
}
////////////////////////////////////////// INTERNET TIME END


////////////////////////////////////////// TEMPERATURE
// Temperature calculation constants
#define SAMPLERATE 10               // Number of samples to average
#define THERMISTORNOMINAL 10000     // Nominal resistance at 25C
#define RESISTOR_DIVIDER 10000      // Resistor of resistor bridge
#define TEMPERATURENOMINAL 298.25   // = 25 + 273.15
#define BCOEFFICIENT 3950           // Beta coefficient
#define THERMISTOR 35               // Thermistor PIN
String readTemperature(void) {
    Serial.println("Reading temperature...");
    int sample = analogRead(THERMISTOR);
    int thermalSamples[SAMPLERATE];
    float average, kelvin, Rt, celsius;
    int i;

    // Collect SAMPLERATE (default 5) samples
    for (i=0; i<SAMPLERATE; i++) {
      thermalSamples[i] = analogRead(THERMISTOR);
      delay(10);
    }
    // Calculate the average value of the samples
    average = 0;
    
    for (i=0; i<SAMPLERATE; i++) {
      average += thermalSamples[i];
    }

    average /= SAMPLERATE;
    Serial.println(String("Average reading: ") + String(average ,2));

    float Vs = 3.3;
    int adcMax = 4095;

    float Vout = average * Vs/adcMax;
    Rt = RESISTOR_DIVIDER * Vout / (Vs - Vout);

    kelvin = 1/(1/TEMPERATURENOMINAL + log(Rt/THERMISTORNOMINAL)/BCOEFFICIENT);  // Temperature in Kelvin

    celsius = kelvin - 273.15;                      // Celsius

    Serial.println(String("Calculated temp ") + String(celsius) +  String("ºC"));

    return String(celsius);
}
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

  app.onRepeat(1000, [] () {
    //get time every second
    getTime();
  });

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


  app.onRepeat(3000, [] () {
  
    // read temperature every 5 minutes (300s = 30000ticks)
    Serial.println("temp reading");
    writeInFile("/temperatures.txt", String( readTemperature() ));

  });
}

void setup() {
  pinMode (THERMISTOR, INPUT);
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

  ///////////////////////////////////////////
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
    request->send_P(200, "text/plain", readTemperature().c_str());
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
        ledcWrite(redChannel, colors[0].toInt());
        ledcWrite(greenChannel, colors[1].toInt());
        ledcWrite(blueChannel, colors[2].toInt());

      } else {
          Serial.println("No params found");
      }
      request->send_P(200, "text/plain", "RGB request received by ESP32");
      
      
      

  });

  // Start server
  server.begin();
}

void loop(){
  app.tick();
}
