// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	  : 4/26/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :
// =======================================================================
// Change History   	  :  
//        4/26/2017 - Initial commit of the compass module & ino file      
//    
// =======================================================================
#include <Wire.h>
#include "Compass.h"

Compass compass;     //Declare Compass obj

void printHeadingData()
{
  Serial.println("Heading/Accel Data: ");
  Serial.print("Heading: ");
  Serial.println(compass.heading);
  Serial.println((float) compass.heading/10.0);
  
  Serial.print("Pitch: ");
  Serial.println(compass.pitch);
  Serial.println((float) compass.pitch/10.0);
  
  Serial.print("Roll: ");
  Serial.println(compass.roll);
  Serial.println((float)compass.roll/10.0);
}

void printAccelData()
{
  //Serial.println("Accelerometer Data (Raw value, in g forces):");
  Serial.print("X: ");
  Serial.println(compass.accelX);
  //Serial.println((float)compass.accelX/1024.0);

  Serial.print("Y: ");
  Serial.println(compass.accelY);
  //Serial.println((float)compass.accelY/1024.0);

  Serial.print("Z: ");
  Serial.println(compass.accelX);
  //Serial.println((float)compass.accelZ/1024.0);
}

void printTemp()
{
  Serial.print("Temp: ");
  Serial.println(compass.temperature);
}
void setup()
{
  Serial.begin(19200);
  delay(500);

  if(!compass.init())
    Serial.println("Sensor Initialization Failed\n");
}

void loop()
{
  compass.readHeading();
  printHeadingData();
  compass.readAccel();
  printAccelData();
  compass.readTilt();
  printTemp();
  Serial.println("-----------------------------------");
  delay(5000);
}
