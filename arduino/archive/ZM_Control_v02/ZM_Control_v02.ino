#define ZMDIR   6    // DIR pin - motor direction
#define ZMPWR   5    // PWM pin activates power
#define ZMCUR   A3   // analog input of current
#define ZMSLP   4    // sleep enable

#define ZMDIRVAR	1

#define SWTOP   3    // limit switch at max height
#define SWBOT   2    // limit switch at full retract

#define DELAYSLOW	100  // 10hz
#define DELAYFAST	10		// 100hz
#define STATECHANGEDELAY	15  // .01 s
#define FORCEDURATION		20	// .1 s
unsigned long now = 0, prevMillisSlow=0, prevMillisFast=0;
unsigned int stateDelay = 0, forceDuration = 0;

#define	INPUTHDR	0x80
#define	OUTPUTHDR	0xFD
#define ZMACK		0xFA
#define ZMNACK		0xFB
#define ZMINVALID	0xF0

#define	ZMUP		0x20
#define ZMDOWN		0x30
#define ZMSTOP		0x40
#define ZMSTAT		0x50

#define ZMSTEPUP	0x25
#define ZMSTEPDOWN	0x35
#define ZMDISTUP	0x21
#define ZMDISTDOWN	0x31

#define	ZMSTATDOWN	0xA0
#define	ZMSTATUP	0xB0
#define	ZMSTATMID	0xC0
#define	ZMSTATTOPSW	0xD0
#define	ZMSTATBOTSW	0xD8
#define	ZMSTATRUNUP 0xA8
#define	ZMSTATRUNDN	0xB8
#define	ZMSTATERROR	0xC8

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
int erCode = 0;		// 0 = no error
					// 1 = system init at unknown height
					// 2 = limit switch unexpected value during up
					// 3 = limit switch unexpected value during down

#define AVGELEMENTS	10
unsigned long curSum, curAvgSum;
unsigned int curNow = 0, curLast = 0, curIndex = 0, curAvg = 0, curRaw = 0, timeRunning = 0;
unsigned int curAvgArray[AVGELEMENTS];
byte curAvgByte = 0;

// 500 = 1.75" = 3428
#define CURRENTPERFOOT	3658
int estHeight = 0, curTarget = 0;
boolean heightFlag = false;

int recvHeader = 0;
byte indx = 0;
byte inputByte[16];

int topState = 0;
int botState = 0;
int stateFlag = 0;  // idle, waiting for input, zm fully retracted
				//  1 = idle, waiting for input, zm fully extended
				//  2 = zm driving up
				//  4 = zm not driving, zm not fully up or down
				//  3 = zm driving down
				//	5 = zm force down
				//  6 = zm force up

//#define SERIAL_EN      //uncomment this line to enable serial IO debug messages, leave out if you want low power
#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif
void setup() {

	Serial.begin(19200);
	
	inputString.reserve(32);
	
	pinMode(ZMDIR, OUTPUT);  //
	pinMode(ZMPWR, OUTPUT);  //  low = off, high = on
	digitalWrite(ZMPWR, LOW);

	pinMode(ZMSLP, OUTPUT);  // low = off, high = on
	digitalWrite(ZMSLP, LOW);

	pinMode(SWTOP, INPUT_PULLUP);

	pinMode(SWBOT, INPUT_PULLUP);

	readSwitches();

	resetStateClean();

	sendResponseHeader();
	Serial.write(ZMSTATDOWN);
	sendResponseFooter();
	//DEBUG("topFlag: ");DEBUGln(topState);
	//DEBUG("botFlag: ");DEBUGln(botState);
	//Serial.print("ZMBOOT:");
	//Serial.println(stateFlag);
	//Serial.println("");

}






void loop() {
processSerial();
 // 10 hz loop

	if (millis() - prevMillisSlow >= DELAYSLOW) {
	   //Serial.println("slow");

		doCurrentMeasure();

		if (stringComplete) {
			//DEBUGln("Str Rec");
			//inputString.toUpperCase();
			//DEBUG(indx);DEBUG(":");

			
			
			if ((inputByte[0] == ZMUP) && (indx < 1)) {
				if ((stateFlag == 0) || (stateFlag == 4)) { 			// idle, zm retracted
					zmUpState2();
					curSum = 0;
					timeRunning = 0;
					stateDelay = STATECHANGEDELAY;
					sendAck();
				} else if (stateFlag == 1) { 	// idle, zm extended
					sendNack();
				} else if (stateFlag == 2) {	// zm driving up
					sendAck();
				} else if (stateFlag == 3) {	// zm driving down	
					sendNack();
				} 
			} else if ((inputByte[0] == ZMDISTUP) && (indx == 1)) {
				DEBUGln(inputByte[1]);
				if ((stateFlag == 0) || (stateFlag == 4)) { 			// idle, zm retracted
					heightFlag = true;
					estHeight = 0;
					curSum = 0;
					timeRunning = 0;
					stateDelay = STATECHANGEDELAY;
					zmUpState2();
					sendAck();
				} else if (stateFlag == 1) { 	// idle, zm extended
					sendNack();
				} else if (stateFlag == 2) {	// zm driving up
					sendAck();
				} else if (stateFlag == 3) {	// zm driving down	
					sendNack();
				}
			} else if ((inputByte[0] == ZMDOWN) && (indx < 1)) {
				if (stateFlag == 0) { 			// idle, zm retracted
					sendNack();
				} else if ((stateFlag == 1) || (stateFlag == 4)) { 	// idle, zm extended
					zmDownState3();
					curSum = 0;
					timeRunning = 0;
					stateDelay = STATECHANGEDELAY*4;
					sendAck();
				} else if (stateFlag == 2) {	// zm driving up
					sendNack();
				} else if (stateFlag == 3) {	// zm driving down	
					sendAck();
				} 
			} else if ((inputByte[0] == ZMSTOP) && (indx < 1)) {
				if (stateFlag == 0) { 			// idle, zm retracted
					sendNack();
				} else if ((stateFlag == 1) || (stateFlag == 4)) { 	// idle, zm extended
					sendNack();
				} else if (stateFlag == 2) {	// zm driving up
					zmOff();	// turn off zm
					stateFlag = 4;
					sendAck();
				} else if (stateFlag == 3) {	// zm driving down	
					zmOff();	// turn off zm
					stateFlag = 4;
					sendAck();
				} 
			} else if ((inputByte[0] == ZMSTAT) && (indx < 1)) {
				DEBUG(stateFlag); DEBUG(":");
				if (stateFlag == 0) { 			// idle, zm retracted
					sendResponseHeader();
					Serial.write(ZMSTATDOWN);
					sendResponseFooter();
					
				} else if (stateFlag == 1) { 	// idle, zm extended
					sendResponseHeader();
					Serial.write(ZMSTATUP);
					sendResponseFooter();
				} else if (stateFlag == 2) {	// zm driving up
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
				} else if (stateFlag == 3) {	// zm driving down	
					sendResponseHeader();
					Serial.write(ZMSTATRUNDN);
					Serial.write(curAvgByte);
					//Serial.print(":");
					//Serial.print(timeRunning);
					//Serial.print(":");
					//Serial.print("estimateheight");
					//Serial.println("");
					sendResponseFooter();					
				} else if (stateFlag == 4) {	// error
					sendResponseHeader();
					Serial.write(ZMSTATMID);
					//Serial.write(erCode);
					sendResponseFooter();
				}
			} else if (inputString == "CLRER") {
				if (stateFlag == 0) { 			// idle, zm retracted
					//sendNack();
				} else if (stateFlag == 1) { 	// idle, zm extended
					//zmDownState3();
					//sendAck();
				} else if (stateFlag == 2) {	// zm driving up
					//sendNack();
				} else if (stateFlag == 3) {	// zm driving down	
					//sendAck();
				} else if (stateFlag == 4) {	// error
					//sendNack();
					resetStateClean();
					erCode = 0;
					sendAck();
				}
			} else if ((inputByte[0] == ZMSTEPUP) && (indx < 1)) {
				forceDuration = FORCEDURATION;
				zmUpState2();
				stateFlag = 6;
			} else if ((inputByte[0] == ZMSTEPDOWN) && (indx < 1)) {
				forceDuration = FORCEDURATION;
				zmDownState3();
				stateFlag = 5;
			
			} else {
				sendResponseHeader();
				Serial.write(ZMINVALID);
				sendResponseFooter();
				//Serial.println(0xF0);
			}
		indx = 0;
		inputString = "";
		stringComplete = false;
		}
	   prevMillisSlow = millis();
	}


 
  /// --------  100 hz loop  ------

	if (millis() - prevMillisFast >= DELAYFAST) {
	   //Serial.println("fast");   
	   
		readSwitches();
		switch (stateFlag) {
			case 0:		// idle, waiting for input, zm fully retracted
				//zmOff();
				break;
			case 1:		// idle, waiting for input, zm fully extended
				//zmOff();
				break;
			case 2:		// zm driving up
				if ((topState == LOW) && (botState == HIGH)) { // top limit hit
					zmOff();	// turn off zm
					stateFlag = 1;
					sendResponseHeader();
					Serial.write(ZMSTATUP);
					sendResponseFooter();
				} else if((topState == HIGH) && (botState == HIGH)) {  // somewhere between top and bottom
					// all is well...
					if (curNow >= 150) {
						DEBUGln("Error 6: over current");
						zmOff();
						stateFlag = 4;
						stateDelay = 1;
						erCode = 6;
					}
					if (heightFlag) {
						if (curSum > CURRENTPERFOOT) {
							zmOff();
							stateFlag = 1;
							heightFlag = false;
							//Serial.print("c:");Serial.println(curSum);
							//Serial.print("t:");Serial.println(timeRunning);
						}
					}
				} else if((topState == HIGH) && (botState == LOW)) {  // bottom limit hit/error
					if (--stateDelay < 1) {
						DEBUGln("Error 4: bottom limit during upward drive");
						zmOff();
						stateFlag = 4;
						stateDelay = 1;
						erCode = 2;
					}
				}
				break;
			case 3:		// zm driving down
				if ((topState == HIGH) && (botState == LOW)) {  // bottom limit hit
					zmOff();
					stateFlag = 0;
					sendResponseHeader();
					Serial.write(ZMSTATDOWN);
					sendResponseFooter();
				} else if ((topState == HIGH) && (botState == HIGH)) {  //  somewhere in between
					//curSum += analogRead(ZMCUR);
				} else if ((topState == LOW) && (topState == HIGH)) {  // top limit hit/error
					if (--stateDelay < 1) {
						DEBUGln("Error 3: top limit during downward drive");
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
				if ((botState = LOW) || (--forceDuration < 1)) {
					zmOff();
					resetStateClean();
				} else {
				
				}
				break;
			case 6:		// zm force up
				if ((topState = LOW) || (--forceDuration < 1)) {
					zmOff();
					resetStateClean();
				} else {
				
				}
			default:
				break;

		}
		prevMillisFast = millis();
	}

}



void zmOff() {
	digitalWrite(ZMPWR, LOW);
	digitalWrite(ZMSLP, LOW);
}

void zmUpState2 () {
	digitalWrite(ZMSLP, HIGH);		// wake up driver
	digitalWrite(ZMDIR, ZMDIRVAR);  // set driver direction

	digitalWrite(ZMPWR, HIGH);		// power on ZM

	stateFlag = 2;
}

void zmDownState3 () {
	digitalWrite(ZMSLP, HIGH);		// wake up driver
	digitalWrite(ZMDIR, !ZMDIRVAR);  // set driver direction

	digitalWrite(ZMPWR, HIGH);		// power on ZM

	stateFlag = 3;
}

void readSwitches() {
	topState = !digitalRead(SWTOP);
	botState = !digitalRead(SWBOT);
	//DEBUG(topState);DEBUG(":");DEBUGln(botState);
}

void doCurrentMeasure() {
	if ((stateFlag == 2) || (stateFlag == 3)) {
		timeRunning++;
		curLast = curNow;
		curRaw = analogRead(ZMCUR);
		//DEBUG("CR: ");	DEBUGln(curRaw);
		// 5 volts / 1024 units or, .0049 volts (4.9 mV) per unit
		//  CS pin: 40mV/A + 50mV offset
		//  y = (x - 10) / 40 
		curRaw = (curRaw-10)*10;
		curNow = curRaw / 8;
		curSum += curNow;
	
		if (++curIndex > AVGELEMENTS-1) { curIndex = 0; }
		
		curAvgArray[curIndex] = curNow;
		DEBUGln(curAvgArray[curIndex]);
		curAvgSum = 0;
		for (int i = 0; i < AVGELEMENTS-1, i++;) {
			curAvgSum += curAvgArray[i];
		}
		curAvg = int(curAvgSum / AVGELEMENTS);
		DEBUG("CN: ");	DEBUGln(curNow);
		DEBUG("CA: ");	DEBUGln(curAvg);
		//DEBUG("CS: ");  DEBUGln(curSum);
	} else {
		curLast = curNow = curSum = 0;
	}
}

void sendAck() {
	//Serial.println("ACK");
	sendResponseHeader();
	Serial.write(ZMACK);
	sendResponseFooter();
}

void sendNack() {
	//Serial.println("NACK");
	sendResponseHeader();
	Serial.write(ZMNACK);
	sendResponseFooter();
}

void sendResponseHeader() {
	Serial.write(OUTPUTHDR);
	Serial.write(OUTPUTHDR);
}

void sendResponseFooter() {
	Serial.write(0x0D);
	Serial.write(0x0A);
}

void processSerial() {
  
  
  while (Serial.available() > 0) {

    // get the new byte:
    byte inChar = (byte)Serial.read();
	//DEBUGln(inChar);
    // add it to the inputString:
	if ((inChar == INPUTHDR) && (recvHeader < 2)) {
		
		recvHeader++;
		//DEBUGln(recvHeader);
		break;
	} else if (recvHeader < 2) {
		recvHeader = 0;
		indx = 0;
	}
	if (recvHeader == 2) {
		if (inChar == 0x0D) {  // linefeed
			stringComplete = true;
			recvHeader = 0;
			indx--;
			//DEBUGln("true");
		} else {
			//DEBUGln(inChar);
			inputByte[indx++] = inChar; 
		}
	}
  }
}

void resetStateClean() {
	if ((topState == LOW) && (botState == HIGH)) { // top limit hit
		stateFlag = 1;
	} else if (topState == botState) { //&& (botState == HIGH)) {  // somewhere between top and bottom
		DEBUGln("Error 1: unexpected limit states");
		stateFlag = 4;
		erCode = 1;
	} else if ((topState == botState) && (botState == LOW)) {  // bottom limit hit/error
		stateFlag = 0;
	}
}