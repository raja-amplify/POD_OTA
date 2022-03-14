/*
 * FOTA_OCPP.h
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


#ifndef FOTA_OCPP_H
#define FOTA_OCPP_H

#include "OcppMessage.h"


class FotaNotification : public OcppMessage {
public:
	FotaNotification();

	const char* getOcppOperationType();

	//DynamicJsonDocument* createReq();
	void createReq();
	void processConf(JsonObject payload);

	DynamicJsonDocument* processReq(JsonObject payload);

	DynamicJsonDocument* createConf();
};



#endif
