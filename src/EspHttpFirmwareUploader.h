#ifndef NEXTION_UPDATE_HELPER_H
#define NEXTION_UPDATE_HELPER_H

#define NEXTION_HMI

#ifdef NEXTION_HMI
#include <ESPNexUpload.h>
#endif

#ifdef ESP8266
    #include <ESP8266WebServer.h>
    #include <Updater.h>
#elif ESP32
    #include <WebServer.h>
    #include <Update.h>
#endif

#define ERROR_MESSAGE_SIZE 250
class EspHttpFirmwareUploader
{
    private:
      #ifdef ESP8266
        ESP8266WebServer* _server;
      #elif ESP32
        WebServer* _server;
      #endif
      
      bool _result = true;
      char _error[ERROR_MESSAGE_SIZE];

      #ifdef NEXTION_HMI
        ESPNexUpload* _nexUploader = nullptr;
        ESPNexUpload* _getNexUploader();
        bool _pageUploadNextion();
        void _pageUploadNextionSuccess();
        void _pageNextionScript();
      #endif

      void _pageError();
      void _print(char* text);
      void setUpdateLastError(const char* error);
      void _pageFirmwareUI();
      void _pageUploadFirmwareSuccess();
      bool _pageUploadFirmware();
      void _pageStyle();
      void _pageScript();
    
    public:
      #ifdef ESP8266
        EspHttpFirmwareUploader(ESP8266WebServer* server, const char* urlPageUI);
      #elif ESP32
        EspHttpFirmwareUploader(WebServer* server, const char* urlPageUI);
      #endif
      ~EspHttpFirmwareUploader();
};

#endif