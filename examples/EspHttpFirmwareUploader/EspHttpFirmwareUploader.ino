#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EspHttpFirmwareUploader.h>

const char* ssid      = "xxxxxx";
const char* password  = "xxxxxxxxxxxxx";
const char* host      = "nextion";

WebServer server(80);
EspHttpFirmwareUploader nextionUpdater(&server, "/update_nextion.html");

void setup(void){
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  MDNS.begin(host);
  server.begin();
}
 
void loop(void){
  server.handleClient();
}
