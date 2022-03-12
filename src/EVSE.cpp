// matth-x/ESP8266-OCPP
// Copyright Matthias Akstaller 2019 - 2020
// MIT License

/**
Edited by Pulkit Agrawal.
*/

#include "EVSE.h"
#include "Master.h"
#include "ControlPilot.h"

//timeout for heartbeat signal.
ulong T_SENDHEARTBEAT = 60000;
bool timeout_active=false;
bool timer_initialize = false;
ulong timeout_start=0;
extern ATM90E36 eic;

//new variable names defined by @Pulkit. might break the build.
OnBoot onBoot;
OnReadUserId onReadUserId;
OnSendHeartbeat onSendHeartbeat;
OnAuthentication onAuthentication;
OnStartTransaction onStartTransaction;
OnStopTransaction onStopTransaction;
OnUnauthorizeUser onUnauthorizeUser;

//new flag names. replace them with old names.
bool evIsPlugged; 
bool flag_evseIsBooted;
bool flag_evseReadIdTag;
bool flag_evseAuthenticate;
bool flag_evseStartTransaction;
bool flag_evRequestsCharge;
bool flag_evseStopTransaction;
bool flag_evseUnauthorise;
bool flag_rebootRequired;
bool flag_evseSoftReset; //added by @Wamique

//not used. part of Smart Charging System.
float chargingLimit = 32.0f;
String Ext_currentIdTag = "";
extern MFRC522 mfrc522;
extern String currentIdTag;
long int blinckCounter =0;
int counter1 =0;

ulong t;
ulong relay_timer = 0;
int connectorDis_counter=0;
void EVSE_StartCharging();
void EVSE_Suspended();
void EVSE_StopSession();

extern EVSE_states_enum EVSE_state;
extern Preferences preferences;

short int fault_counter = 0;
bool flag_faultOccured = false;

short int counter_drawingCurrent = 0;
float drawing_current = 0;

extern bool webSocketConncted;
extern bool isInternetConnected;
short counter_faultstate = 0;
//initialize function. called when EVSE is booting. 
//NOTE: It should be also called when there is a reset or reboot required. create flag to control that. @Pulkit
/**********************************************************/
void EVSE_StopSession(){

	if(getChargePointStatusService()->getEvDrawsEnergy() == true){
		getChargePointStatusService()->stopEvDrawsEnergy();
	}
	
	//digitalWrite(32, LOW);
    requestForRelay(STOP,1); 
    delay(100);
    requestForRelay(STOP,2);
    delay(100);           	
    flag_evseReadIdTag = false;
	flag_evseAuthenticate = false;
	flag_evseStartTransaction = false;
	flag_evRequestsCharge = false;
	flag_evseStopTransaction = true;
	flag_evseUnauthorise = false;
    Serial.println("[EVSE] Stopping Session : " +String(EVSE_state));
}
/**************************************************************************/

void EVSE_initialize() {
	if(DEBUG_OUT) Serial.print(F("[EVSE] EVSE is powered on or reset. Starting Boot.\n"));
	onBoot();
}

//This is the main loop function which is controlling the whole charfing process. All the flags created are used to control the flow of the program. 



void EVSE_loop() {
	
	if (flag_evseIsBooted == false){
		if(DEBUG_OUT) Serial.println("[EVSE] Booting...");
		delay(1000);
		//onBoot();
		t = millis();
		return;
	} else if (flag_evseIsBooted == true) {
		if (flag_evseReadIdTag == true) {
			if (onReadUserId != NULL) {
				onReadUserId();
			}
			return;
		} else if (flag_evseAuthenticate == true) {
			if (onAuthentication != NULL) {
				onAuthentication();
				
			}
			return;
		} else if (flag_evseStartTransaction == true) {
			if (onStartTransaction != NULL) {
				#if CP_ACTIVE
				if((EVSE_state == STATE_C || EVSE_state == STATE_D) && getChargePointStatusService()->getEmergencyRelayClose() == false){
					onStartTransaction();
				}else{
					Serial.println("Connect the Connector to EV / Or fault exist");     //here have to add timeout of 30 sec
					connectorDis_counter++;
					//EVSE_stopTransactionByRfid();
					if(connectorDis_counter > 25){
						connectorDis_counter = 0;

						EVSE_StopSession();
					}
					
				}
				#endif

				#if !CP_ACTIVE
					onStartTransaction();
				#endif

			}
		} else if (flag_evRequestsCharge == true){

		#if CP_ACTIVE
			//flag_evRequestsCharge = false;
				if (getChargePointStatusService() != NULL && getChargePointStatusService()->getEvDrawsEnergy() == false) {
				
			/***********************Control Pilot @Wamique******************/
					if(EVSE_state == STATE_C || EVSE_state == STATE_D){
						if (getChargePointStatusService()->getEmergencyRelayClose() == false) {
							EVSE_StartCharging();
						} else if (getChargePointStatusService()->getEmergencyRelayClose() == true) {
							Serial.println("The voltage / current / Temp is out or range. FAULTY CONDITION DETECTED.");
						}
					}else if(EVSE_state == STATE_SUS){
						EVSE_Suspended();
						Serial.println(counter1);
						if(counter1++ > 25){    //Have to implement proper timeout
							counter1 = 0;
							EVSE_StopSession();
						}

					}else if(EVSE_state == STATE_DIS || EVSE_state == STATE_E || EVSE_state == STATE_B || EVSE_state == STATE_A){
				
					//	EVSE_StopSession();     // for the very first time cable can be in disconnected state

						//if(txn == true){           // can implement counter > 10 just to remove noise
						EVSE_StopSession();
				//	}

					}else{

						Serial.println("[EVSE] STATE Error" +String(EVSE_state));
						delay(2000);

					//	requestLed(RED,START,1);
					}
				} 
				if(getChargePointStatusService()->getEvDrawsEnergy() == true){

				//	txn = true;
				
					if(EVSE_state == STATE_C || EVSE_state == STATE_D ){

						if(DEBUG_OUT) Serial.println("[EVSE_CP] Drawing Energy");	

						if(millis() - t > 10000){
				 			if(getChargePointStatusService()->getEmergencyRelayClose() == false){
				 				requestLed(BLINKYGREEN,START,1);
				 				t = millis();
				 			}
				 		}
						/*
						if(blinckCounter++ % 2 == 0){
							requestLed(GREEN,START,1);
						}else{
							requestLed(GREEN,STOP,1);
						}*/
					}else if(EVSE_state == STATE_A || EVSE_state == STATE_E || EVSE_state == STATE_B){//Although CP Inp will never go to A,B state
						if(counter_faultstate++ > 5){
						 EVSE_StopSession();
						 counter_faultstate = 0;
						}
                
					}else if(EVSE_state == STATE_SUS){
						EVSE_Suspended();    //pause transaction :update suspended state is considered in charging state

					}else if(EVSE_state == STATE_DIS){

						Serial.println("[EVSE] Connect the Connector with EV and Try again");
						EVSE_StopSession();						
				}                      
			}

			 /***Implemented Exit Feature with RFID @Wamique****/ 
			  EVSE_stopTransactionByRfid();
		#endif

			
			#if !CP_ACTIVE
			if (getChargePointStatusService() != NULL && getChargePointStatusService()->getEvDrawsEnergy() == false) {
				if (getChargePointStatusService()->getEmergencyRelayClose() == false) {
					getChargePointStatusService()->startEvDrawsEnergy();
					
					if (DEBUG_OUT) Serial.print(F("[EVSE] Opening Relays.\n"));
					requestForRelay(START,1);
					delay(100);
					requestForRelay(START, 2);
					requestLed(ORANGE,START,1);
    				delay(1200);
    				requestLed(WHITE,START,1);
    				delay(1200);
    				requestLed(GREEN,START,1);
   				 	delay(1000);
					if(DEBUG_OUT) Serial.println("[EVSE] Started Drawing Energy");
				} else if (getChargePointStatusService()->getEmergencyRelayClose() == true) {
						Serial.println("The voltage or current is out or range. FAULTY CONDITION DETECTED.");
					}
			} 
			if(getChargePointStatusService()->getEvDrawsEnergy() == true){
				//delay(250);
            
				 if(DEBUG_OUT) Serial.println("[EVSE] Drawing Energy");

				 //blinking green Led
				 if(millis() - t > 5000){
				 	// if((WiFi.status() == WL_CONNECTED) && (webSocketConncted == true) && (isInternetConnected == true)&& getChargePointStatusService()->getEmergencyRelayClose() == false){
				 	// 	requestLed(BLINKYGREEN_EINS,START,1);
				 	// 	t = millis();
				 	// }

					if(getChargePointStatusService()->getEmergencyRelayClose() == false){
					 		requestLed(BLINKYGREEN,START,1);
					 		t = millis();
					 		 if(millis() - relay_timer > 15000){
					 			requestForRelay(START,1);
					 			delay(100);
					 			requestForRelay(START,2);
					 			relay_timer = millis();

							 }
					 	}

					


				 }
				 //Current check
				 drawing_current = eic.GetLineCurrentA();
				 if(drawing_current <= 0.15){
				 	counter_drawingCurrent++;
				 	if(counter_drawingCurrent > 70){
				 		counter_drawingCurrent = 0;

				 		EVSE_StopSession();
				 	}

				 }else{
				 	counter_drawingCurrent = 0;
				 	Serial.println("counter_drawingCurrent Reset");

				 }
				
			}
			   //Implemented Exit Feature with RFID @Wamique//
			 EVSE_stopTransactionByRfid();
			#endif
			//this is the only 'else if' block which is calling next else if block. the control is from this file itself. the control is not changed from any other file. but the variables are required to be present as extern in other file to decide calling of other functions. 
			return;
		} else if (flag_evseStopTransaction == true) {
			if (getChargePointStatusService() != NULL) {
				getChargePointStatusService()->stopEvDrawsEnergy();

			}
			if (onStopTransaction != NULL) {
				onStopTransaction();
				#if CP_ACTIVE
				requestforCP_OUT(STOP);  //stop pwm
				#endif
			}
			return;
		} else if (flag_evseUnauthorise == true) {
			if (onUnauthorizeUser != NULL) {
				onUnauthorizeUser();
			}
			return;
		} else if (flag_rebootRequired == true) {
			//soft reset execution.
			flag_evseIsBooted = false;
			flag_rebootRequired = false;
			flag_evseSoftReset = false;
			if(DEBUG_OUT) Serial.print(F("[EVSE] rebooting in 5 seconds...\n"));
			delay(5000);
			ESP.restart();
		} else {
			if(DEBUG_OUT) Serial.print(F("[EVSE] waiting for response...\n"));
			delay(500);
		}	
	}
}

// void emergencyRelayClose_Loop(){
// 	float volt = eic.GetLineVoltageA();
// 	float current = eic.GetLineCurrentA();
// 	float temp= eic.GetTemperature();
// 	Serial.println("Voltage: "+String(volt)+", Current: "+String(current)+", Temperature: "+String(temp));
// 	if (getChargePointStatusService() != NULL) {
// 		if(getChargePointStatusService()->getOverVoltage() == true ||
// 			getChargePointStatusService()->getUnderVoltage() == true ||
// 			getChargePointStatusService()->getUnderTemperature() == true ||
// 			getChargePointStatusService()->getOverTemperature() == true ||
// 			getChargePointStatusService()->getOverCurrent() == true){
// 				Serial.println("Fault Occurred.");						
// 				getChargePointStatusService()->setEmergencyRelayClose(true);
// 				if (!timer_initialize){
// 					timeout_start = millis();
// 					timer_initialize = true;
// 				}
// 			} else if(getChargePointStatusService()->getOverVoltage() == false &&
// 					getChargePointStatusService()->getUnderVoltage() == false &&
// 					getChargePointStatusService()->getUnderTemperature() == false &&
// 					getChargePointStatusService()->getOverTemperature() == false &&
// 					getChargePointStatusService()->getOverCurrent() == false){
// 				Serial.println("Not Faulty.");						
// 				getChargePointStatusService()->setEmergencyRelayClose(false);
// 				//if (!timer_initialize){
// 					timeout_start = 0;
// 					timer_initialize = false;
// 				//}
// 			}
			
// 		if (getChargePointStatusService()->getEmergencyRelayClose() == true){
// 			timeout_active = true;
// 			requestForRelay(STOP,1);
// 			delay(500);
// 			#if LED_ENABLED
// 			requestLed(RED,START,1);
// 			#endif

// 			flag_faultOccured = true;
// 		} else if (getChargePointStatusService()->getEmergencyRelayClose() == false && flag_faultOccured == true){
// 			timeout_active = false;
// 			if ( (getChargePointStatusService()->getTransactionId() != -1)){ //can be buggy
// 				if(fault_counter++ > 2){
// 					fault_counter = 0;
// 					requestForRelay(START,1);
// 					delay(500);
// 					Serial.println("[EmergencyRelay] Starting Txn");
// 					flag_faultOccured = false;
// 				}
// 			}
// 		}

// 		if (timeout_active && getChargePointStatusService()->getTransactionId() != -1) {
// 			if (millis() - timeout_start >= TIMEOUT_EMERGENCY_RELAY_CLOSE){
// 				Serial.println("Emergency Stop.");
// 				flag_evRequestsCharge = false;
// 				flag_evseStopTransaction = true;
// 				timeout_active = false;
// 				timer_initialize = false;
// 			}
// 		}
// 	}
// }

bool relayTriggered = 0;
ulong faultTimer;
bool EMGCY_FaultOccured = false;

void emergencyRelayClose_Loop(){
	if(millis() - faultTimer > 3000){
		faultTimer = millis();
		bool EMGCY_status = requestEmgyStatus();
		delay(100);
		bool GFCI_status  = requestGfciStatus();
		Serial.println("EMGCY_Status: "+String(EMGCY_status));
		Serial.println("GFCI_Status: "+String(GFCI_status));
		if(EMGCY_status == true || GFCI_status == true){
			
			  if(relayTriggered == false){
			  	relayTriggered = true;
					requestForRelay(STOP,1);
					delay(100);
					requestForRelay(STOP, 2);
					requestLed(BLINKYRED,START,1);
				}
				Serial.println("[EMGY-GFCI] Fault occurred");
				//if(EMGCY_status == true){
					//getChargePointStatusService()->setEmgcyButtonPressed(true);
				//}else if(GFCI_status == true){
					//getChargePointStatusService()->setGfciError(true);
				//}
				getChargePointStatusService()->setEmergencyRelayClose(true);
				EMGCY_FaultOccured = true;
				//EMGCY_counter = 0;
			
		}else{
			relayTriggered = false;
			EMGCY_FaultOccured = false;
			// EMGCY_counter = 0;
		//	getChargePointStatusService()->setEmgcyButtonPressed(false);
		//	getChargePointStatusService()->setGfciError(false);
			getChargePointStatusService()->setEmergencyRelayClose(false);
		}

		if(EMGCY_FaultOccured == true && getChargePointStatusService()->getTransactionId() != -1){

			flag_evseReadIdTag = false;
			flag_evseAuthenticate = false;
			flag_evseStartTransaction = false;
			flag_evRequestsCharge = false;
			flag_evseStopTransaction = true;
		//	getChargePointStatusService()->stopEvDrawsEnergy();
		//	Serial.println("[EMGCY TX ID: " + String(getChargePointStatusService()->getTransactionId()));
		
		}else if(EMGCY_FaultOccured == false){

				float volt = eic.GetLineVoltageA();
				float current = eic.GetLineCurrentA();
				float temp= eic.GetTemperature();
				Serial.println("Voltage: "+String(volt)+", Current: "+String(current)+", Temperature: "+String(temp));
				if (getChargePointStatusService() != NULL) {
					if(getChargePointStatusService()->getOverVoltage() == true ||
						getChargePointStatusService()->getUnderVoltage() == true ||
						getChargePointStatusService()->getUnderTemperature() == true ||
						getChargePointStatusService()->getOverTemperature() == true ||
						getChargePointStatusService()->getOverCurrent() == true){
							Serial.println(F("[EVSE] Fault Occurred."));						
							getChargePointStatusService()->setEmergencyRelayClose(true);
							if (!timer_initialize){
								timeout_start = millis();
								timer_initialize = true;
							}
						} else if(getChargePointStatusService()->getOverVoltage() == false &&
								getChargePointStatusService()->getUnderVoltage() == false &&
								getChargePointStatusService()->getUnderTemperature() == false &&
								getChargePointStatusService()->getOverTemperature() == false &&
								getChargePointStatusService()->getOverCurrent() == false){
							Serial.println(F("[EVSE] Not Faulty."));						
							getChargePointStatusService()->setEmergencyRelayClose(false);
							//if (!timer_initialize){
								timeout_start = 0;
								timer_initialize = false;
							//}
						}
						
					if (getChargePointStatusService()->getEmergencyRelayClose() == true){
						timeout_active = true;
						requestForRelay(STOP,1);
						delay(50);
						requestForRelay(STOP,2);
						#if LED_ENABLED
						requestLed(RED,START,1);
						#endif

						flag_faultOccured = true;
					} else if (getChargePointStatusService()->getEmergencyRelayClose() == false && flag_faultOccured == true){
						timeout_active = false;
						if ( (getChargePointStatusService()->getTransactionId() != -1)){ //can be buggy
							if(fault_counter++ > 10){
								fault_counter = 0;
								requestForRelay(START,1);
								delay(50);
								requestForRelay(START, 2);
								Serial.println(F("[EmergencyRelay] Starting Txn"));
								flag_faultOccured = false;
							}
						}
					}

					if (timeout_active && getChargePointStatusService()->getTransactionId() != -1) {
						if (millis() - timeout_start >= TIMEOUT_EMERGENCY_RELAY_CLOSE){
							Serial.println(F("[EVSE] Emergency Stop."));
							flag_evRequestsCharge = false;
							flag_evseStopTransaction = true;
							timeout_active = false;
							timer_initialize = false;
						}
					}
				}
		    }
   }
}

/*
* @param limit: expects current in amps from 0.0 to 32.0
*/
void EVSE_setChargingLimit(float limit) {
	if(DEBUG_OUT) Serial.print(F("[EVSE] New charging limit set. Got "));
	if(DEBUG_OUT) Serial.print(limit);
	if(DEBUG_OUT) Serial.print(F("\n"));
	chargingLimit = limit;
}

bool EVSE_EvRequestsCharge() {
	return flag_evRequestsCharge;
}

bool EVSE_EvIsPlugged() {
	return evIsPlugged;
}


void EVSE_setOnBoot(OnBoot onBt){
	onBoot = onBt;
}

void EVSE_setOnReadUserId(OnReadUserId onReadUsrId){
	onReadUserId = onReadUsrId;
}

void EVSE_setOnsendHeartbeat(OnSendHeartbeat onSendHeartbt){
	onSendHeartbeat = onSendHeartbt;
}

void EVSE_setOnAuthentication(OnAuthentication onAuthenticatn){
	onAuthentication = onAuthenticatn;
}

void EVSE_setOnStartTransaction(OnStartTransaction onStartTransactn){
	onStartTransaction = onStartTransactn;
}

void EVSE_setOnStopTransaction(OnStopTransaction onStopTransactn){
	onStopTransaction = onStopTransactn;
}

void EVSE_setOnUnauthorizeUser(OnUnauthorizeUser onUnauthorizeUsr){
	onUnauthorizeUser = onUnauthorizeUsr;
}

void EVSE_getSsid(String &out) {
	out += "Pied Piper";
}
void EVSE_getPass(String &out) {
	out += "plmzaq123";
}


void EVSE_getChargePointSerialNumber(String &out) {


	out += preferences.getString("chargepoint","");

	/*
	#if STEVE
	out += "dummyCP002";
	#endif

	#if EVSECEREBRO
	out += "testpodpulkit";
	#endif
	*/
}



char *EVSE_getChargePointVendor() {
	return "Amplify Mobility";
}

char *EVSE_getChargePointModel() {
	return "POD_BB";
}

String EVSE_getCurrnetIdTag(MFRC522 * mfrc522) {
    //String currentIdTag = "";
	currentIdTag = EVSE_readRFID(mfrc522);
	
	if (getChargePointStatusService()->getIdTag().isEmpty() == false){
		if(DEBUG_OUT) Serial.println("[EVSE] Reading from Charge Point Station Service ID Tag stored.");
		currentIdTag = getChargePointStatusService()->getIdTag();
		if(DEBUG_OUT) Serial.print("[EVSE] ID Tag: ");
		if(DEBUG_OUT) Serial.println(currentIdTag);
		Serial.flush();
	}
	
	return currentIdTag;
}


String EVSE_readRFID(MFRC522 * mfrc522) {
	String currentIdTag;	
	currentIdTag = readRfidTag(true, mfrc522);
	return currentIdTag;
}



/********Added new funtion @Wamique***********************/

void EVSE_stopTransactionByRfid(){

	Ext_currentIdTag = EVSE_readRFID(&mfrc522);
	if(currentIdTag.equals(Ext_currentIdTag) == true){
		flag_evRequestsCharge = false;
		flag_evseStopTransaction = true;
	}else{
			if(Ext_currentIdTag.equals("")==false)
			if(DEBUG_OUT) Serial.println("\n[EVSE] Incorrect ID tag\n");
		}
}



#if CP_ACTIVE
/**************CP Implementation @mwh*************/
void EVSE_StartCharging(){

	if(getChargePointStatusService()->getEvDrawsEnergy() == false){
		getChargePointStatusService()->startEvDrawsEnergy();
	}
    if (DEBUG_OUT) Serial.print(F("[EVSE] Opening Relays.\n"));
 //   pinMode(32,OUTPUT);
  //  digitalWrite(32, HIGH); //RELAY_1
    //digitalWrite(RELAY_2, RELAY_HIGH);
    requestForRelay(START,1);
    delay(100);
    requestForRelay(START,2);
    requestLed(ORANGE,START,1);
    delay(1200);
    requestLed(WHITE,START,1);
    delay(1200);
    requestLed(GREEN,START,1);
    delay(1000);
    Serial.println("[EVSE] EV is connected and Started charging");
	if(DEBUG_OUT) Serial.println("[EVSE] Started Drawing Energy");
	delay(500);
}


void EVSE_Suspended(){


	if(getChargePointStatusService()->getEvDrawsEnergy() == true){
		getChargePointStatusService()->stopEvDrawsEnergy();
	}
		requestLed(BLUE,START,1);
		requestForRelay(STOP,1);
		delay(100);
		requestForRelay(STOP,2);
	//	delay(1000);
		Serial.printf("[EVSE] EV Suspended");


}



/**************************************************/

#endif