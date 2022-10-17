#include "EspHttpFirmwareUploader.h"
#include <http_parser.h>
#include <WString.h>

const char* CSS_UPGRADE_STYLE = 
    "table {"
    "   width: 420px;"
    "   border: 1px solid #F30021;"
    "}"
    ".table_header {"
    "   background-color: #F30021;"
    "   text-align: center;"
    "}"
    "button {"
    "   height: 21.5px;"
    "   width: 70px;"
    "}";

#ifdef NEXTION_HMI
const char* NEXTION_SCRIPT = 
    "function upgradeNextion(){ "
    "   var uploadedFile = document.getElementById('nextion_upload'); "
    "   let fileSize = uploadedFile.files[0].size; "
    "   let formData = new FormData(document.forms.form_nextion); "
    "   let xhr = new XMLHttpRequest(); "
    "   xhr.open('POST', '/nextion_upgrade.do?length='+fileSize); "
    "   xhr.send(formData); "
    "} ";
#endif

const char* PAGE_UPGRADE_SCRIPT = 
    "function UpgradePage(){ "

    "} ";

const char* HTML_UPGRADE_TEMPLATE =
    " <html>"
    "    <head>"
    "        <link rel='stylesheet' href='upgrade.css'>"
    "        <script type='text/javascript' src='upgrade.js'></script>"
#ifdef NEXTION_HMI
    "        <script type='text/javascript' src='nextion.js'></script>"
#endif
    "    </head>"
    "    <body>"
    "        <table>"
    "            <tr>"
    "                <td colspan='4' class='table_header'>Firmware</td>"
    "            </tr>"
#ifdef NEXTION_HMI
    "            <tr>"
    "                <td style='width: 100px;'>"
    "                    <div>Nextion HMI: </div>"
    "                </td>"
    "                <td colspan='2' style='width: 200px'>"
    "                    <form id='form_nextion' action='/nextion_upgrade.do' method='POST' enctype='multipart/form-data' style='display:inline;'>"
    "                        <input type='file' name='nextion_upload' id='nextion_upload' form='form_nextion' required='required'/>"
    "                    </form>"
    "                </td>"
    "                <td>"
    "                    <button onclick='upgradeNextion();'>Upgrade</button>"  
    "                </td>"
    "            </tr>"
#endif
    "            <tr>"
    "                <td>"
    "                    <div>Device: </div>"
    "                </td>"
    "                <td colspan='2'>"
    "                    <form id='form_firmware' name='form_firmware' action='/firmware_upgrade.do' method='POST' enctype='multipart/form-data' style='display:inline;'>"
    "                        <input type='file' name='firmware_upload' required='required'/>"
    "                    </form>"     
    "                </td>"
    "                <td>"
    "                    <button type='submit' form='form_firmware'>Upgrade</button>" 
    "                </td>"
    "            </tr>"
    "            <tr>"
    "                <td colspan='3'>"
    "                </td>"
    "                <td>"
    "                    <button name='cancel' onclick=\"location.href='/'\">Back</button>"
    "                </td>"
    "            </tr>"
    "        </table>"
    "    </body>"
    "<script>"
    "   UpgradePage();"
    "</script>"
    "</html>";   

void EspHttpFirmwareUploader::_pageFirmwareUI()
{
   _server->send(200, "text/html", HTML_UPGRADE_TEMPLATE);
}

void EspHttpFirmwareUploader::_pageUploadFirmwareSuccess()
{   
    _server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    
    if (Update.hasError())
      return;

    delay(1000);
    ESP.restart();
}

void EspHttpFirmwareUploader::_pageStyle()
{
    _server->send(200, "text/html", CSS_UPGRADE_STYLE);
}

void EspHttpFirmwareUploader::_pageScript()
{
    _server->send(200, "text/javascript", PAGE_UPGRADE_SCRIPT);
}

EspHttpFirmwareUploader::~EspHttpFirmwareUploader(){
#ifdef NEXTION_HMI
    delete _nexUploader;
#endif
}

bool EspHttpFirmwareUploader::_pageUploadFirmware()
{
    HTTPUpload &upload = _server->upload();
    if (upload.status == UPLOAD_FILE_START)
    {
      //setUpdateLastError(0);
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace))
      {
        //setUpdateLastError(Update.getError());
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        //setUpdateLastError(Update.getError());
      }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      Update.end(true);
    }  
}   

void EspHttpFirmwareUploader::setUpdateLastError(const char* error)
{
    _error[0] = '\0';
    strcat(_error, error);
}


#ifdef ESP8266
EspHttpFirmwareUploader::EspHttpFirmwareUploader(ESP8266WebServer* server, const char* urlPageUI)
{
    _server = server;
    _server->on(urlPageUI, HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageFirmwareUI, this));
    
    _server->on("/upgrade.css", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageStyle, this));
    _server->on("/upgrade.js", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageScript, this));
    _server->on("/firmware_upgrade.do", HTTP_POST, std::bind(&EspHttpFirmwareUploader::_pageUploadFirmwareSuccess,this), 
        std::bind(&EspHttpFirmwareUploader::_pageUploadFirmware, this));

    #ifdef NEXTION_HMI   
        _nexUploader = new ESPNexUpload(115200);

        _server->on("/nextion_upgrade.do", HTTP_POST, std::bind(&EspHttpFirmwareUploader::_pageUploadNextionSuccess,this), 
            std::bind(&EspHttpFirmwareUploader::_pageUploadNextion, this));
        _server->on("/nextion_upgrade.state", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageStateNextion, this));
        _server->on("/nextion.js", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageNextionScript, this));
    #endif
}
#elif ESP32
EspHttpFirmwareUploader::EspHttpFirmwareUploader(WebServer* server, const char* urlPageUI)
{
    _server = server;
    _server->on(urlPageUI, HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageFirmwareUI, this));
    
    memset(_error, '\0', ERROR_MESSAGE_SIZE);

    _server->on("/upgrade.css", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageStyle, this));
    _server->on("/upgrade.js", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageScript, this));
    _server->on("/firmware_upgrade.do", HTTP_POST, std::bind(&EspHttpFirmwareUploader::_pageUploadFirmwareSuccess,this), 
        std::bind(&EspHttpFirmwareUploader::_pageUploadFirmware, this));
    _server->on("/upgrade.error", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageError, this));

    #ifdef NEXTION_HMI   
        _nexUploader = new ESPNexUpload(115200);
        _server->on("/nextion_upgrade.do", HTTP_POST, std::bind(&EspHttpFirmwareUploader::_pageUploadNextionSuccess,this), 
            std::bind(&EspHttpFirmwareUploader::_pageUploadNextion, this));
        _server->on("/nextion.js", HTTP_GET, std::bind(&EspHttpFirmwareUploader::_pageNextionScript, this));
    #endif
}
#endif

#ifdef NEXTION_HMI

void EspHttpFirmwareUploader::_pageUploadNextionSuccess()
{
    _server->send(200, "text/plain", "OK");
}

void EspHttpFirmwareUploader::_pageNextionScript()
{
    _server->send(200, "text/javascript", NEXTION_SCRIPT);
}

void EspHttpFirmwareUploader::_pageError()
{
    _server->send(200, "text/plain", (char*)_error);
}

bool EspHttpFirmwareUploader::_pageUploadNextion()
{
    HTTPUpload& upload = _server->upload();
    int contentSize = atoi(_server->arg("length").c_str());
  
    if(!upload.filename.endsWith(F(".tft"))){
        setUpdateLastError("Only TFT file allowed");
        return false;
    }
  
    if(!_result){
        setUpdateLastError( _getNexUploader()->statusMessage.c_str());
        return false;
    }

    if(upload.status == UPLOAD_FILE_START){
        _result = _getNexUploader()->prepareUpload(contentSize);
    } else if(upload.status == UPLOAD_FILE_WRITE){
         _result = _getNexUploader()->upload(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END){
         _getNexUploader()->end();
        return true;
    }

    if(!_result){
        setUpdateLastError( _getNexUploader()->statusMessage.c_str());
        return false;
    }
} 

ESPNexUpload* EspHttpFirmwareUploader::_getNexUploader(){
    return _nexUploader;
}

#endif


