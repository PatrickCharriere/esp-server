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
#include <AsyncElegantOTA.h>


///////////////////////////////////////////////////////////////////////// e-MAIL
// #define SMTP_HOST "smtp.gmail.com"
// #define SMTP_PORT 465
// #define AUTHOR_EMAIL "smarthome.205.marlborough@gmail.com"
// #define AUTHOR_PASSWORD "mqlskdjf"
// #define RECIPIENT_EMAIL "jimmy_angelmp@hotmail.com"
// SMTPSession smtp;
// void smtpCallback(SMTP_Status status);


///////////////////////////////////////////////////////////////////////// GPIOs
#define WiFi_LED_PIN 2
#define FAN_PIN 18


///////////////////////////////////////////////////////////////////////// OLED DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // Display width (px)
#define SCREEN_HEIGHT 64 // Display height (px)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


///////////////////////////////////////////////////////////////////////// WiFi
const char* ssid     = "WiFi-14FE";
const char* password = "205.easternStar";


///////////////////////////////////////////////////////////////////////// WEB SERVER
AsyncWebServer server(80);
using namespace reactesp;
ReactESP app;
String header;


///////////////////////////////////////////////////////////////////////// PWM
int dutyCycle = 0;
const int redLedPin = 12;
const int greenLedPin = 13;
const int blueLedPin = 14;
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
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedTime;
String dayStamp;
String timeStamp;
void forceTimeUpdate(void) {
  timeClient.forceUpdate();
}
void initialiseTime(void) {
  
    timeClient.begin();
    // To adjust timezone / GMT +1 = 3600 / GMT 0 = 0
    timeClient.setTimeOffset(39600);
    forceTimeUpdate();

}
void getTime(void) {
  
  formattedTime = timeClient.getFormattedTime(); //format hh:mm:ss
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
}


///////////////////////////////////////////////////////////////////////// TEMPERATURE
const float temperatureHighThresold = 25.5;
const float temperatureLowThresold = 24;
const int oneWireBus = 4;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
float temperatureC = 0;
String temperatureString = "";
const int intervalSecControlTemperature = 60;


void initDebug() {
  // Serial port for debugging purposes
  Serial.begin(115200);

}

bool wifiConnect() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }

  AsyncElegantOTA.begin(&server);
  Serial.println(String("Connected to WiFi ") + String(ssid));
  digitalWrite(WiFi_LED_PIN, HIGH);

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

void updateLedColor(AsyncWebServerRequest *request) {
  String colors[3];

  Serial.println("RGB request ___");

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

}

void setTemperatureParamsByWeb(AsyncWebServerRequest *request) {

  Serial.println("Update Temperature params web request ___");

  if (request->hasParam(PARAM_MESSAGE)) {
    String paramString = request->getParam(PARAM_MESSAGE)->value();

    paramString.substring(0, paramString.indexOf(','));
    
    Serial.println(paramString);
  } else {
      Serial.println("No params found");
  }
  request->send_P(200, "text/plain", "Update Temperature params request received");

}

void setNetworkParamsByWeb(AsyncWebServerRequest *request) {

  Serial.println("Update Network params web request ___");

  if (request->hasParam(PARAM_MESSAGE)) {
    String paramString = request->getParam(PARAM_MESSAGE)->value();

    paramString.substring(0, paramString.indexOf(','));
    
    Serial.println(paramString);
  } else {
      Serial.println("No params found");
  }
  request->send_P(200, "text/plain", "Update Network params request received");

}

void monitorTemperature(void) {

    static float previousTempWarning = false;
    static bool temperatureHighWarning = false;

    if( (!temperatureHighWarning) && (temperatureC >= temperatureHighThresold)) {

      Serial.println("Temperature HIGH Warning");
      temperatureHighWarning = true;

    } else if ( (temperatureHighWarning) && (temperatureC <= temperatureLowThresold)) {

      Serial.println("Temperature LOW");
      temperatureHighWarning = false;
      
    }

    if(previousTempWarning != temperatureHighWarning) {

      previousTempWarning = temperatureHighWarning;

      if(temperatureHighWarning) {
        digitalWrite(FAN_PIN, LOW);
      } else {
        digitalWrite(FAN_PIN, HIGH);
      }
      
    }

}

void refreshDisplay() {

  //refresh screen every second
  display.setTextSize(5);
  display.setCursor(5, 5);
  display.clearDisplay();
  display.println(temperatureString);
  display.setTextSize(2);
  display.setCursor(15, 46);
  display.println(formattedTime);
  display.display();

}

/*void smtpCallback(SMTP_Status status){
  
  Serial.println(status.info());

  
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
  }
}

void setupEmailParameters(void) {

  smtp.debug(1);

  smtp.callback(smtpCallback);

  ESP_Mail_Session session;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;

  message.sender.name = "ESP";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Test Email";
  message.addRecipient("Sara", RECIPIENT_EMAIL);

  String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello Jaime!</h1><p>It's too hot in here, I just started the fan to cool down the marine fish tank ;)</p><p>Have a good day</p><p>Smart home</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session))
    return;

  if (!MailClient.sendMail(&smtp, &message))
    Serial.print("Error sending Email, ");
    Serial.println(smtp.errorReason());

}*/

void startAsyncProcesses() {

  //Every second
  app.onRepeat(1000, [] () {
    getTime();
    monitorTemperature();
    refreshDisplay();
  });

  app.onRepeat(intervalSecControlTemperature * 1000, [] () { updateTemperature(); });

  //Every Day
  app.onRepeat(24 * 60 * 60 * 1000, [] () { forceTimeUpdate(); });

}

void setup() {
  
  Wire.begin();

  pinMode(WiFi_LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH); //Disable at start

  // Start the async process
  startAsyncProcesses();

  bool status;
  initDebug();
  wifiConnect();

  initFileSytem();

  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);
  ledcAttachPin(redLedPin, redChannel);
  ledcAttachPin(greenLedPin, greenChannel);
  ledcAttachPin(blueLedPin, blueChannel);

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/index.html"); });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/temp.txt"); });
  server.on("/color", HTTP_GET, [](AsyncWebServerRequest *request) { updateLedColor(request); } );
  server.on("/setTemperatureParams", HTTP_GET, [](AsyncWebServerRequest *request) { setTemperatureParamsByWeb(request); } );
  server.on("/setNetworkParams", HTTP_GET, [](AsyncWebServerRequest *request) { setNetworkParamsByWeb(request); } );

  server.serveStatic("/", SPIFFS, "/");
  
  Serial.println("Start the DS18B20 sensor");
  sensors.begin();
  sensors.setResolution(0x7F);

  // Start server
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();


  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  initialiseTime();
  getTime();
  updateTemperature();

  //setupEmailParameters();

}

void loop(){
  app.tick();
}
