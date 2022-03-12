// matth-x/ESP8266-OCPP
// Copyright Matthias Akstaller 2019 - 2020
// MIT License

#include "MeteringService.h"
#include "OcppOperation.h"
#include "MeterValues.h"
#include "OcppEngine.h"
#include "SimpleOcppOperationFactory.h"
#include "display.h"
#include <EEPROM.h>

bool flag_MeteringIsInitialised = false;

MeteringService::MeteringService(WebSocketsClient *webSocket)
      : webSocket(webSocket) {
  
  sampleTimeA = LinkedList<time_t>();
  sampleTimeB = LinkedList<time_t>();
  sampleTimeC = LinkedList<time_t>();
  power = LinkedList<float>();
  energyA = LinkedList<float>();
  energyB = LinkedList<float>();
  energyC = LinkedList<float>();
  voltageA = LinkedList<float>();
  voltageB = LinkedList<float>();
  voltageC = LinkedList<float>();
  currentA = LinkedList<float>();
  currentB = LinkedList<float>();
  currentC = LinkedList<float>();
  temperature = LinkedList<float>();

  setMeteringSerivce(this); //make MeteringService available through Ocpp Engine

}

void MeteringService::addDataPoint(time_t currentTime, float currentPower, float currentEnergy, float currentVoltage, float currentCurrent, float currentTemperature){
	
	if (getChargePointStatusService() != NULL && getChargePointStatusService()->getTransactionId() == -1){
		sampleTimeA.add(currentTime);
		power.add(currentPower);
		energyA.add(currentEnergy);
		voltageA.add(currentVoltage);
		currentA.add(currentCurrent);
		temperature.add(currentTemperature);
		
		/*EEPROM.begin(sizeof(EEPROM_Data));
		EEPROM.put(4, currentEnergy);   
		EEPROM.commit();
		EEPROM.end();*/
		

		lastSampleTimeA = currentTime;
		//lastPower = currentPower;

		/*
		* Check if to send all the meter values to the server
		*/
		if (power.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& energyA.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& voltageA.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& currentA.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& temperature.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH) {
		flushLinkedListValues();
		}			
	} else 	if (getChargePointStatusService() != NULL && getChargePointStatusService()->getConnectorId() == 1){
		sampleTimeA.add(currentTime);
		power.add(currentPower);
		energyA.add(currentEnergy);
		voltageA.add(currentVoltage);
		currentA.add(currentCurrent);
		temperature.add(currentTemperature);
		

		EEPROM.begin(sizeof(EEPROM_Data));
		EEPROM.put(4, currentEnergy);   
		EEPROM.commit();
		EEPROM.end();
		

		lastSampleTimeA = currentTime;
		//lastPower = currentPower;

		/*
		* Check if to send all the meter values to the server
		*/
		if (power.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& energyA.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& voltageA.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& currentA.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& temperature.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH) {
		flushLinkedListValues();
		}			
	} else if (getChargePointStatusService() != NULL && getChargePointStatusService()->getConnectorId() == 2){
		sampleTimeB.add(currentTime);
		power.add(currentPower);
		energyB.add(currentEnergy);
		voltageB.add(currentVoltage);
		currentB.add(currentCurrent);
		temperature.add(currentTemperature);

		lastSampleTimeB = currentTime;
		//lastPower = currentPower;

		/*
		* Check if to send all the meter values to the server
		*/
		if (power.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& energyB.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& voltageB.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& currentB.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& temperature.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH) {
		flushLinkedListValues();
		}			
	} else 	if (getChargePointStatusService() != NULL && getChargePointStatusService()->getConnectorId() == 3){
		sampleTimeC.add(currentTime);
		power.add(currentPower);
		energyC.add(currentEnergy);
		voltageC.add(currentVoltage);
		currentC.add(currentCurrent);
		temperature.add(currentTemperature);

		lastSampleTimeC = currentTime;
		//lastPower = currentPower;

		/*
		* Check if to send all the meter values to the server
		*/
		if (power.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& energyC.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& voltageC.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& currentC.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH
			&& temperature.size() >= METER_VALUES_SAMPLED_DATA_MAX_LENGTH) {
		flushLinkedListValues();
		}			
	} 
	if (DEBUG_OUT) Serial.println("It is NULL");
}


void MeteringService::flushLinkedListValues() {
  if (getChargePointStatusService() != NULL) {
    if (getChargePointStatusService()->getTransactionId() == -1){
		if (power.size() == 0 && energyA.size() == 0 && voltageA.size() == 0 && currentA.size() == 0 && temperature.size() == 0) return; //Nothing to report

		OcppOperation *meterValues = makeOcppOperation(webSocket,
		new MeterValues(&sampleTimeA, &power, &energyA, &voltageA, &currentA, &temperature));
		initiateOcppOperation(meterValues);
		sampleTimeA.clear();
		power.clear();
		energyA.clear();
		voltageA.clear();
		currentA.clear();
		temperature.clear();
	} else if (getChargePointStatusService()->getConnectorId() == 1){
		if (power.size() == 0 && energyA.size() == 0 && voltageA.size() == 0 && currentA.size() == 0 && temperature.size() == 0) return; //Nothing to report

		if (getChargePointStatusService()->getTransactionId() < 0) {      
		  sampleTimeA.clear();
		  power.clear();
		  energyA.clear();
		  voltageA.clear();
		  currentA.clear();
		  temperature.clear();
		  return;
		}
		OcppOperation *meterValues = makeOcppOperation(webSocket,
		new MeterValues(&sampleTimeA, &power, &energyA, &voltageA, &currentA, &temperature));
		initiateOcppOperation(meterValues);
		sampleTimeA.clear();
		power.clear();
		energyA.clear();
		voltageA.clear();
		currentA.clear();
		temperature.clear();
	} else if (getChargePointStatusService()->getConnectorId() == 2){
		if (power.size() == 0 && energyB.size() == 0 && voltageB.size() == 0 && currentB.size() == 0 && temperature.size() == 0) return; //Nothing to report
		if (getChargePointStatusService()->getTransactionId() < 0) {      
		  sampleTimeB.clear();
		  power.clear();
		  energyB.clear();
		  voltageB.clear();
		  currentB.clear();
		  temperature.clear();
		  return;
		}	
		OcppOperation *meterValues = makeOcppOperation(webSocket,
		new MeterValues(&sampleTimeB, &power, &energyB, &voltageB, &currentB, &temperature));
		initiateOcppOperation(meterValues);
		sampleTimeB.clear();
		power.clear();
		energyB.clear();
		voltageB.clear();
		currentB.clear();
		temperature.clear();
		
	} else if (getChargePointStatusService()->getConnectorId() == 3){
		if (power.size() == 0 && energyC.size() == 0 && voltageC.size() == 0 && currentC.size() == 0 && temperature.size() == 0) return; //Nothing to report
		if (getChargePointStatusService()->getTransactionId() < 0) {      
		  sampleTimeC.clear();
		  power.clear();
		  energyC.clear();
		  voltageC.clear();
		  currentC.clear();
		  temperature.clear();
		  return;
		}
		OcppOperation *meterValues = makeOcppOperation(webSocket,
		new MeterValues(&sampleTimeC, &power, &energyC, &voltageC, &currentC, &temperature));
		initiateOcppOperation(meterValues);
		sampleTimeC.clear();
		power.clear();
		energyC.clear();
		voltageC.clear();
		currentC.clear();
		temperature.clear();
		
	}
  }
}


void MeteringService::loop(){

  /*
   * Calculate energy consumption which finally should be reportet to the Central Station in a MeterValues.req.
   * This code uses the EVSE's own energy register, if available (i.e. if energySampler is set). Otherwise it
   * uses the power sampler.
   * If no powerSampler is available, estimate the energy consumption taking the Charging Schedule and CP Status
   * into account.
   */
   
   
   
   //@bug: fix it for multiple connectors.
   if (DEBUG_OUT_M) Serial.println("The last sample time is: ");
   if (DEBUG_OUT_M) Serial.println(lastSampleTimeA + METER_VALUE_SAMPLE_INTERVAL);
   if (DEBUG_OUT_M) Serial.println("The now() is: ");
   if (DEBUG_OUT_M) Serial.println(now());
   
   if ((now() >= (time_t) METER_VALUE_SAMPLE_INTERVAL + lastSampleTimeA)) {
	  if (energyASampler != NULL 
		&& powerASampler != NULL
		&& voltageASampler != NULL
		&& currentASampler != NULL
		&& temperatureSampler != NULL){

			time_t sampledTimeA = now();
			time_t deltaA = sampledTimeA - lastSampleTimeA;
			float sampledVoltage = voltageASampler();			
			float sampledCurrent = currentASampler();
			float sampledPower = powerASampler(sampledVoltage, sampledCurrent);			
			float sampledEnergyA = energyASampler(sampledVoltage, sampledCurrent, deltaA);
			float sampledTemperature = temperatureSampler();
			if (DEBUG_OUT_M) Serial.println("Inside if cond.");
			addDataPoint(sampledTimeA, sampledPower, sampledEnergyA, sampledVoltage, sampledCurrent, sampledTemperature);
			
			//addDataPoint(sampledTimeA, 0, 0,sampledVoltage, sampledCurrent, sampledTemperature);
		}
	}
}



void MeteringService::init(MeteringService *meteringService){
  
  flag_MeteringIsInitialised = true;
	
  meteringService->lastSampleTimeA = now(); //0 means not charging right now
  meteringService->lastSampleTimeB = now(); //0 means not charging right now
  meteringService->lastSampleTimeC = now(); //0 means not charging right now
  
   /*
   * initialize EEPROM
   */
  int isInitialized;
  float currEnergy;
  EEPROM.begin(sizeof(EEPROM_Data));
  EEPROM.get(0,isInitialized);
  EEPROM.get(4,currEnergy);
  if (isInitialized == 22121996){
  	if(currEnergy > 100000){
  	//	EEPROM.put(0, 0);
	  	EEPROM.put(4, 0.0f);
  	}
	
  } else if (isInitialized != 22121996){
	  isInitialized = 22121996;
	  EEPROM.put(0, isInitialized);
	  EEPROM.put(4, 0.0f);
  }/* else if (currEnergy > 1000000){
	  EEPROM.put(0, 0);
	  EEPROM.put(4, 0.0f);
  }*/
  
  EEPROM.commit();
  EEPROM.end();
  Serial.println("[Mertering init] isInitialized "+ String(isInitialized));
  Serial.println("[Mertering init] current energy: "+ String(currEnergy));

  meteringService->setVoltageASampler([]() {
    return (float) eic.GetLineVoltageA();
  });
  meteringService->setVoltageBSampler([]() {
    return (float) eic.GetLineVoltageB();
  });
  meteringService->setVoltageCSampler([]() {
    return (float) eic.GetLineVoltageC();
  });
  meteringService->setCurrentASampler([]() {
    return (float) eic.GetLineCurrentA(); 
  });
  meteringService->setCurrentBSampler([]() {
    return (float) eic.GetLineCurrentB(); 
  });
  meteringService->setCurrentCSampler([]() {
    return (float) eic.GetLineCurrentC(); 
  });
  meteringService->setEnergyASampler([](float volt, float current, time_t delta) {
	EEPROM.begin(sizeof(EEPROM_Data));
	float lastEnergy;
	EEPROM.get(4,lastEnergy);
	float finalEnergy = lastEnergy + ((float) (volt*current*((float)delta)))/3600;
	EEPROM.put(4, finalEnergy);
	EEPROM.commit();
	EEPROM.end();
	if (DEBUG_OUT) Serial.println("EEPROM Energy Register Value: ");
	if (DEBUG_OUT) Serial.println(finalEnergy);
	return finalEnergy;
  });
  meteringService->setPowerASampler([](float volt, float current) {
	return volt*current/1000;
  });
  meteringService->setEnergyBSampler([]() {
    return (float) eic.GetLineCurrentC(); //@bug: change this to energy.
  });
  meteringService->setEnergyCSampler([]() {
    return (float) eic.GetLineCurrentC(); //@bug: change this to energy.
  });
  meteringService->setTemperatureSampler([]() {
    return (float) eic.GetTemperature(); //example values. Put your own power meter in heres
  });
  
  timer_init = false;
	
}

float MeteringService::currentEnergy(){
	EEPROM.begin(sizeof(EEPROM_Data));
	float lastEnergy;
	EEPROM.get(4,lastEnergy);
	float volt = voltageASampler();
	float curr = currentASampler();
	time_t delta = now() - lastSampleTimeA;
	float finalEnergy = energyASampler(volt, curr, delta);
	EEPROM.put(4, finalEnergy);
	EEPROM.commit();
	EEPROM.end();
	if (DEBUG_OUT_M) Serial.println("The last Energy is is: ");
		if (DEBUG_OUT_M) Serial.println(lastEnergy);
if (DEBUG_OUT_M) Serial.println("The Final Energy is is: ");
	if (DEBUG_OUT_M) Serial.println(finalEnergy);

	return finalEnergy;
	
}//wamique, can you flash this one?

/*void MeteringService::setPowerSampler(float (*ps)()){
  this->powerSampler = ps;
}*/

void MeteringService::setEnergyASampler(float (*es)(float volt, float current, time_t delta)){
  this->energyASampler = es;
}

void MeteringService::setPowerASampler(float (*ps)(float volt, float current)){
  this->powerASampler = ps;
}

void MeteringService::setEnergyBSampler(float (*es)()){
  this->energyBSampler = es;
}

void MeteringService::setEnergyCSampler(float (*es)()){
  this->energyCSampler = es;
}

void MeteringService::setVoltageASampler(float (*vs)()){
  this->voltageASampler = vs;
}

void MeteringService::setVoltageBSampler(float (*vs)()){
  this->voltageBSampler = vs;
}

void MeteringService::setVoltageCSampler(float (*vs)()){
  this->voltageCSampler = vs;
}

void MeteringService::setCurrentASampler(float (*cs)()){
  this->currentASampler = cs;
}

void MeteringService::setCurrentBSampler(float (*cs)()){
  this->currentBSampler = cs;
}

void MeteringService::setCurrentCSampler(float (*cs)()){
  this->currentCSampler = cs;
}

void MeteringService::setTemperatureSampler(float (*ts)()){
  this->temperatureSampler = ts;
}