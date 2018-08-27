// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	: 4/26/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :
// =======================================================================
// Change History   	:  
//        4/26/2017 - Initial commit       
//    
// =======================================================================
#include "Zipper.h"

Zipper::Zipper()
{

}

Zipper::~Zipper()
{
    //Do nothing??
}

bool Zipper::init()
{
    return true;
}

void Zipper::readSwitches()
{
    topState = !digitalRead(SWTOP);
	botState = !digitalRead(SWBOT);
}

void Zipper::resetStateClean()
{
    if((topState == LOW) && (botState == HIGH))
    {
        stateFlag = 1;
    }
    else if(topState == botState)
    {
        Serial.println("Error 1: unexpected limit states");
        stateFlag = 4;
        erCode = 1;
    }
    else if((topState == botState) && (botState == LOW))
    {
        stateFlag = 0;
    }
}

void Zipper::sendResponseHeader()
{
    Serial.write(OUTPUTHDR);
	Serial.write(OUTPUTHDR);
}

void Zipper::sendResponseFooter()
{
    
    Serial.write(0x0D);
	Serial.write(0x0A);
}

//Issues:
// 1) recv header is only used here??
void Zipper::processSerial()
{
    while(Serial.available() > 0)
    {
        byte inChar = (byte)Serial.read();
        if((inChar == INPUTHDR) && (recvHeader < 2))   
        {
            recvHeader++;
            break;      //hacky way of doing this
        }
        else if(recvHeader < 2)
        {
            recvHeader = 0;
            indx = 0;
        }

        if(recvHeader == 2)
        {
            if(inChar == 0x0D)
            {
                stringComplete = true;
                recvHeader = 0;
                indx--;
            }
            else
            {
                inputByte[indx++] = inChar;
            }
        }
    }
}

//Is this even being used??
void Zipper::doCurrentMeasure()
{
    if((stateFlag == 2) || stateFlag == 3)
    {
        timeRunning++;
        curLast = curNow;
        curRaw = analogRead(ZMCUR);

        curRaw = (curRaw-10)*10;
        curNow = curRaw / 8;
        curSum += curNow;

        if(++curIndex > AVGELEMENTS - 1)
            curIndex = 0;

        curAvgArray[curIndex] = curNow;
        curAvgSum = 0;
        for(int x = 0; x < AVGELEMENTS; x++)
            curAvgSum += curAvgArray[x];
        
        curAvg = int(curAvgSum / AVGELEMENTS);
    }
    else //Cancel everything out
    {
        curLast = curNow = curSum = 0;
    }
}

void Zipper::process_inputByte()
{
    if ((inputByte[0] == ZMUP) && (indx < 1)) 
    {
	    if ((stateFlag == 0) || (stateFlag == 4)) // idle, zm retracted
        { 			
		    zmUpState2();
			curSum = 0;
			timeRunning = 0;
			stateDelay = STATECHANGEDELAY;
			sendAck();
		} 
        else if (stateFlag == 1)  	// idle, zm extended
			sendNack();
        else if (stateFlag == 2)   // zm driving up	
			sendAck();
        else if (stateFlag == 3)	// zm driving down	
			sendNack();
	} 
    else if ((inputByte[0] == ZMDISTUP) && (indx == 1)) 
    {
		//DEBUGln(inputByte[1]);
		if ((stateFlag == 0) || (stateFlag == 4)) // idle, zm retracted
        { 			
			heightFlag = true;
			estHeight = 0;
			curSum = 0;
			timeRunning = 0;
			stateDelay = STATECHANGEDELAY;
			zmUpState2();
			sendAck();
		} 
        else if (stateFlag == 1) // idle, zm extended	
			sendNack();
        else if (stateFlag == 2) // zm driving up	
			sendAck();
		else if (stateFlag == 3)	// zm driving down	
			sendNack();
	} 
    else if ((inputByte[0] == ZMDOWN) && (indx < 1)) 
    {
		if (stateFlag == 0)  			// idle, zm retracted
			sendNack();
		else if ((stateFlag == 1) || (stateFlag == 4)) 
        { 	// idle, zm extended
			zmDownState3();
			curSum = 0;
			timeRunning = 0;
			stateDelay = STATECHANGEDELAY * 4;
			sendAck();
		} 
        else if (stateFlag == 2) 	// zm driving up
			sendNack();
        else if (stateFlag == 3) 	// zm driving down	
			sendAck();
	} 
    else if ((inputByte[0] == ZMSTOP) && (indx < 1)) 
    {
		if (stateFlag == 0) 			// idle, zm retracted
					sendNack();
		else if ((stateFlag == 1) || (stateFlag == 4))  	// idle, zm extended
					sendNack();
		else if (stateFlag == 2) 
        {	// zm driving up
			zmOff();	// turn off zm
			stateFlag = 4;
			sendAck();
		} 
        else if (stateFlag == 3) 
        {	// zm driving down	
			zmOff();	// turn off zm
			stateFlag = 4;
			sendAck();
		} 
	} 
    else if ((inputByte[0] == ZMSTAT) && (indx < 1)) 
    {
		if (stateFlag == 0) // idle, zm retracted
        { 			
			sendResponseHeader();
			Serial.write(ZMSTATDOWN);
			sendResponseFooter();	
		} 
        else if (stateFlag == 1) // idle, zm extended
        { 	
			sendResponseHeader();
			Serial.write(ZMSTATUP);
			sendResponseFooter();
		} 
        else if (stateFlag == 2) // zm driving up
        {	
			sendResponseHeader();
			Serial.write(ZMSTATRUNUP);
			//Serial.print("DRIVEUP:");
			Serial.write(curAvgByte);
			//Serial.print(":");
			//Serial.print(timeRunning);
			//Serial.print(":");
			//Serial.print("estimateheight");
			//Serial.println("");
			sendResponseFooter();
		} 
        else if (stateFlag == 3) // zm driving down	
        {	
			sendResponseHeader();
			Serial.write(ZMSTATRUNDN);
			Serial.write(curAvgByte);
			//Serial.print(":");
			//Serial.print(timeRunning);
			//Serial.print(":");
			//Serial.print("estimateheight");
			//Serial.println("");
			sendResponseFooter();					
		} 
        else if (stateFlag == 4)  // error
        {
			sendResponseHeader();
			Serial.write(ZMSTATMID);
			//Serial.write(erCode);
			sendResponseFooter();
		}
	} 
    else if (inputString == "CLRER")
    {
		if (stateFlag == 3) {	// zm driving down	
					//sendAck();
		} 
        else if (stateFlag == 4)  // error
        {	
			//sendNack();
			resetStateClean();
			erCode = 0;
			sendAck();
		}
	} 
    else if ((inputByte[0] == ZMSTEPUP) && (indx < 1)) 
    {
		forceDuration = FORCEDURATION;
		zmUpState2();
		stateFlag = 6;
	} 
    else if ((inputByte[0] == ZMSTEPDOWN) && (indx < 1)) 
    {
		forceDuration = FORCEDURATION;
		zmDownState3();
		stateFlag = 5;
	} 
    else 
    {
		sendResponseHeader();
		Serial.write(ZMINVALID);
		sendResponseFooter();
		//Serial.println(0xF0);
	}

    indx = 0;
    inputString = "";
    stringComplete = false;
}

void Zipper::zmUpState2()
{
    digitalWrite(ZMSLP, HIGH);  //wake up driver
    digitalWrite(ZMDIR, ZMDIRVAR); //set driver direction
    digitalWrite(ZMPWR, HIGH); 
    stateFlag = 2;
}

void Zipper::sendNack()
{
    sendResponseHeader();
    Serial.write(ZMNACK);
    sendResponseFooter();
}

void Zipper::sendAck()
{
    sendResponseHeader();
	Serial.write(ZMACK);
	sendResponseFooter();
}

void Zipper::zmOff()
{
    digitalWrite(ZMPWR, LOW);
	digitalWrite(ZMSLP, LOW);
}

void Zipper::process_stateFlag()
{
    switch (stateFlag) 
    {
	    case 0:		// idle, waiting for input, zm fully retracted
		    //zmOff();
			break;
		case 1:		// idle, waiting for input, zm fully extended
			//zmOff();
			break;
		case 2:		// zm driving up
			if ((topState == LOW) && (botState == HIGH)) // top limit hit
            { 
				zmOff();	// turn off zm
				stateFlag = 1;
				sendResponseHeader();
				Serial.write(ZMSTATUP);
				sendResponseFooter();
			} 
            else if((topState == HIGH) && (botState == HIGH)) // somewhere between top and bottom
            {  
				// all is well...
				if (curNow >= 150) 
                {
					//DEBUGln("Error 6: over current");
					zmOff();
					stateFlag = 4;
					stateDelay = 1;
					erCode = 6;
				}
				if (heightFlag) 
                {
					if (curSum > CURRENTPERFOOT) 
                    {
						zmOff();
						stateFlag = 1;
						heightFlag = false;
						//Serial.print("c:");Serial.println(curSum);
						//Serial.print("t:");Serial.println(timeRunning);
					}
				}
			}
            else if((topState == HIGH) && (botState == LOW))  // bottom limit hit/error
            {  
				if (--stateDelay < 1)
                {
					//DEBUGln("Error 4: bottom limit during upward drive");
					zmOff();
					stateFlag = 4;
					stateDelay = 1;
					erCode = 2;
				}
			}
			break;
		case 3:		// zm driving down
			if ((topState == HIGH) && (botState == LOW)) // bottom limit hit
            {  
				zmOff();
				stateFlag = 0;
				sendResponseHeader();
				Serial.write(ZMSTATDOWN);
				sendResponseFooter();
			} 
            else if ((topState == HIGH) && (botState == HIGH)) //  somewhere in between
            {  
				//curSum += analogRead(ZMCUR);
			} 
            else if ((topState == LOW) && (topState == HIGH)) // top limit hit/error
            {  
				if (--stateDelay < 1) 
                {
					//DEBUGln("Error 3: top limit during downward drive");
					zmOff();
					stateFlag = 4;
					stateDelay = 1;
					erCode = 3;
				}
			}
			break;
		case 4:    // error mode
			zmOff();
			break;
		case 5:		// zm force down
			if ((botState = LOW) || (--forceDuration < 1)) 
            {
				zmOff();
				resetStateClean();
			} 
            else 
            {
				//do nothing??
			}
			break;
		case 6:		// zm force up
			if ((topState = LOW) || (--forceDuration < 1)) 
            {
				zmOff();
				resetStateClean();
			}
            else 
            {
				//do nothing??
			}
		default:
			break;
    }
}

void Zipper::zmDownState3()
{
    digitalWrite(ZMSLP, HIGH);		// wake up driver
	digitalWrite(ZMDIR, !ZMDIRVAR);  // set driver direction
	digitalWrite(ZMPWR, HIGH);		// power on ZM
	stateFlag = 3;
}