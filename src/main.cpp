#include <WiFi.h>
#include <ota.h>
#include "SPIFFS.h"

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// This string should correspond to github tag used for Releasing (via. Github Actions)
#define VERSION "3.0.0"

// Replace your_username/your_repo with your values (ex. axcap/Esp-GitHub-OTA)
// This is a link to repo where your firmware updates will be pulled from
// #define RELEASE_URL "https://api.github.com/repos/your_username/your_repo/releases/latest"

// Use this version of the URL together with init_ota(VERSION, true) under debugging
// to spare yourself from getting timeout from GitHub API
#define RELEASE_URL "https://github.com/andreas-hagenmeyer/ghota_test/releases/latest"

#define DELAY_MS 1000


#define HOSTNAME "ESP32 OTA"
#define LED_BUILTIN 13

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";

//Variables to save values from HTML form
String ssid;
String pass;


// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";




String readFile(fs::FS &fs, const char * path);
void initSPIFFS();
void writeFile(fs::FS &fs, const char * path, const char * message);
bool setup_wifi();

///
///////////////////

void setup()
{
  Serial.begin(115200);

  initSPIFFS();
    // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);

  Serial.println(ssid);
  Serial.println(pass);


  if(setup_wifi())
  {
    init_ota(VERSION,true);
    handle_ota(RELEASE_URL);
  }
  else
  {
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

        // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: ");
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }


  pinMode(LED_BUILTIN, OUTPUT);

}

void loop()
{
  

  // Your code goes here
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(DELAY_MS);                  // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(DELAY_MS);                  // wait for a second
}



////////////////
////////////////

bool setup_wifi(){
  Serial.println("Initialize WiFi");
  if(ssid==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.hostname(HOSTNAME);
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Connecting");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    i ++;
    if(i>10)
      return false;
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}
