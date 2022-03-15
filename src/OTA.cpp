#include "OTA.h"
#include "ChargePointStatusService.h"
#include "OcppEngine.h"
#include "Master.h"

extern bool wifi_connect;
unsigned long int startTimer = 0;

void setupOTA() {
    Serial.println("[HTTP] begin...");
    
    int updateSize = 0;
    HTTPClient http;
  
    // configure server and url
    //Changed Location to POD_V3 for all our new updates from 23rd Dec
    String post_data = "{\"version\":\"POD_V3/POD_EVRE.ino.esp32\", \"deviceId\":\"POD\"}";  //POD_V2/POD_EVRE.ino.esp32 this location belongs to subhash and manish tiwari
    http.begin("https://us-central1-evre-iot-308216.cloudfunctions.net/otaUpdate");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "keep-alive");

    int httpCode = http.POST(post_data);
  
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.println( "Checking for new firmware updates..." );
    
        // If file found at server
        if(httpCode == HTTP_CODE_OK) {
            // get lenght of document (is -1 when Server sends no Content-Length header)
            int len = http.getSize();
            updateSize = len;
            Serial.printf("[OTA] Update found, File size(bytes) : %d\n", len);
    
            // get tcp stream
            WiFiClient* client = http.getStreamPtr();

            Serial.println();
            performUpdate(*client, (size_t)updateSize);   
            Serial.println("[HTTP] connection closed or file end.\n");
        }
        // If there is no file at server
        if(httpCode == HTTP_CODE_INTERNAL_SERVER_ERROR) {
            Serial.println("[HTTP] No Updates");
            Serial.println();
            //ESP.restart();
        }
    }
    http.end();
}


// perform the actual update from a given stream
void performUpdate(WiFiClient& updateSource, size_t updateSize) {
    if (Update.begin(updateSize)) {
        Serial.println("...Downloading File...");
        Serial.println();
        
        // Writing Update
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize) {
            Serial.println("Written : " + String(written) + "bytes successfully");
        }
        else {
            Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        if (Update.end()) {
            Serial.println("OTA done!");
            if (Update.isFinished()) {
                Serial.println("Update successfully completed. Rebooting...");
                Serial.println();
                ESP.restart();
            }
            else {
                Serial.println("Update not finished? Something went wrong!");
            }
        }
        else {
            Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        }
    }
    else{
        Serial.println("Not enough space to begin OTA");
    }
}

short int flag_otatrails;
void ota_Loop(){

	if(millis() - startTimer > 43200000){
        // Add wifi connect/ ethernet connect. Both wifi and ethernet use the same API's
        // if(wifi_connect == true){ 
		if((getChargePointStatusService()->inferenceStatus() == ChargePointStatus::Available  || getChargePointStatusService()->inferenceStatus() == ChargePointStatus::Faulted) && wifi_connect == true){
			requestLed(BLUE,START,1); 
			setupOTA();
            //startTimer = millis();
            if(flag_otatrails++ > 2){
                Serial.println("Restarting ESP32");
                ESP.restart();
            }
		}
        //Add 4G/GSM connect 
        //else if(gsm_connect == true)
        /*
        if(wifi_connect == true){
//#if WIFI_ENABLED || ETHERNET_ENABLED
   success = webSocket->sendTXT(out);
   Serial.println("[WIFI: ]SendConf" +String(out)+String(success));
//#endif
 }else if(gsm_connect == true){
        */
	}
}