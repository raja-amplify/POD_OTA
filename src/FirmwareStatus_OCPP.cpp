/*
 * FOTA_OCPP.cpp
 * 
 * Copyright 2022 raja <raja@raja-IdeaPad-Gaming-3-15IMH05>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "Variants.h"

#include "FirmwareStatus_OCPP.h"
#include "EVSE.h"
#include "OcppEngine.h"

#include <string.h>
#include "TimeHelper.h"

/*
 * @breif: Instantiate an object FirmwareStatus
 */ 

FirmwareStatus::FirmwareStatus() {

}

/*
 * @breif: Method - getOcppOperationType => This method gives the type of Ocpp operation
 */ 

const char* FirmwareStatus::getOcppOperationType(){
	return "FirmwareStatus";
}

/*
 * @breif: Method - createReq => This method creates Fota request to be sent to the OCPP server
 * Field Name   | Field Type         | Card. | Description
 * status       | FirmwareStatus enum| 1..1  | Required. This contains the progress status of the firmware installation.
 */ 

DynamicJsonDocument* FirmwareStatus::createReq() {
	// For now the object size is 2, but with custom data, it will increase
	//DynamicJsonDocument *doc = new DynamicJsonDocument(JSON_OBJECT_SIZE(3) + strlen(EVSE_getChargePointVendor()) + 1 + cpSerial.length() + 1 + strlen(EVSE_getChargePointModel()) + 1);
	DynamicJsonDocument *doc = new DynamicJsonDocument(JSON_OBJECT_SIZE(1));
	JsonObject payload = doc->to<JsonObject>();
	payload["status"] = "enum"; // fill this enum
	//Need nested json for customData here -
	
	return doc;
}

void FirmwareStatus::processConf(JsonObject payload){
	/*
	 * OTA update should be processed.
	 */ 
}

void FirmwareStatus::processReq(JsonObject payload){
	/*
	* Ignore Contents of this Req-message, because this is for debug purposes only
	*/
}

DynamicJsonDocument* FirmwareStatus::createConf(){
	/*
	* Ignore Contents of this Req-message, because this is for debug purposes only
	*/
}
