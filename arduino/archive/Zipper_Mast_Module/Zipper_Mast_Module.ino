// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	: 4/26/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :
// =======================================================================
// Change History   	:  
//        4/26/2017 - Initial commit      
//		  4/28/2017 - Cleaned up code and compiled it on avr  
//    
// =======================================================================
#include "Zipper.h"

#define	ZMSTATDOWN	    0xA0
#define DELAYSLOW	    100         // 10hz
#define DELAYFAST	    10		    // 100hz
#define ZMDIR           6           // DIR pin - motor direction
#define ZMPWR           5           // PWM pin activates power
#define ZMSLP           4           // sleep enable
unsigned long now, prevMillisSlow, prevMillisFast;

Zipper zip;
void setup()
{
    Serial.begin(19200);

    //set up pin modes
    pinMode(ZMDIR, OUTPUT);  
	pinMode(ZMPWR, OUTPUT);  
    pinMode(ZMSLP, OUTPUT);  
    pinMode(SWTOP, INPUT_PULLUP);
    pinMode(SWBOT, INPUT_PULLUP);

	digitalWrite(ZMPWR, LOW);
	digitalWrite(ZMSLP, LOW);

    //initialize before doing anything -> function needs work
    if(!zip.init())
        Serial.println("Houston, we have a problem :(");

    zip.readSwitches();
    zip.resetStateClean();

    zip.sendResponseHeader();
    Serial.write(ZMSTATDOWN);
    zip.sendResponseFooter();
}

void loop()
{
    zip.processSerial();

    /// --------  10 hz loop  -------
    if (millis() - prevMillisSlow >= DELAYSLOW) 
    {
        zip.doCurrentMeasure();

        if(zip.stringComplete)
        {
            zip.process_inputByte();
            prevMillisSlow = millis();
        }
    }

    /// --------  100 hz loop  ------
    if (millis() - prevMillisFast >= DELAYFAST)
    {
        zip.readSwitches();
        prevMillisFast = millis();
    }
}
