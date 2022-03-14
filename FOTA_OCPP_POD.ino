/************************************/
/*
<POD without Control Pilot Support>
The following code was initialized by Pulkit Agrawal & Wamique.
Modified by G. Raja Sumant
*Added Master-Slave files
*Added EnergyMeter Fix
*Added 2G+Wifi
*Added OTA
*Fixed RFID
*Raja added 4G , FOTA OCPP class , firmware status OCPP class and URL parser
*/

//ESP32 Libraries
#include <Arduino.h>
#include "src/libraries/arduinoWebSockets-master/src/WebSocketsClient.h"
//#include <ArduinoJson.h>
#include "src/Peripherals.h"
#include "src/OTA.h" 

//#if WIFI_ENABLED
#include <WiFi.h>
//#define SSIDW 	"Amplify Mobility_PD"
//#define PSSWD 	"Amplify5"
//#endif

//OCPP Message Generation Class
#include "src/OcppEngine.h"
#include "src/SmartChargingService.h"
#include "src/ChargePointStatusService.h"
#include "src/MeteringService.h"
#include "src/GetConfiguration.h"
#include "src/TimeHelper.h"
#include "src/SimpleOcppOperationFactory.h"
#include "src/EVSE.h"

//OCPP message classes
#include "src/OcppOperation.h"
#include "src/OcppMessage.h"
#include "src/Authorize.h"
#include "src/BootNotification.h"
#include "src/Heartbeat.h"
#include "src/StartTransaction.h"
#include "src/StopTransaction.h"
#include "src/DataTransfer.h"
#include "src/Variants.h"

//Master Class
#include"src/Master.h"

//Power Cycle
#include"src/PowerCycle.h"

#if CP_ACTIVE
//Control Pilot files
#include "src/ControlPilot.h"
#endif

#include "src/internet.h"

//Gsm Files
//#if GSM_ENABLED
#include "src/CustomGsm.h"
extern TinyGsmClient client;
//#endif

WebSocketsClient webSocket;

//SmartChargingService *smartChargingService;
ChargePointStatusService *chargePointStatusService;

//Mertering Service declarations
MeteringService *meteringService;
ATM90E36 eic(5);
#define SS_EIC 5          //GPIO 5 chip_select pin
SPIClass * hspi = NULL;


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
bool webSocketConncted = false;

//RFID declarations
#define MFRC_RST    22
#define MFRC_SS     15
MFRC522 mfrc522(MFRC_SS, MFRC_RST); // Create MFRC522 instance
SPIClass *hspiRfid =NULL;

//Flags used to control the section. They are defined in EVSE.h/EVSE.cpp
extern bool flag_evseIsBooted;
extern bool flag_evseReadIdTag;
extern bool flag_evseAuthenticate;
extern bool flag_evseStartTransaction;
extern bool flag_evRequestsCharge;
extern bool flag_evseStopTransaction;
extern bool flag_evseUnauthorise;
extern bool flag_evseSoftReset;
extern bool flag_rebootRequired;
extern bool flag_controlPAuthorise; 
//global variable for currentIdTag used by various functions. 
String currentIdTag;  

//Bluetooth
#include "src/bluetoothConfig.h"
#define TIMEOUT_BLE 60000
extern BluetoothSerial SerialBT;
bool isInternetConnected = true;

bool flagswitchoffBLE = false;
int startBLETime = 0;
String ws_url_prefix_m = "";
String host_m = "";
int port_m = 0;
int oc_m = 0;

int thresholdCurrent;
String protocol_m = "";
String key_m = "";
String ssid_m = "";

bool wifi_enable = false;
bool gsm_enable  = false;

bool wifi_connect = false;
bool gsm_connect = false;

extern Preferences preferences;
String url_m ="";
Preferences resumeTxn;
String idTagData_m = "";
bool ongoingTxn_m = false;

//metering Flag
extern bool flag_MeteringIsInitialised;

//Led timers
ulong timercloudconnect = 0;
void wifi_Loop();
void connectToWebsocket();

void setup() {
//Test LED
  //  pinMode(16,OUTPUT);
	Serial.begin(115200);
	Master_setup();
	//https://arduino-esp8266.readthedocs.io/en/latest/Troubleshooting/debugging.html
	Serial.setDebugOutput(true);
  
  if(DEBUG_OUT) Serial.println();
	if(DEBUG_OUT) Serial.println();
	if(DEBUG_OUT) Serial.println();

	for (uint8_t t = 4; t > 0; t--) {
		if(DEBUG_OUT) Serial.println("[SETUP]BOOT WAIT...." + VERSION);
		Serial.flush();
		delay(500);
	}
    
    requestLed(BLINKYWHITE,START,1); // 1 means 1st connector
    #if 0
  	requestLed(BLINKYWHITE_EINS,START,1); // 1 means 1st connector
    //test block
    delay(3000);
    requestLed(BLINKYWHITE_EINS,START,2); // 1 means 1st connector

    delay(3000);
    requestLed(BLINKYWHITE_EINS,START,3); // 1 means 1st connector

    delay(5000);
    requestLed(BLINKYGREEN_EINS,START,1); // 1 means 1st connector

    delay(5000);
    
    requestLed(BLINKYGREEN_EINS,START,2); // 1 means 1st connector

    delay(5000);
    requestLed(BLINKYGREEN_EINS,START,3); // 1 means 1st connector

    delay(5000);
    requestLed(RED,START,1); // 1 means 1st connector

    delay(5000);
    requestLed(RED,START,2); // 1 means 1st connector

    delay(5000);
    requestLed(BLINKYWHITE_EINS,START,3); // 1 means 1st connector

    delay(5000);

    #endif


    /*****************************************/
    requestForRelay(STOP,1);
    delay(100);
    requestForRelay(STOP,2);
    requestforCP_OUT(STOP);

    Serial.println("BL Enabled");
    #if BLE_ENABLE
    startingBTConfig();
    #endif
    /************************Preferences***********************************************/
    preferences.begin("credentials",false);
    
    ws_url_prefix_m = preferences.getString("ws_url_prefix",""); //characters
    if(ws_url_prefix_m.length() > 0){
    	Serial.println("Fetched WS URL success: " + String(ws_url_prefix_m));
    }else{
    	Serial.println("Unable to Fetch WS URL / Empty");
    }
    delay(100);

    host_m = preferences.getString("host","");
    if(host_m.length() > 0){
    	Serial.println("Fetched host data success: "+String(host_m));
    }else{
    	Serial.println("Unable to Fetch host data / Empty");
    }
    delay(100);

    port_m = preferences.getInt("port",0);
    if(port_m>0){
    	Serial.println("Fetched port data success: "+String(port_m));
    }else{
    	Serial.println("Unable to Fetch port Or port is 0000");
    }
    delay(100);

    protocol_m = preferences.getString("protocol","");
    if(protocol_m.length() > 0){
    	Serial.println("Fetched protocol data success: " + String(protocol_m));
    }else{
    	Serial.println("Unable to Fetch protocol");
    }

    oc_m = preferences.getInt("oc", 0);
    if(oc_m > 0){
      thresholdCurrent = oc_m;
      Serial.println("Fetched OverCurrent data success:" + String(oc_m));
    }else{
      thresholdCurrent = 16;
      Serial.println("Unable to fetch OverCurrent Value. Setting it to default: " + String(thresholdCurrent));
    }

    ssid_m = preferences.getString("ssid","");
    if(ssid_m.length() > 0){
    	Serial.println("Fetched SSID: "+String(ssid_m));
    }else{
    	Serial.println("Unable to Fetch SSID");
    }

    key_m = preferences.getString("key","");
    if(key_m.length() > 0){
    	Serial.println("Fetched Key: "+String(key_m));
    }else{
    	Serial.println("Unable to Fetch key");
    }

    wifi_enable = preferences.getBool("wifi",0);
    Serial.println("Fetched protocol data: "+String(wifi_enable));

    gsm_enable = preferences.getBool("gsm",0);
    Serial.println("Fetched protocol data: "+String(gsm_enable));

//WiFi
    wifi_connect = wifi_enable;
    gsm_connect  = gsm_enable;
    bool internet = false;
    int counter_wifiNotConnected = 0;
    int counter_gsmNotConnected = 0;
    
    if(wifi_enable == true){

      WiFi.begin(ssid_m.c_str(), key_m.c_str());
    }

    while(internet == false){
      Serial.println("Internet loop");
      bluetooth_Loop();
      if(wifi_enable == true && wifi_connect == true){
        Serial.println("Waiting for WiFi Connction...");
        
        if(WiFi.status() == WL_CONNECTED){
          internet = true;
          gsm_connect = false;
          Serial.println("Connected via WiFi");
          delay(100);
          connectToWebsocket();
        }else if(WiFi.status() != WL_CONNECTED){
          Serial.print(".");
          delay(1000);
          bluetooth_Loop();
          wifi_Loop();
          Serial.println("Wifi Not Connected: "+ String(counter_wifiNotConnected));
          if(counter_wifiNotConnected++ > 150){
            counter_wifiNotConnected = 0;
            
            if(gsm_enable == true){ wifi_connect = false;  gsm_connect =true;}
          }

        }
      }else if(gsm_enable == true && gsm_connect == true){
        SetupGsm();                                     //redundant @optimise
        ConnectToServer();
        if(!client.connected()){
          gsm_Loop();
          bluetooth_Loop();
          Serial.println("GSM not Connected: " + String(counter_gsmNotConnected));
          if(counter_gsmNotConnected++ > 2){  //2 == 5min
            counter_gsmNotConnected = 0;
            
            if(wifi_enable == true){ wifi_connect = true; gsm_connect = false;}
          }
        }else if(client.connected()){
          internet = true;
          wifi_connect = false;
          Serial.println("connected via GSM");
        }

      }


    }

  //  #if WIFI_ENABLED
	//Update the WiFi credentials in EVSE.h
 //  if(wifi_enable == true){
	//  if (DEBUG_OUT) Serial.println("Waiting for WiFi Connction...");
	//  WiFi.begin(ssid_m.c_str(), key_m.c_str());

	//  while (WiFi.status() != WL_CONNECTED) {
 //  		Serial.print(".");
	// 	delay(1000);
 //    bluetooth_Loop();
 //    wifi_Loop();
	//  }
	// //#endif
 //  }
	/*
	#if GSM_ENABLED
 	SetupGsm();
  	delay(1000);
  	ConnectToServer();
  	delay(5000);

 	 while(!client.connected()){
    	delay(1000);
    	gsm_Loop();
  	}

  	Serial.println("Connected via GSM");
#endif
*/



    //SPI Enable for Energy Meter Read
    hspi = new SPIClass(HSPI); // Init SPI bus
    hspi->begin();
    pinMode(SS_EIC, OUTPUT); //HSPI SS Pin

    // SPI Enable for RFID
    hspiRfid = new SPIClass(HSPI); 
    hspiRfid->begin();
    mfrc522.PCD_Init(); // Init MFRC522

//ocppEngine_initialize(&webSocket, 4096); //default JSON document size = 4096



  //	Serial.println("closing preferences");
  //	preferences.end();
  	ocppEngine_initialize(&webSocket, 4096); //default JSON document size = 2048

  	/*********Preferences Block For Restarting previously running Txn [Power Cycle]***********/
  	resumeTxn.begin("resume",false); //opening preferences in R/W mode
  	idTagData_m = resumeTxn.getString("idTagData","");
  	ongoingTxn_m = resumeTxn.getBool("ongoingTxn",false);

  	Serial.println("Stored ID:"+String(idTagData_m));
  	Serial.println("Ongoing Txn: "+String(ongoingTxn_m));
  	/****************************************************************************************/

	/*//Example for SmartCharging Service usage
	smartChargingService = new SmartChargingService(16.0f); //default charging limit: 16A
	smartChargingService->setOnLimitChange([](float limit) {
		if (DEBUG_OUT) Serial.print(F("setOnLimitChange Callback: Limit Change arrived at Callback function: new limit = "));
		if (DEBUG_OUT) Serial.print(limit);
		if (DEBUG_OUT) Serial.print(F(" A\n"));
		EVSE_setChargingLimit(limit);
	});*/

  chargePointStatusService = new ChargePointStatusService(&webSocket); //adds itself to ocppEngine in constructor
  meteringService = new MeteringService(&webSocket);

	//set system time to default value; will be without effect as soon as the BootNotification conf arrives
  setTimeFromJsonDateString("2021-22-12T11:59:55.123Z"); //use if needed for debugging  

  flag_evseIsBooted = false;
  flag_evseSoftReset = false;
  flag_rebootRequired = false;
	
	EVSE_setOnBoot([]() {
		//this is not in loop, that is why we need not update the flag immediately to avoid multiple copies of bootNotification.
		OcppOperation *bootNotification = makeOcppOperation(&webSocket,	new BootNotification());
		initiateOcppOperation(bootNotification);
		bootNotification->setOnReceiveConfListener([](JsonObject payload) {

      	if( flag_MeteringIsInitialised == false){
      		Serial.println("[SetOnBooT] Initializing metering services");
      		meteringService->init(meteringService);
      	}

      if (DEBUG_OUT) Serial.print(F("EVSE_setOnBoot Callback: Metering Services Initialization finished.\n"));


			flag_evseIsBooted = true; //Exit condition for booting. 	
			flag_evseReadIdTag = true; //Entry condition for reading ID Tag.
			flag_evseAuthenticate = false;
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = false;
			flag_evseUnauthorise = false;
      if (DEBUG_OUT) Serial.print(F("EVSE_setOnBoot Callback: Closing Relays.\n"));

			if (DEBUG_OUT) Serial.print(F("EVSE_setOnBoot Callback: Boot successful. Calling Read User ID Block.\n"));
		});
	});

	EVSE_setOnReadUserId([] () {
		if (DEBUG_OUT) Serial.print(F("EVSE_setOnReadUserId Callback: EVSE waiting for User ID read...\n"));
		static ulong timerForRfid = millis();
		currentIdTag = "";  	
		idTagData_m = resumeTxn.getString("idTagData","");
  		ongoingTxn_m = resumeTxn.getBool("ongoingTxn",false);
     
    if(wifi_connect == true){
		  if((ongoingTxn_m == 1) && (idTagData_m != "") && 
		      (getChargePointStatusService()->getEmergencyRelayClose() == false) &&
		      (WiFi.status() == WL_CONNECTED)&&
		      (webSocketConncted == true)&&
		      (isInternetConnected == true)){   //giving priority to stored data
			  currentIdTag = resumeTxn.getString("idTagData","");
			  Serial.println("[EVSE_setOnReadUserId] Resuming Session");
        requestLed(BLUE,START,1);    
		  }else if((getChargePointStatusService()->getEmergencyRelayClose() == false) &&
		          (WiFi.status() == WL_CONNECTED) &&
		          (webSocketConncted == true) && 
		          (isInternetConnected == true)){
			  #if LED_ENABLED
			    if(millis() - timerForRfid > 5000){ //timer for sending led request
    		    requestLed(GREEN,START,1);
    		    timerForRfid = millis();
    		  }
    		#endif
        
			currentIdTag = EVSE_getCurrnetIdTag(&mfrc522);
			Serial.println("[WiFi]********RFID**********");
		}
 }else if(gsm_connect){
        if((ongoingTxn_m == 1) && (idTagData_m != "") && 
          (getChargePointStatusService()->getEmergencyRelayClose() == false) &&
          (client.connected() == true)){   //giving priority to stored data
        currentIdTag = resumeTxn.getString("idTagData","");
        Serial.println("[EVSE_setOnReadUserId] Resuming Session");
        requestLed(BLUE,START,1);    
      }else if((getChargePointStatusService()->getEmergencyRelayClose() == false) &&
              (client.connected() == true)){
        #if LED_ENABLED
          if(millis() - timerForRfid > 5000){ //timer for sending led request
            requestLed(GREEN,START,1);
            timerForRfid = millis();
          }
        #endif
        
      currentIdTag = EVSE_getCurrnetIdTag(&mfrc522);
      Serial.println("[GSM]********RFID**********");
    }
  }
	if (currentIdTag.equals("") == true) {
			flag_evseReadIdTag = true; //Looping back read block as no ID tag present.
			flag_evseAuthenticate = false;
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = false;
			flag_evseUnauthorise = false;
		} else {
			flag_evseReadIdTag = false;
			flag_evseAuthenticate = true; //Entry condition for authentication block.
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = false;
			flag_evseUnauthorise = false;
			if (DEBUG_OUT) Serial.print(F("EVSE_setOnReadUserId Callback: Successful User ID Read. Calling Authentication Block.\n"));
		}	
	});
	
	EVSE_setOnsendHeartbeat([] () {
    if (DEBUG_OUT) Serial.print(F("EVSE_setOnsendHeartbeat Callback: Sending heartbeat signal...\n"));
    OcppOperation *heartbeat = makeOcppOperation(&webSocket, new Heartbeat());
    initiateOcppOperation(heartbeat); 
    heartbeat->setOnReceiveConfListener([](JsonObject payload) {
        const char* currentTime = payload["currentTime"] | "Invalid";
        if (strcmp(currentTime, "Invalid")) {
          if (setTimeFromJsonDateString(currentTime)) {
            if (DEBUG_OUT) Serial.print(F("EVSE_setOnsendHeartbeat Callback: Request has been accepted!\n"));
          } else {
            Serial.print(F("EVSE_setOnsendHeartbeat Callback: Request accepted. But Error reading time string. Expect format like 2020-02-01T20:53:32.486Z\n"));
          }
        } else {
          Serial.print(F("EVSE_setOnsendHeartbeat Callback: Request denied. Missing field currentTime. Expect format like 2020-02-01T20:53:32.486Z\n"));
        }
    });   
  });
 
	EVSE_setOnAuthentication([] () {
		if (DEBUG_OUT) Serial.print(F("EVSE_setOnAuthentication Callback: Authenticating...\n"));
		flag_evseAuthenticate = false;
		OcppOperation *authorize = makeOcppOperation(&webSocket, new Authorize(currentIdTag));
		initiateOcppOperation(authorize);
		chargePointStatusService->authorize(currentIdTag);
		authorize->setOnReceiveConfListener([](JsonObject payload) {
			const char* status = payload["idTagInfo"]["status"] | "Invalid";
			if (!strcmp(status, "Accepted")) {
				flag_evseReadIdTag = false;
				flag_evseAuthenticate = false;
				flag_evseStartTransaction = true; //Entry condition for starting transaction.
				flag_evRequestsCharge = false;
				flag_evseStopTransaction = false;
				flag_evseUnauthorise = false;
				
				if (DEBUG_OUT) Serial.print(F("EVSE_setOnAuthentication Callback: Authorize request has been accepted! Calling StartTransaction Block.\n"));
        requestLed(BLUE,START,1);
				#if CP_ACTIVE 
				flag_controlPAuthorise = true;
				#endif

			} else {
				flag_evseReadIdTag = false;
				flag_evseAuthenticate = false;
				flag_evseStartTransaction = false;
				flag_evRequestsCharge = false;
				flag_evseStopTransaction = false;
				flag_evseUnauthorise = true; //wrong ID tag received, so clearing the global current ID tag variable and setting up to read again.
        resumeTxn.putBool("ongoingTxn",false);
        resumeTxn.putString("idTagData","");
				if (DEBUG_OUT) Serial.print(F("EVSE_setOnAuthentication Callback: Authorize request has been denied! Read new User ID. \n"));
			}  
		});
	});
	
	EVSE_setOnStartTransaction([] () {
		flag_evseStartTransaction = false;
		OcppOperation *startTransaction = makeOcppOperation(&webSocket, new StartTransaction());
		initiateOcppOperation(startTransaction);
		startTransaction->setOnReceiveConfListener([](JsonObject payload) {
    		const char* status = payload["idTagInfo"]["status"] | "Invalid";
      if (!strcmp(status, "Accepted")) { 

      flag_evseReadIdTag = false;
      flag_evseAuthenticate = false;
      flag_evseStartTransaction = false;
      flag_evRequestsCharge = true;
      flag_evseStopTransaction = false;
      flag_evseUnauthorise = false;
      if (DEBUG_OUT) Serial.print(F("EVSE_setOnStartTransaction Callback: StartTransaction was successful\n"));
      //*****Storing tag data in EEPROM****//
	    resumeTxn.putString("idTagData",currentIdTag);
      resumeTxn.putBool("ongoingTxn",true);
      //***********************************//

      } else {
        flag_evseReadIdTag = false;
        flag_evseAuthenticate = false;
        flag_evseStartTransaction = false;
        flag_evRequestsCharge = false;
        flag_evseStopTransaction = false;
        flag_evseUnauthorise = true; //wrong ID tag received, so clearing the global current ID tag variable and setting up to read again.
        if (DEBUG_OUT) Serial.print(F("EVSE_setOnStartTransaction Callback: StartTransaction was unsuccessful\n"));

    	#if CP_ACTIVE
		requestforCP_OUT(STOP);  //stop pwm
		#endif
		//resume namespace
		resumeTxn.putBool("ongoingTxn",false);
		resumeTxn.putString("idTagData","");

      }
		});
	});
			
	EVSE_setOnStopTransaction([] () {
		flag_evseStopTransaction = false;
		OcppOperation *stopTransaction = makeOcppOperation(&webSocket, new StopTransaction());
		initiateOcppOperation(stopTransaction);
    	if (DEBUG_OUT) Serial.print(F("EVSE_setOnStopTransaction  before Callback: Closing Relays.\n"));

    	/**********************Until Offline functionality is implemented***********/
    	//Resume namespace(Preferences)
    	resumeTxn.putBool("ongoingTxn",false);
    	resumeTxn.putString("idTagData","");

      if(wifi_connect == true){
      	if(!webSocketConncted || WiFi.status() != WL_CONNECTED || isInternetConnected == false ){
          chargePointStatusService->stopTransaction();
      		chargePointStatusService->unauthorize();  //can be buggy 
          flag_evseReadIdTag = true;
          flag_evseAuthenticate = false;
          flag_evseStartTransaction = false;
          flag_evRequestsCharge = false;
          flag_evseStopTransaction = false;
          flag_evseUnauthorise = false;
      		Serial.println("[Wifi] Clearing Stored ID tag in StopTransaction()");
      	}
      }else if(gsm_connect == true){
          if(client.connected() == false ){
          chargePointStatusService->stopTransaction();
          chargePointStatusService->unauthorize();  //can be buggy 
          flag_evseReadIdTag = true;
          flag_evseAuthenticate = false;
          flag_evseStartTransaction = false;
          flag_evRequestsCharge = false;
          flag_evseStopTransaction = false;
          flag_evseUnauthorise = false;
          Serial.println("[GSM] Clearing Stored ID tag in StopTransaction()");
        }
      }
    	requestForRelay(STOP,1);
      delay(100);
      requestForRelay(STOP,2);
    //  requestLed(VOILET,START,1);
    	delay(500);
    	/***************************************************************************/
		stopTransaction->setOnReceiveConfListener([](JsonObject payload) {
			flag_evseReadIdTag = false;
			flag_evseAuthenticate = false;
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = false;
			flag_evseUnauthorise = true;

      requestForRelay(STOP,1);
      delay(100);
      requestForRelay(STOP,2);
    //  requestLed(VOILET,START,1);
      delay(100);
      		if (DEBUG_OUT) Serial.print(F("EVSE_setOnStopTransaction Callback: Closing Relays.\n"));
			if (DEBUG_OUT) Serial.print(F("EVSE_setOnStopTransaction Callback: StopTransaction was successful\n"));
			if (DEBUG_OUT) Serial.print(F("EVSE_setOnStopTransaction Callback: Reinitializing for new transaction. \n"));
		});
	});
	
	EVSE_setOnUnauthorizeUser([] () {
		if(flag_evseSoftReset == true){
			//This 'if' block is developed by @Wamique.
			flag_evseReadIdTag = false;
			flag_evseAuthenticate = false;
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = false;
			flag_evseUnauthorise = false;
			flag_rebootRequired = true;
			if (DEBUG_OUT) Serial.println("EVSE_setOnUnauthorizeUser Callback: Initiating Soft reset");
		} else if(flag_evseSoftReset == false){
			flag_evseReadIdTag = true;
			flag_evseAuthenticate = false;
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = false;
			flag_evseUnauthorise = false;
			if (DEBUG_OUT) Serial.print(F("EVSE_setOnUnauthorizeUser Callback: Unauthorizing user and setting up for new user ID read.\n"));
			chargePointStatusService->unauthorize();
			
		}
	});

  if (DEBUG_OUT) Serial.println("Waiting for Web Socket Connction...");
  while (!webSocketConncted && wifi_connect == true){
    Serial.print("*");
	  delay(50); //bit**
	  webSocket.loop();
    bluetooth_Loop();
	} //can add break, when Offline functionality is implemented
//#endif

	EVSE_initialize();
	Serial.println("Stored ID:"+String(idTagData_m));
  Serial.println("Ongoing Txn: "+String(ongoingTxn_m));
 
	//requestLed(GREEN,START,1);
 // PowerCycleInit();             

  Serial.println("End of Setup");
  startBLETime = millis();

} //end of setup()




void loop() {

Serial.println("****************************************************************************S***********************************************");
	#if BLE_ENABLE
	if(millis() - startBLETime < TIMEOUT_BLE){
		bluetooth_Loop();
		flagswitchoffBLE = true;
	}else{
		if(flagswitchoffBLE == true){
			flagswitchoffBLE = false;
			Serial.println("Disconnecting BT");
    		//SerialBT.println("Wifi Connected");
    		SerialBT.println("Disconnecting BT");
    		delay(100);
    		SerialBT.flush();
   			SerialBT.disconnect();
   			SerialBT.end();
   			Serial.println(ESP.getFreeHeap());
		}
	}
	#endif

	ocppEngine_loop();

  emergencyRelayClose_Loop();
  
	EVSE_loop();

  internetLoop();


   cloudConnectivityLed_Loop();

	chargePointStatusService->loop();
	meteringService->loop();
//	PowerCycle_Loop();
	
	#if CP_ACTIVE
  	ControlP_loop();
	#endif

  ota_Loop();
	Serial.println("FREE HEAP");
	Serial.println(ESP.getFreeHeap());
	Serial.println("\n*********************************************************E**************************************");
  delay(700);

}


/*
   Called by Websocket library on incoming message on the internet link
*/
//#if WIFI_ENABLED || ETHERNET_ENABLED
extern OnSendHeartbeat onSendHeartbeat;
int wscDis_counter = 0;
int wscConn_counter = 0;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	switch (type) {
		case WStype_DISCONNECTED:
			webSocketConncted = false;
      Serial.println("Counter:"+String(wscDis_counter)); 
      if(DEBUG_OUT) Serial.print(F("[WSc] Disconnected!!!\n"));
			if(wscDis_counter++>2){
        delay(200);
        Serial.println("Trying to reconnect to WSC endpoint");
				wscDis_counter = 0;
				Serial.println("URL:"+String(url_m));
       // webSocket.beginSslWithCA("ws.semaconnect.in", 443, "/D0F20E3950800000", ENDPOINT_CA_CERT);
				webSocket.begin(host_m, port_m, url_m, protocol_m);
        while (!webSocketConncted){                              //how to take care if while loop fails
          Serial.print("..**..");
          delay(100); //bit**
          webSocket.loop();                                     //after certain time stop relays and set fault state
          if(wscConn_counter++>30){
            wscConn_counter = 0;
            Serial.println("[Wsc] Unable To Connect");
            break;
          }
        }
			}
			    //have to add websocket.begin if websocket is unable to connect //Static variable
			break;
		case WStype_CONNECTED:
		webSocketConncted = true;
			if(DEBUG_OUT) Serial.printf("[WSc] Connected to url: %s\n", payload);
			break;
		case WStype_TEXT:
			if (DEBUG_OUT) if(DEBUG_OUT) Serial.printf("[WSc] get text: %s\n", payload);

			if (!processWebSocketEvent(payload, length)) { //forward message to OcppEngine
				if(DEBUG_OUT) Serial.print(F("[WSc] Processing WebSocket input event failed!\n"));
			}
			break;
		case WStype_BIN:
			if(DEBUG_OUT) Serial.print(F("[WSc] Incoming binary data stream not supported"));
			break;
		case WStype_PING:
			// pong will be send automatically
			if(DEBUG_OUT) Serial.print(F("[WSc] get ping\n"));
			break;
		case WStype_PONG:
			// answer to a ping we send
			if(DEBUG_OUT) Serial.print(F("[WSc] get pong\n"));
			break;
	}
}
//#endif



//#if WIFI_ENABLED
int wifi_counter = 0;
void wifi_Loop(){
    Serial.println("[WiFi_Loop]");
      if (WiFi.status() != WL_CONNECTED) {
        delay(200);
        if(wifi_counter++ > 50 && (WiFi.status() != WL_CONNECTED) ){ 
          wifi_counter = 0;
          Serial.print(".");         
          WiFi.disconnect();
          delay(1000);      
          Serial.println("[WIFI] Trying to reconnect again");
          WiFi.reconnect();
          //WiFi.begin(ssid_m.c_str(),key_m.c_str());  
          delay(2000);       
      }
    }
}
//#endif
short int counterPing = 0;
void cloudConnectivityLed_Loop(){


  if(wifi_connect == true){
    if(counterPing++ >= 3){   // sending ping after every 30 sec [if internet is not there sending ping packet itself consumes 10sec]
      isInternetConnected = webSocket.sendPing();
      Serial.println("*Sending Ping To Server: "+ String(isInternetConnected));
      counterPing = 0;
   }
	 if((WiFi.status() != WL_CONNECTED || webSocketConncted == false || isInternetConnected == false ) && getChargePointStatusService()->getEmergencyRelayClose() == false){ //priority is on fault
	 	 if(millis() - timercloudconnect > 10000){ //updates in 5sec
	 		  #if LED_ENABLED
	 		  requestLed(BLINKYWHITE, START, 1);   //No internet
	 		  #endif
	 		  timercloudconnect = millis();
	 	 }
	}}else if(gsm_connect == true && client.connected() ==  false && getChargePointStatusService()->getEmergencyRelayClose() == false){
        if(millis() - timercloudconnect > 10000){ //updates in 5sec
          #if LED_ENABLED
          requestLed(BLINKYWHITE, START, 1);   //No internet
          #endif
          timercloudconnect = millis();

        }
  }

}

void connectToWebsocket(){

  url_m = String(ws_url_prefix_m);
  String cpSerial = String("");
  EVSE_getChargePointSerialNumber(cpSerial);
  url_m += cpSerial; //most OCPP-Server require URLs like this. Since we're testing with an echo server here, this is obsolete

//#if WIFI_ENABLED || ETHERNET_ENABLED
  Serial.println(url_m);
  webSocket.begin(host_m, port_m, url_m, protocol_m);
// webSocket.begin("35.190.199.146", 8080, /stationServer/websocket/Wx2, protocol_m);
  // event handler
 // ws://35.190.199.146:8080/stationServer/websocket/Wx2
  webSocket.onEvent(webSocketEvent);

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);
//#endif
}
