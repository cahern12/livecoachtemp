// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	: 4/26/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :
// =======================================================================
// Change History   	  :  
//        4/26/2017 - Initial commit   
//		  4/28/2017 - Cleaned up code and compiled it on avr     
//    
// =======================================================================
#include "Arduino.h"

// stateFlag:
//  1 = idle, waiting for input, zm fully extended
//  2 = zm driving up
//  4 = zm not driving, zm not fully up or down
//  3 = zm driving down
//	5 = zm force down
//  6 = zm force up

// erCode: 
// 0 = no error
// 1 = system init at unknown height
// 2 = limit switch unexpected value during up
// 3 = limit switch unexpected value during down

#define ZMDIRVAR	        1

#define SWTOP               3       // limit switch at max height
#define SWBOT               2       // limit switch at full retract

#define DELAYSLOW	        100     // 10hz
#define DELAYFAST	        10		// 100hz
#define STATECHANGEDELAY	15      // .01 s
#define FORCEDURATION		20	    // .1 s
#define	INPUTHDR	        0x80
#define	OUTPUTHDR	        0xFD
#define ZMACK		        0xFA
#define ZMNACK		        0xFB
#define ZMINVALID	        0xF0

#define	ZMUP		        0x20
#define ZMDOWN		        0x30
#define ZMSTOP		        0x40
#define ZMSTAT		        0x50

#define ZMSTEPUP	        0x25
#define ZMSTEPDOWN	        0x35
#define ZMDISTUP	        0x21
#define ZMDISTDOWN	        0x31

#define	ZMSTATDOWN	        0xA0
#define	ZMSTATUP	        0xB0
#define	ZMSTATMID	        0xC0
#define	ZMSTATTOPSW	        0xD0
#define	ZMSTATBOTSW	        0xD8
#define	ZMSTATRUNUP         0xA8
#define	ZMSTATRUNDN	        0xB8
#define	ZMSTATERROR	        0xC8
#define ZMCUR               A3      // analog input of current
#define AVGELEMENTS	        10
#define ZMDIR               6       // DIR pin - motor direction
#define ZMPWR               5       // PWM pin activates power
#define ZMSLP               4       // sleep enable

// 500 = 1.75" = 3428
#define CURRENTPERFOOT	3658

class Zipper
{
 public:
    //Constructor and destructor
    Zipper();
    ~Zipper();

    //Initialize variables
    boolean stringComplete = false;  // whether the string is complete
    int stateFlag = 0;

    //Helper functions & methods
    bool init();
    void readSwitches();
    void resetStateClean();
    void sendResponseHeader();
    void sendResponseFooter();
    void processSerial();
    void doCurrentMeasure();
    void process_inputByte();
    void process_stateFlag();
 private:
    unsigned int stateDelay = 0, forceDuration = 0;
    int topState            = 0, 
        botState            = 0, 
        erCode              = 0,
        recvHeader          = 0,
        estHeight           = 0,
        curTarget           = 0;
    byte indx = 0;
    byte curAvgByte = 0;
    byte inputByte[16];
    unsigned int curNow     = 0, 
                curLast     = 0, 
                curIndex    = 0, 
                curAvg      = 0, 
                curRaw      = 0, 
                timeRunning = 0;
    unsigned long curSum, 
                  curAvgSum;
    unsigned int curAvgArray[AVGELEMENTS];
    boolean heightFlag = false;
    String inputString = "";

    //Helper functions & methods
    void zmUpState2();
    void sendAck();
    void sendNack();
    void zmOff();
    void zmDownState3();
};
