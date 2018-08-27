// =======================================================================
// AUTHOR               : Colin Ahern
// CREATE DATE      	  : 5/01/2017
// COMPANY              : Zel Technologies
// PURPOSE              :
// SPECIAL NOTES        :  
//                    1) Need to add in logic for Iridium (receive message from TX1). 
//                    2) Include interrupt on Iridium pin to wake up for sleep state
//                    3) add sleep stuff
// =======================================================================
// Change History   	:  
//        5/1/2017 - Initial commit of the hacked together ADAfruit GPS module  
//                  Also removed Interrupt and just read data every cycle    
//    
// =======================================================================

#include "GPS.h"
#include <SoftwareSerial.h>
#include "IridiumSBD.h"
#include "LowPower.h"
#include "Compass.h"

#define LEDPIN              15
#define VALID_SATELLITES    4
#define GPS_RX              0
#define GPS_TX              1
#define IRID_RX             22
#define IRID_TX             23
#define IRID_SLEEP          3
#define GPS_ERR_DELAY       1
#define GPSECHO             false       // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console. 
                                        //Set to 'true' if you want to debug and listen to the raw GPS sentences. 
#define tx1_serial          Serial
#define zip_serial          Serial1
enum
{
  _STARTUP,       //SET UP MODE FOR BOOT UP
  _RUN,           //RUNNING MODE
  _GPS_ERROR,     //GPS ERROR MODE
  _IRID_ERROR,    //IRIDIUM ERROR
  _COMPASS_ERROR,  //COMPASS ERROR
  _AVR_SLEEP      //PUT THE AVR TO SLEEP
};

SoftwareSerial gps_serial(GPS_RX, GPS_TX);
SoftwareSerial nss(IRID_RX, IRID_TX);
IridiumSBD isbd(nss, IRID_SLEEP);
GPS gps(&gps_serial);
Compass compass; 

boolean usingInterrupt        = true;  // this keeps track of whether we're using the interrupt
boolean no_error              = true;
String  zip_avr_serial_data   = "";
String  tx1_serial_data       = "";
uint8_t current_state         = _STARTUP;

//---------------------------------------------------------------------------------
//--------------------------------MAIN PROGRAM-------------------------------------
//---------------------------------------------------------------------------------
void setup()  
{
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  tx1_serial.begin(19200);
  zip_serial.begin(19200);
  init_gps();
  init_iridium();
  no_error = true;
  if(!compass.init())
  {
    current_state = _COMPASS_ERROR;
  }
  else
    current_state = _RUN;
}

uint32_t timer = millis();
void loop()                     
{  
  switch(current_state)
  {
      case _STARTUP:
          setup();
          break;
      case _RUN:
          no_error = true;
          process_gps_compass_data();
          break;
      case _GPS_ERROR:
          tx1_serial.println("Invalid Data From GPS.. Going to sleep");
          no_error = false;
          if(gps.satellites >= VALID_SATELLITES)
          {
            current_state = _RUN;
            tx1_serial.println("GPS back up and running.");
            break;
          }
          delay(5000);
          tx1_serial.println("Woke back up");
          break;
      case _IRID_ERROR:
          no_error = false;
          break;
      case _COMPASS_ERROR:
          no_error = false;
          tx1_serial.println("Compass sensor Failed");
          break;
      case _AVR_SLEEP:
          break;
  }
}

//---------------------------------------------------------------------------------
//----------------------------HELPER FUNCTIONS-------------------------------------
//---------------------------------------------------------------------------------
void process_gps_compass_data()
{
  if (gps.newNMEAreceived())                // if a sentence is received, we can check the checksum, parse it...
  {
    if (!gps.parse(gps.lastNMEA()))         // this also sets the newNMEAreceived() flag to false
      return;                               // we can fail to parse a sentence in which case we should just wait for another
    
    if(gps.satellites < VALID_SATELLITES)   //change states if invalid
    {
      current_state = _GPS_ERROR;
      return;
    }
    print_gps_data();
    print_compass_data();                 //now that gps is complete - process compass data
    delay(1000);                            //delay for 1 seconds
  }
}
void print_compass_data()
{
  compass.readHeading();
  compass.readAccel();
  compass.readTilt();
  tx1_serial.println(compass.get_heading());
  tx1_serial.println(compass.get_pitch());
  tx1_serial.println(compass.get_roll());
  tx1_serial.println(compass.get_temp());
}
void print_gps_data()
{
  tx1_serial.println(gps.get_time());
  tx1_serial.println(gps.get_date());
  tx1_serial.println(gps.get_location());
  tx1_serial.println(gps.get_speed());
  tx1_serial.println(gps.get_altitude());
  tx1_serial.println(gps.get_satellites());
  zip_serial.print("Hello from the main avr");
}
//not being used yet
/*void sleep_delay(int minutes)
{
  tx1_serial.println("AVR Going To Sleep...");
  int counter = (minutes * 60000) / 8000;
  for(int x = 0; x < counter; x++)
  {
    LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
                SPI_OFF, USART0_OFF, TWI_OFF);
  }
  tx1_serial.println("Work Back Up...");
}*/
//---------------------------------------------------------------------------------
//-----------------------------INIT FUNCTIONS--------------------------------------
//---------------------------------------------------------------------------------
void init_gps()
{
  gps.begin(9600);
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  useInterrupt(usingInterrupt);
  delay(1000);
  digitalWrite(LEDPIN, LOW);
}

void init_iridium()
{
  /*nss.begin(19200);
  isbd.attachConsole(Serial);  //Remove this line when done
  isbd.attachDiags(Serial);    //Remove this line when done
  isbd.begin();                //Cant connect to satellites :(
  isbd.setPowerProfile(1);*/
}
//---------------------------------------------------------------------------------
//----------------------------------INTERRUPTS-------------------------------------
//---------------------------------------------------------------------------------
// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) 
{
  char c = gps.read();
    #ifdef UDR0
    if (GPSECHO)
        if (c) 
          UDR0 = c;  
    #endif
}

void useInterrupt(boolean v) 
{
  if (v) 
  {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else 
  {
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void serialEvent()     //Receiving data from TX1
{
  if(no_error)
  {
    tx1_serial_data = "";

    while(tx1_serial.available())
      tx1_serial_data += (char)tx1_serial.read();
    
    tx1_serial.println(tx1_serial_data);

    if(tx1_serial_data.equals("turn_off"))
    {
      tx1_serial.println("yay it worked. turn off now");
    }
    else if(tx1_serial_data.equals("turn_on"))
    {
      tx1_serial.println("yayyyyyy.. turn on now");
    }
  }
}

void serialEvent1()     //Receiving data from zipper mast avr
{
  if(no_error)
  {
    zip_avr_serial_data = "";

    while(zip_serial.available())
      zip_avr_serial_data += (char)zip_serial.read();
    
    tx1_serial.println(zip_avr_serial_data);
  }
}
