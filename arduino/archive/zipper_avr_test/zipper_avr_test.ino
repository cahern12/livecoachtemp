// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	  : 5/01/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :  Need to add in logic for Iridium (receive 
//                      message from TX1). 
// =======================================================================
// Change History   	:  
//        5/1/2017 - Initial commit of the hacked together ADAfruit GPS module  
//                  Also removed Interrupt and just read data every cycle    
//    
// =======================================================================

#include <SoftwareSerial.h>
#define LEDPIN              15
#define main_avr            Serial1

void setup()  
{
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, HIGH);
    Serial.begin(19200);
    main_avr.begin(19200);
    digitalWrite(LEDPIN, LOW);
}

void loop()                     
{  
    digitalWrite(LEDPIN, HIGH);
    delay(500);
    digitalWrite(LEDPIN, LOW);
    delay(500);
    main_avr.println("hello from the zipper avr");
}

void serialEvent1()
{
    char main_avr_serial_data[50] = "";
    int count = 0;
    while(main_avr.available() && count < 50)
    {
        main_avr_serial_data[count] = (char)main_avr.read();
        ++count;
    }
    Serial.println(main_avr_serial_data);
}