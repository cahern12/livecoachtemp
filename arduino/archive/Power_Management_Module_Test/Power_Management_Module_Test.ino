// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	: 5/01/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :
// =======================================================================
// Change History   	:  
//        5/1/2017 - Initial commit      
//    
// =======================================================================

#include <SoftwareSerial.h>
#define LEDPIN        15


SoftwareSerial pm_serial(8, 9);
int count;

void setup()  
{
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  Serial.begin(115200);
  pm_serial.begin(19200);
  delay(5000);
  digitalWrite(LEDPIN, LOW);
}

void loop()                     
{  
  pm_serial.listen();
  /*count = 0;
  while(pm_serial.available())
  {
    ++count;
    Serial.println(pm_serial.read());
  }
  if(count == 0)
    Serial.println("Waiting on RS232...");
  pm_serial.flush();*/
  
  if(pm_serial.available())
    Serial.println(pm_serial.readString());
  //else
    //Serial.println("Waiting on RS232...");


   //delay(2000);
}
