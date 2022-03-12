// matth-x/ESP8266-OCPP
// Copyright Matthias Akstaller 2019 - 2020
// MIT License

#include "Variants.h"

#include "RemoteStopTransaction.h"
#include "OcppEngine.h"

extern bool flag_evseIsBooted;
extern bool flag_evseReadIdTag;
extern bool flag_evseAuthenticate;
extern bool flag_evseStartTransaction;
extern bool flag_evRequestsCharge;
extern bool flag_evseStopTransaction;
extern bool flag_evseUnauthorise;

RemoteStopTransaction::RemoteStopTransaction() {

}

const char* RemoteStopTransaction::getOcppOperationType(){
	return "RemoteStopTransaction";
}


// sample message: [2,"9f639cdf-8a81-406c-a77e-60dff3cb93eb","RemoteStopTransaction",{"transactionId":2042}]
void RemoteStopTransaction::processReq(JsonObject payload) {
	transactionId = payload["transactionId"];
	if (transactionId == getChargePointStatusService()->getTransactionId()){
		flag_evseReadIdTag = false;
		flag_evseAuthenticate = false;
		flag_evseStartTransaction = false;
		flag_evRequestsCharge = false;
		flag_evseStopTransaction = true;
		flag_evseUnauthorise = false;
		//getChargePointStatusService()->stopTransaction();
	}
	
}

DynamicJsonDocument* RemoteStopTransaction::createConf(){
	DynamicJsonDocument* doc = new DynamicJsonDocument(JSON_OBJECT_SIZE(1));
	JsonObject payload = doc->to<JsonObject>();
	payload["status"] = "Accepted";
	return doc;
}

DynamicJsonDocument* RemoteStopTransaction::createReq() {
	DynamicJsonDocument *doc = new DynamicJsonDocument(JSON_OBJECT_SIZE(1));
	JsonObject payload = doc->to<JsonObject>();

	payload["idTag"] = "fefed1d19876";

	return doc;
}

void RemoteStopTransaction::processConf(JsonObject payload){
	String status = payload["status"] | "Invalid";

	if (status.equals("Accepted")) {
		if (DEBUG_OUT) Serial.print(F("[RemoteStopTransaction] Request has been accepted!\n"));
		} else {
			Serial.print(F("[RemoteStopTransaction] Request has been denied!"));
	}
}
