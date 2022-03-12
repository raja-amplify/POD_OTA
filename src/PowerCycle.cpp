#include "PowerCycle.h"
#include "display.h"
#include<Preferences.h>
ulong cycleTimer = 0;
short waitCounter = 0;
bool ongoingTxn_p;
String idTagData_p="";
extern Preferences resumeTxn;

void PowerCycleInit(){

	cycleTimer = millis();
}

void PowerCycle_Loop(){

	// if(millis() - cycleTimer > 4321110){
	// 	Serial.println("PowerCycle is Scheduled: "+String(waitCounter));
	// 	idTagData_p= resumeTxn.getString("idTagData","");
 //  		ongoingTxn_p = resumeTxn.getBool("ongoingTxn",false);
	// 	Serial.println("Stored ID:"+String(idTagData_p));
 //  		Serial.println("Ongoing Txn: "+String(ongoingTxn_p));
	// 	if(ongoingTxn_p == 0 && idTagData_p == ""){
	// 		if(waitCounter++ > 20){ //to allow some buffer time and let things stablize
	// 			waitCounter = 0;   //not required still following convention
	// 			Serial.println("Rebooting Device in 5 sec");
	// 			thanks_Disp("Please wait....");
	// 			delay(2000);
	// 			ESP.restart();
	// 		}
	// 	}
	// }
}