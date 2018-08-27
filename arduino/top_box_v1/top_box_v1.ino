/*
 Name:		top_box_v1.ino
 Created:	8/7/2017 1:19:09 PM
 Author:	THOR
*/


//	comment out to disable iridium and enable a test mode that turns on the system upon power up
//#define		USEIRIDIUM

//#define		DEBUG_GPS

#include <SoftwareSerial.h>
#include <LowPower.h>
#include <GPS.h>

#ifdef USEIRIDIUM
	#include <IridiumSBD.h>
#endif

//{ --------  Bit banging macros  ------------
// Macros to define the ports and bitmasks for the pins on the 328p succinctly
// modified to be specific for atmega1284P (MoteinoMega) 
#define     DirRegFromPin(pin)      pin<8 ? 0x04 : pin<16 ? 0x0A : pin<24 ? 0x07 : 0x01 // Register which controls input or output
#define     DataRegFromPin(pin)     pin<8 ? 0x05 : pin<16 ? 0x0B : pin<24 ? 0x08 : 0x02 // Register which controls high or low
#define     BitmaskFromPin(pin)     pin<8 ? 1<<pin : pin<16 ? 1<<(pin-8) : pin<24 ? 1<<(pin-16) : 1<<(pin-24)
// Bit-banging macros - each adds 2 bytes of sketch size and takes 1 clock cycle to execute
#define     MakeOutput(pin)         _SFR_IO8(DirRegFromPin(pin))  |=  BitmaskFromPin(pin)
#define     MakeInput(pin)          _SFR_IO8(DirRegFromPin(pin))  &=  ~(BitmaskFromPin(pin))
#define     PullHigh(pin)           _SFR_IO8(DataRegFromPin(pin)) |=  BitmaskFromPin(pin)
#define     PullLow(pin)            _SFR_IO8(DataRegFromPin(pin)) &=  ~(BitmaskFromPin(pin))
//}

//{ ------- main board hardware defines --------
#define		LEDONE				15 	// pd7
#define		LEDTWO				14	// pd6
#define		LEDTHREE			13	// pd5
#define		BUTTONONE			24	// pa0
#define		BUTTONTWO			25	// pa1

#define		IRIDIUMPWR			2	// pb2
#define		RADIOPWR			3	// pb3
#define		CAMERAPWR			18	// pc2
#define		TX1PWR				17	// pc1
#define		GPSPWR				21	// pc5

#define		TPLDRV				16	// pc0
#define		TPLDONE				27	// pa3
#define		GPSFIX				20	// pc4
//}


//{ ------ zipper defines ----------
#define		INPUTHDR		0x80
#define		OUTPUTHDR		0xFD
#define		ZMACK			0xFA
#define		ZMNACK			0xFB
#define		ZMINVALID		0xF0

#define		ZMSTATDOWN		0xA0
#define		ZMSTATUP		0xB0
#define		ZMSTATMID		0xC0
#define		ZMSTATTOPSW		0xD0
#define		ZMSTATBOTSW		0xD8
#define		ZMSTATRUNUP		0xA8
#define		ZMSTATRUNDN		0xB8
#define		ZMSTATERROR		0xC8
#define		ZMRESPONSETIME	1000

enum { ZMUP, ZMDOWN, ZMSTAT, ZMSTOP, 
	ZMGOINGUP, ZMGOINGDOWN, ZMFAULT, ZMNORESPONSE }; // OUTBOUND COMMANDS AND STATUSES
//}
  
byte		zmFlag=0;
byte		zmNewStatus=0; // flag if new status is waiting
byte		zmStatus=0;
byte		zmLastStatus=0;
uint16_t	zmStatusTimer=0;
uint16_t	zmWaitForResponse=0;
byte		zmBattLevel[8];
boolean zipStringComplete = false;  // whether the string is complete
int zipRecvHeader = 0;
byte zipIndx = 0;
byte zipInputByte[16];

#define		zipSerial	Serial1

// ------ main program stuff ----
#define DELAYSLOW	100  // 10hz
#define DELAYFAST	10		// 100hz

enum {
	_STARTUP,		// SET UP MODE FOR BOOT UP
	_RUN,			// RUNNING MODE
	_GPS_ERROR,		// GPS ERROR MODE
	_IRID_ERROR,	// IRIDIUM ERROR
	_CHECK_IRID,	// Checking iridium mailbox
	_AVR_SLEEP,		// PUT THE AVR TO SLEEP
	_POWER_UP,		// payload init sequence
	_POWER_DOWN,	// payload shutdown sequence
	_CHK_ZM_DOWN
	};

#define	ZIPSTATWAIT			2000
#define SLEEP_CYCLES		16	// number of times to sleep 8 seconds between iridium checks
#define VALID_SATELLITES	4
#define GPS_RX				0
#define GPS_TX				1
#define IRID_RX				22
#define IRID_TX				23
#define IRID_SLEEP			31  // no connect
#define GPS_ERR_DELAY		1
                      
#define SERIAL_EN                //comment out if you don't want any debug output

#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

SoftwareSerial		gps_serial(GPS_RX, GPS_TX);
GPS					GPS(&gps_serial);

#ifdef USEIRIDIUM
	SoftwareSerial		nss(IRID_RX, IRID_TX);
	IridiumSBD			isbd(nss, IRID_SLEEP);
#endif

byte		needGPS = 0;
float		gpsLat=28.1234;
float		gpsLon=80.12345;
float		gpsAlt;
uint16_t	gpsTimer = 0;

//{
//boolean usingInterrupt        = true;  // this keeps track of whether we're using the interrupt
//boolean no_error              = true;
//String  zip_avr_serial_data   = "";
//String  tx1_serial_data       = "";
//}

uint8_t		currentState			= _STARTUP;
uint8_t		sleepCycles				= SLEEP_CYCLES-2;
uint8_t		b1Debounce, b2Debounce;
uint16_t	initCounter = 0;
uint16_t	schedulerTimer = 0;
byte		schedulerByte;

String inputString = "";         // a string to hold incoming data from serial0
boolean stringComplete = false;  // whether the string is complete
int recvHeader = 0;
byte indx = 0;
int serTimer=0;

void zmCommand(byte cmd, int delay=0);

void setup() {
	//{ -- OUTPUT INIT 
	MakeOutput(LEDONE);
	PullLow(LEDONE);
	MakeOutput(LEDTWO);
	PullLow(LEDTWO);
	MakeOutput(LEDTHREE);
	PullLow(LEDTHREE);
	
	MakeInput(BUTTONTWO);
	PullHigh(BUTTONTWO);
	MakeInput(BUTTONONE);
	PullHigh(BUTTONONE);
	
	MakeOutput(IRIDIUMPWR);
	PullLow(IRIDIUMPWR);	// turn off iridium modem
	MakeOutput(RADIOPWR);
	PullLow(RADIOPWR);		// turn off radio
	MakeOutput(CAMERAPWR);
	PullLow(CAMERAPWR);		// turn off camera
	MakeOutput(TX1PWR);
	PullLow(TX1PWR);		// turn off tx1
	
	MakeOutput(GPSPWR);		// gps is inverted logic
	PullHigh(GPSPWR);		// gps off
	//}
	
	Serial.begin(19200);  // primary serial port used for debug and control from tx1
	zipSerial.begin(19200);  // hw serial used for comms with zippermast
	
	DEBUGln("attempt gps begin");
	GPS.begin(9600);
#ifdef DEBUG_GPS
		DEBUGln("gps begin");
		delay(1000);
		GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
		GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
		DEBUGln("GPS init");
#endif
	DEBUGln("begin");
	currentState = 20;//_AVR_SLEEP;
	inputString.reserve(24);
	
#ifndef USEIRIDIUM
	PullLow(GPSPWR);
	delay(2000);
	GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
	GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
	needGPS = 1;
	DEBUGln("tx1 on");
	PullHigh(TX1PWR);
	DEBUGln("radio on");
	PullHigh(RADIOPWR);
	
#endif
}

uint32_t now = 0, prevMillisSlow=0, prevMillisFast=0;

void loop() {
	readMainSerial();
	readZipperSerial();
	// ------------------------------------------------------------
	// -------------------- 10hz main loop ------------------------
	// ---- debug button press handling
	// ---- wait for zippermast responses
	// ---- execute scheduled commands
	
	if (millis() - prevMillisSlow >= DELAYSLOW) {  // 10 hz loop
	//testGPS();
		if (schedulerTimer) {
			if (--schedulerTimer == 0) { // timer elapsed
				zmCommand(schedulerByte);
			}
		}
		
		if (zmWaitForResponse) {
			if (zmWaitForResponse-- == 1) { //countdown timer expired
				zmLastStatus = zmStatus;
				zmStatus = ZMNORESPONSE;
			} else {					// countdown timer still going
				
			}
		}
		
#ifdef SERIAL_EN
		// ---------  assign events to external buttons  -----------
		if (digitalRead(BUTTONONE)) { b1Debounce = 0; } else { b1Debounce++; }
		if (digitalRead(BUTTONTWO)) { b2Debounce = 0; } else { b2Debounce++; }
		if (b1Debounce == 3) {   /// button one debug action
		
		}
		if (b2Debounce == 3) {   /// button two debug action
		
		}
#endif
		prevMillisSlow = millis();
	}
	
	// ------------------------------------------------------------
	// ------------------- 100hz main loop ------------------------
	// ----  state machine
	// ----  execute serial commands
#ifndef USEIRIDIUM
	currentState = _RUN;
#endif
	if (millis() - prevMillisFast >= DELAYFAST) {  // 100 hz loop
	zipSerialResponse();
	mainSerialResponse();
	
	// ----- state machine ----
	switch (currentState) {
		case _STARTUP:
			setup();
			break;
		case _RUN:		// payload on, awaiting commands
			if (needGPS && GPS.newNMEAreceived()) {
				if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
					return;  // we can fail to parse a sentence in which case we should just wait for another
				if ((GPS.fix) && (GPS.satellites > 3)) {
					gpsLat = GPS.latitude;
					gpsLon = GPS.longitude;
					gpsAlt = GPS.altitude;
					needGPS = 0;
					PullHigh(GPSPWR);  // we got our location, turn off gps
				}
			}
			break;
		case _AVR_SLEEP:
			if (++sleepCycles > SLEEP_CYCLES) {
				currentState = _CHECK_IRID;  // check iridium
				sleepCycles = 0;
				break;
			}
#ifdef SERIAL_EN
			LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
#else
			LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
#endif
			break;
		case _CHECK_IRID:
			DEBUGln("checking iridium");
			for (int i=0; i<5; i++) {
				PullHigh(LEDTWO);
				delay(100);
				PullLow(LEDTWO);
				delay(100);
			}
			DEBUGln("iridium done");
			//if iridium command to wake
			//    initCounter = 0
			//    currentState = _POWER_UP
			//else
			currentState = _AVR_SLEEP;
			break;
		case _POWER_UP:		// power on sequence, stay active but sleep to reduce processing speed
			LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
			switch(++initCounter) {
				case 2:			// turn on camera
					DEBUGln("Camera on");
					PullHigh(CAMERAPWR);
					break;
				case 380:		// get gps started up
					DEBUGln("gps on");
					PullLow(GPSPWR);
					break;
				case 400:		// 12 sec in start the zm up
					DEBUGln("zmup");
					zmCommand(ZMUP);
					break;
				case 560:		// 17 sec turn on tx1
					DEBUGln("init gps");
					GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
					GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
					needGPS = 1;
					DEBUGln("tx1 on");
					PullHigh(TX1PWR);
					break;
				case 800:		// 24 sec turn on radio
					DEBUGln("radio on");
					PullHigh(RADIOPWR);
					currentState = _RUN;
					break;
			}
			break;
		case _POWER_DOWN:		// power off sequence.  reduce proc speed but turn it all off pretty quick
			LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
			switch(++initCounter) {
				case 2:			// get out of the sky asap
					DEBUGln("zm down");
					zmCommand(ZMDOWN);
					break;
				case 40:
					DEBUGln("camera off");
					PullLow(CAMERAPWR);
					break;
				case 80:
					DEBUGln("radio off");
					PullLow(RADIOPWR);
					break;
				case 160:
					DEBUGln("tx1 off");
					PullLow(TX1PWR);
					zmStat();
					currentState = _CHK_ZM_DOWN;
					break;
				}
			break;
		case _CHK_ZM_DOWN:  // waiting for the zipper to go down
			if (zmNewStatus) {  // we have received a status update from the zippermast
				switch (zmStatus) { // zippermast is down, we are happy
					case ZMDOWN:
						
						break;
					case ZMGOINGDOWN:   //zipper isnt down yet, but its working on it
						zmCommand(ZMSTAT, 400);
						break;
					case ZMUP:
					case ZMSTOP:
					case ZMGOINGUP:
					case ZMNORESPONSE:
						zmCommand(ZMDOWN);
						zmCommand(ZMSTAT,100);
					
					
					
					//ZMDOWN, ZMSTAT, ZMSTOP, ZMGOINGUP, ZMGOINGDOWN, ZMFAULT, ZMNORESPONSE 
				}
			}
			
			break;
		}
		prevMillisFast = millis();
	}
}

//  -------------------------------------------
//  ---  read data from serial line and parse into a single command
//  ---  then hand off to other function
void readMainSerial() {
	if (!stringComplete) {
		while (Serial.available() > 0) {
			serTimer = 0;
			// get the new byte:
			char inChar = Serial.read();
			// add it to the inputString:
			if ((inChar == 'a') && (recvHeader < 2)) {
				//Serial.println(inChar);
				recvHeader++;
				//DEBUGln(recvHeader);
				break;
			} else if (recvHeader < 2) {
				recvHeader = 0;
				indx = 0;
			}
			if (recvHeader == 2) {
				if ((inChar == '\n')) {  // linefeed
					stringComplete = true;
					recvHeader = 0;
					//indx--;
					//DEBUGln("true");
				} else {
					//DEBUGln(inChar);
					if (indx < 20) {
						inputString += inChar;
						//Serial.println(indx);
						} else {
						recvHeader = 0; // reset the device
						indx = 0;
						//stringComplete = false;
					}
				}
			}
		}
	}
}

//  ------------------------------------
//  ---  read in serial data from serial1 and parse out header and footer
void readZipperSerial() { 
	if (!zipStringComplete) {
		while (zipSerial.available() > 0) {
			serTimer = 0;
			// get the new byte:
			byte inChar = (byte)zipSerial.read();

			// add it to the inputString:
			if ((inChar == INPUTHDR) && (zipRecvHeader < 2)) {
				
				zipRecvHeader++;
				//DEBUGln(recvHeader);
				break;
			} else if (zipRecvHeader < 2) {
				zipRecvHeader = 0;
				zipIndx = 0;
			}
			if (zipRecvHeader == 2) {
				if ((inChar == 0x0D) && (zipIndx == 2)) {  // linefeed
					zipStringComplete = true;
					zipRecvHeader = 0;
					//zipIndx--;
					//DEBUGln("true");
				} else {
					//DEBUGln(inChar);
					if (zipIndx < 2) {
						zipInputByte[zipIndx++] = inChar; // zipInputByte array no longer than 3
					} else {
						zipRecvHeader = 0; // reset the device
						zipIndx = 0;
						//stringComplete = false;
					}
				}
			}
		}
	}
}

//  -------------------------------------------------------
//  -------  proess serial requests from tx1  -------------
void mainSerialResponse() {
	if (stringComplete) {  // process serial commands from tx1/console
		inputString.trim();
		if (inputString == "gps") {
			Serial.print("lat:");
			Serial.print(String(gpsLat,5));
			Serial.print(",lon:");
			Serial.println(String(gpsLon,5));
			Serial.print(",alt:");
			Serial.println(String(gpsAlt,2));
		} else if (inputString == "zmup") {
			Serial.println("ZMUP");
			zmCommand(ZMUP);
		} else if (inputString == "zmdown") {
			Serial.println("ZMDOWN");
			zmCommand(ZMDOWN);
		} else if (inputString == "camon") {
			Serial.Println("CAMON");
			PullHigh(CAMERAPWR);
		} else if (inputString == "camoff") {
			Serial.Println("CAMOFF");
			PullLow(CAMERAPWR);
		}
		inputString = "";
		stringComplete = false;
		indx = 0;
		
	}
}
//  -------------------------------------------------------
//  ------  process serial responses from zippermast ------
void zipSerialResponse() {
	if (zipStringComplete) {
		switch(zipInputByte[0]) {
			case ZMUP:
				
				break;
			case ZMDOWN:
				
				break;
			case ZMSTAT:  // receive and parse status packet from zm
				zmLastStatus = zmStatus;
				zmStatus = zipInputByte[1];
				for (int i=0; i<8; i++) { zmBattLevel[i] = zipInputByte[i+2]; }
				zmNewStatus = 1;
				break;
			case ZMSTOP:
				
				break;
			default:
				
				break;
		}
		zipStringComplete = false;
		indx = 0;
	}
}

void zmStat() {  // depreciated, use zmCommand(ZMSTAT);
	return;
	zmNewStatus = 0;
	zipSendHeader();
	zipSerial.write(ZMSTAT);
	zipSendFooter();
	zmWaitForResponse = ZMRESPONSETIME;
}

void zmCommand(byte cmd, int delay) {
	if (!delay) {
		if (cmd == ZMSTAT) zmNewStatus = 0;
		zipSendHeader();
		zipSerial.write(cmd);
		zipSendFooter();
		zmWaitForResponse = ZMRESPONSETIME;
	} else {
		schedulerTimer = delay;
		schedulerByte = cmd;
	}
}

void zipSendHeader() {
	zipSerial.write(OUTPUTHDR);
	zipSerial.write(OUTPUTHDR);
}

void zipSendFooter() {
	zipSerial.write(0x0D);
	zipSerial.write(0x0A);
}

void testGPS() {
	    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
      if (c) Serial.print(c);
	if (GPS.newNMEAreceived()) {
		if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
			return;  // we can fail to parse a sentence in which case we should just wait for another
		
		
		//if ((GPS.fix) && (GPS.satellites > 3)) {
			DEBUG("lat: ");DEBUGln(GPS.latitude);
			DEBUG("lon: ");DEBUGln(GPS.longitude);
			DEBUG("alt: ");DEBUGln(GPS.altitude);
			DEBUG("sat: ");DEBUGln(GPS.satellites);
			needGPS = 0;
			
		//}
	}
}