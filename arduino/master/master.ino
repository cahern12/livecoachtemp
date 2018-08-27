#include "GPS.h"
#include <SoftwareSerial.h>
//#include "IridiumSBD.h"
#include "LowPower.h"

// --------  Bit banging macros  ------------
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
//

//------- main board hardware defines --------
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

//more gps stuff.
#define GPS_RX				0
#define GPS_TX				1
SoftwareSerial gps_serial(GPS_RX, GPS_TX);
GPS gps(&gps_serial);
#define GPSECHO             false       // Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console. 
                                        //Set to 'true' if you want to debug and listen to the raw GPS sentences. 

#define tx1_serial          Serial      //Coming from/going to TX1
#define zip_serial          Serial1     //Coming from/Going to the Zippermast AVR

enum
{
  _STARTUP,      //SET UP MODE FOR BOOT UP
  _RUN,          //RUNNING MODE
  _TX1_SHUTDOWN  //MODE FOR TX1 SHUTDOWN
};

boolean using_gps_interrupt   = true;
boolean no_error              = false;
String  zip_avr_serial_data   = "";
String  tx1_serial_data       = "";
uint8_t current_state         = _STARTUP;

void setup()
{
    no_error = false;       //for the interrupts
    tx1_serial.begin(19200);
    zip_serial.begin(19200);
    setup_output_pins();
    init_gps();
    current_state = _RUN;
}
void loop()
{
    switch(current_state)
    {
        case _STARTUP:      //if it ever gets here like this then something went terribly wrong. 
            setup();
            break;
        case _RUN:
            no_error = true;
            send_gps_data();
            break;
    }
}

//---------------------------------------------------------------------------------
//-----------------------------INIT FUNCTIONS--------------------------------------
//---------------------------------------------------------------------------------
void setup_output_pins()
{
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
    PullHigh(RADIOPWR);		// turn on radio
    
	MakeOutput(CAMERAPWR);
    PullHigh(CAMERAPWR);	// turn on camera
    
	MakeOutput(TX1PWR);
	PullHigh(TX1PWR);		// turn on tx1
	
	MakeOutput(GPSPWR);		// gps is inverted logic
	PullLow(GPSPWR);		// gps on
}
void init_gps()
{
    gps.begin(9600);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    use_gps_interrupt(using_gps_interrupt);
    delay(1000);
}

//---------------------------------------------------------------------------------
//----------------------------HELPER FUNCTIONS-------------------------------------
//---------------------------------------------------------------------------------
void send_gps_data()
{
    tx1_serial.println(gps.get_time());
    tx1_serial.println(gps.get_date());
    tx1_serial.println(gps.get_location());
    tx1_serial.println(gps.get_speed());
    tx1_serial.println(gps.get_altitude());
    tx1_serial.println(gps.get_satellites());
}

//---------------------------------------------------------------------------------
//----------------------------------INTERRUPTS-------------------------------------
//---------------------------------------------------------------------------------
SIGNAL(TIMER0_COMPA_vect) 
{
  char c = gps.read();
    #ifdef UDR0
    if (GPSECHO)
        if (c) 
          UDR0 = c;  
    #endif
}

void use_gps_interrupt(boolean v) 
{
  if (v) 
  {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    using_gps_interrupt = true;
  } 
  else 
  {
    TIMSK0 &= ~_BV(OCIE0A);
    using_gps_interrupt = false;
  }
}

void serialEvent() //Receiving data from the tx1
{
    if(no_error)
    {
        tx1_serial_data = "";
        while(tx1_serial.available())
            tx1_serial_data += (char)tx1_serial.read();

        if(tx1_serial_data.equals("SHUTDOWN")) //Do shutdown stuff. 
        {
            //For now we do not want to do this until
            //we get the iridum stuff working.
            //IE we have no way to obtaining how to turn tx1 back on.
        }
        else if(tx1_serial_data.equals("ZMDOWN"))
        {
            zip_serial.write("ZMDOWN");
            //do we want to turn off the camera??
        }
        else if(tx1_serial_data)
        {
            zip_serial.write("ZMUP");
            //turn on the camera???
        }
    }
}

void serialEvent1() //Receive data (power string) from zippermast avr
{
    if(no_error)
    {
        zip_avr_serial_data = "";
        while(zip_serial.available())
            zip_avr_serial_data += (char)zip_serial.read();

        //This is where you write the incoming string to the power management modules
    }
}
