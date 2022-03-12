// matth-x/ESP8266-OCPP
// Copyright Matthias Akstaller 2019 - 2020
// MIT License

#ifndef EVSE_H
#define EVSE_H

#include <Arduino.h>
#include<Preferences.h>
#include<WiFi.h>
#include "ChargePointStatusService.h"
#include "OcppEngine.h"
#include "Variants.h"
#include "Peripherals.h"
/*
#if STEVE

#define WS_HOST "13.233.136.157"
#define PORT 8080

#if WIFI_ENABLED || ETHERNET_ENABLED
#define WS_URL_PREFIX "ws://13.233.136.157:8080/steve/websocket/CentralSystemService/"
#endif

#if GSM_ENABLED
#define WS_URL_PREFIX "ws://steve/websocket/CentralSystemService/" //omit Host address
#endif

#define WS_PROTOCOL "ocpp1.6"

#endif
/*******************/

/*
#if EVSECEREBRO

#define WS_HOST "188.165.223.94"
#define PORT 8140

#if WIFI_ENABLED || ETHERNET_ENABLED
#define WS_URL_PREFIX "ws://evcp.evsecerebro.com/websocket/amplify/"
#endif

#if GSM_ENABLED
#define WS_URL_PREFIX "ws://websocket/amplify/" //omit Host address
#endif

#define WS_PROTOCOL "ocpp1.6"

#endif

/*
#define RELAY_1 4
#define RELAY_2 5
#define RELAY_HIGH HIGH
#define RELAY_LOW LOW
*/

// https://cs.nyu.edu/courses/spring12/CSCI-GA.3033-014/Assignment1/function_pointers.html
typedef void (*OnBoot)();
typedef void (*OnReadUserId)();
typedef void (*OnSendHeartbeat)();
typedef void (*OnAuthentication)();
typedef void (*OnStartTransaction)();
typedef void (*OnStopTransaction)();
typedef void (*OnUnauthorizeUser)();

void EVSE_initialize();

void EVSE_setChargingLimit(float limit);

//not used
bool EVSE_EvRequestsCharge();

//not used
bool EVSE_EvIsPlugged();

//new names defined by @Pulkit. Might break the build.
void EVSE_setOnBoot(OnBoot onBt);
void EVSE_setOnReadUserId(OnReadUserId onReadUsrId);
void EVSE_setOnsendHeartbeat(OnSendHeartbeat onSendHeartbt);
void EVSE_setOnAuthentication(OnAuthentication onAuthenticatn);
void EVSE_setOnStartTransaction(OnStartTransaction onStartTransactn);
void EVSE_setOnStopTransaction(OnStopTransaction onStopTransactn);
void EVSE_setOnUnauthorizeUser(OnUnauthorizeUser onUnauthorizeUsr);

//not used
//void EVSE_afterEvPlug (AfterEvPlug afterEvPlug);
const ulong TIMEOUT_EMERGENCY_RELAY_CLOSE = 60000;
void EVSE_loop();
void emergencyRelayClose_Loop();
//other details.
void EVSE_getChargePointSerialNumber(String &out);
char *EVSE_getChargePointVendor();
char *EVSE_getChargePointModel();

//to read the ID Tag value.
String EVSE_getCurrnetIdTag(MFRC522 * mfrc522);
String EVSE_readRFID(MFRC522 * mfrc522);
void EVSE_stopTransactionByRfid();

#endif
