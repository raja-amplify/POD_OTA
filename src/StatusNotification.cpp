// matth-x/ESP8266-OCPP
// Copyright Matthias Akstaller 2019 - 2020
// MIT License

#include "Variants.h"

#include "StatusNotification.h"

#include <string.h>

StatusNotification::StatusNotification(ChargePointStatus currentStatus) 
	: currentStatus(currentStatus) {

	if (!getJsonDateStringFromSystemTime(timestamp, JSONDATE_LENGTH)){
		Serial.print(F("[StatusNotification] Error reading time string. Expect format like 2020-02-01T20:53:32.486Z\n"));
	}

	if (DEBUG_OUT) {
		Serial.print(F("[StatusNotification] New StatusNotification with timestamp "));
		Serial.print(timestamp);
		Serial.print(". New Status: ");
	}
	
/*	EEPROM.begin(sizeof(EEPROM_Data));
	switch (currentStatus) {
		case (ChargePointStatus::Available):
			if (DEBUG_OUT) Serial.print(F("Available\n"));
		//	EEPROM.put(8, "Available");
			break;
		case (ChargePointStatus::Preparing):
			if (DEBUG_OUT) Serial.print(F("Preparing\n"));
		//	EEPROM.put(8, "Preparing");
			break;
		case (ChargePointStatus::Charging):
			if (DEBUG_OUT) Serial.print(F("Charging\n"));
		//	EEPROM.put(8, "Charging");
			break;
		case (ChargePointStatus::SuspendedEVSE):
			if (DEBUG_OUT) Serial.print(F("SuspendedEVSE\n"));
			EEPROM.put(8, "SuspendedEVSE");
			break;
		case (ChargePointStatus::SuspendedEV):
			if (DEBUG_OUT) Serial.print(F("SuspendedEV\n"));
			EEPROM.put(8, "SuspendedEV");
			break;
		case (ChargePointStatus::Finishing):
			if (DEBUG_OUT) Serial.print(F("Finishing\n"));
			EEPROM.put(8, "Finishing");
			break;
		case (ChargePointStatus::Reserved):
			if (DEBUG_OUT) Serial.print(F("Reserved\n"));
			EEPROM.put(8, "Reserved");
			break;
		case (ChargePointStatus::Unavailable):
			if (DEBUG_OUT) Serial.print(F("Unavailable\n"));
			EEPROM.put(8, "Unavailable");
			break;
		case (ChargePointStatus::Faulted):
			if (DEBUG_OUT) Serial.print(F("Faulted\n"));
			EEPROM.put(8, "Faulted");
			break;
		case (ChargePointStatus::NOT_SET):
			Serial.print(F("NOT_SET\n"));
			break;
		default:
			Serial.print(F("[Error, invalid status code]\n"));
		break;
	}
	EEPROM.commit();
	EEPROM.end();*/
}

const char* StatusNotification::getOcppOperationType(){
	return "StatusNotification";
}

//TODO if the status has changed again when sendReq() is called, abort the operation completely (note: if req is already sent, stick with listening to conf). The ChargePointStatusService will enqueue a new operation itself
DynamicJsonDocument* StatusNotification::createReq() {
	DynamicJsonDocument *doc = new DynamicJsonDocument(JSON_OBJECT_SIZE(4) + (JSONDATE_LENGTH + 1));
	JsonObject payload = doc->to<JsonObject>();

	payload["connectorId"] = 1;        //Hardcoded to be one because only one connector is supported
	payload["errorCode"] = "NoError";  //No error diagnostics support
	currentStatus = getChargePointStatusService()->inferenceStatus();
	//Part of Offline Implementation

	//EEPROM.begin(sizeof(EEPROM_Data));
	switch (currentStatus) {
		case (ChargePointStatus::Available):
		payload["status"] = "Available";
	//	EEPROM.put(8, "Available");
	break;
		case (ChargePointStatus::Preparing):
		payload["status"] = "Preparing";
	//	EEPROM.put(8, "Preparing");
	break;
		case (ChargePointStatus::Charging):
		payload["status"] = "Charging";
	//	EEPROM.put(8, "Charging");
	break;
		case (ChargePointStatus::SuspendedEVSE):
		payload["status"] = "SuspendedEVSE";
	//	EEPROM.put(8, "SuspendedEVSE");
	break;
		case (ChargePointStatus::SuspendedEV):
		payload["status"] = "SuspendedEV";
	//	EEPROM.put(8, "SuspendedEV");
	break;
		case (ChargePointStatus::Finishing):
		payload["status"] = "Finishing";
	//	EEPROM.put(8, "Finishing");
	break;
		case (ChargePointStatus::Reserved):
		payload["status"] = "Reserved";
	//	EEPROM.put(8, "Reserved");
	break;
		case (ChargePointStatus::Unavailable):
		payload["status"] = "Unavailable";
	//	EEPROM.put(8, "Unavailable");
	break;
		case (ChargePointStatus::Faulted):
		payload["status"] = "Faulted";
	//	EEPROM.put(8, "Faulted");
	break;
	default:
		payload["status"] = "NOT_SET";
		Serial.print(F("[StatusNotification] Error: Sending status NOT_SET!\n"));
		break;
	}
//	EEPROM.commit();
//	EEPROM.end();
	payload["timestamp"] = timestamp;

	return doc;
}


void StatusNotification::processConf(JsonObject payload) {
	/*
	* Empty payload
	*/
}


/* 
* For debugging only
*/
StatusNotification::StatusNotification() {

}

/*
* For debugging only
*/
void StatusNotification::processReq(JsonObject payload) {

}

/*
* For debugging only
*/
DynamicJsonDocument* StatusNotification::createConf(){
	DynamicJsonDocument* doc = new DynamicJsonDocument(0);
	JsonObject payload = doc->to<JsonObject>();
	return doc;
}
