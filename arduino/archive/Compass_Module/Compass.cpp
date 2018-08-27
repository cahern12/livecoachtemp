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
#include "Compass.h"

// Constructor - Creates sensor object, sets I2C address, and initializes sensor variables
Compass::Compass()
{
  _addr = HMC6343_I2C_ADDR;
  
  heading = pitch = roll = 0;
  magX = magY = magZ = 0;
  accelX = accelY = accelZ = 0;
  temperature = 0;
  
  clearRawData();
}

// Destructor - Deletes sensor object
Compass::~Compass()
{

}

// Initialize - returns true if successful
// Starts Wire/I2C Communication
// Verifies sensor is there by checking its I2C Address in its EEPROM
bool Compass::init()
{
  bool ret = true;
  uint8_t data = 0x00;

  // Start I2C
  Wire.begin();

  // Check for device by reading I2C address from EEPROM
  data = readEEPROM(SLAVE_ADDR);
  
  // Check if value read in EEPROM is the expected value for the HMC6343 I2C address
  if (!(data == 0x32))
    ret = false;                                // Init failed, EEPROM did not read expected I2C address value
    
  return ret;
}

// Send the HMC6343 a command to read the raw magnetometer values
// Store these values in the integers magX, magY, and magZ
void Compass::readMag()
{
  readGeneric(POST_MAG, &magX, &magY, &magZ);
}

// Send the HMC6343 a command to read the raw accelerometer values
// Store these values in the integers accelX, accelY, and accelZ
void Compass::readAccel()
{
  readGeneric(POST_ACCEL, &accelX, &accelY, &accelZ);
}

// Send the HMC6343 a command to read the raw calculated heading values
// Store these values in the integers heading, pitch, and roll
void Compass::readHeading()
{
  readGeneric(POST_HEADING, &heading, &pitch, &roll);
}

// Send the HMC6343 a command to read the raw calculated tilt values
// Store these values in the integers pitch, roll, and temperature
void Compass::readTilt()
{
  readGeneric(POST_TILT, &pitch, &roll, &temperature);
}

// Generic function which sends the HMC6343 a specified command
// It then collects 6 bytes of data via I2C and consolidates it into three integers
// whose addresses are passed to the function by the above read commands
void Compass::readGeneric(uint8_t command, int* first, int* second, int* third)
{
  sendCommand(command);                 // Send specified I2C command to HMC6343
  delay(1);                             // Delay response time
  
  clearRawData();                       // Clear object's rawData[] array before storing new values in the array
  
  // Wait for a 6 byte response via I2C and store them in rawData[] array
  Wire.beginTransmission(_addr);
  Wire.requestFrom(_addr,(uint8_t)6);   // Request the 6 bytes of data (MSB comes first)
  uint8_t i = 0;
  while(Wire.available() && i < 6)
  {
    rawData[i] = (uint8_t)Wire.read();
    i++;
  }
  Wire.endTransmission();
  
  // Convert 6 bytes received into 3 integers
  *first        =       rawData[0] << 8;             // MSB
  *first       |=       rawData[1];                  // LSB
  *second       =       rawData[2] << 8;
  *second      |=       rawData[3];
  *third        =       rawData[4] << 8;
  *third       |=       rawData[5];
}

// Send specified I2C command to HMC6343
void Compass::sendCommand(uint8_t command)
{
  Wire.beginTransmission(_addr);
  Wire.write(command);
  Wire.endTransmission();
}

// Send enter standby mode I2C command to HMC6343
void Compass::enterStandby()
{
  sendCommand(ENTER_STANDBY);
}

// Send exit standby (enter run) mode I2C command to HMC6343
void Compass::exitStandby()
{
  sendCommand(ENTER_RUN);
}

// Send enter sleep mode I2C command to HMC6343
void Compass::enterSleep()
{
  sendCommand(ENTER_SLEEP);
}

// Send exit sleep mode I2C command to HMC6343
void Compass::exitSleep()
{
  sendCommand(EXIT_SLEEP);
}

// Send enter calibration mode I2C command to HMC6343
void Compass::enterCalMode()
{
  sendCommand(ENTER_CAL);
}

// Send exit calibration mode I2C command to HMC6343
void Compass::exitCalMode()
{
  sendCommand(EXIT_CAL);
}

// Set the physical orientation of the HMC6343 IC to either LEVEL, SIDEWAYS, or FLATFRONT
// This allows the IC to calculate a proper heading, pitch, and roll in tenths of degrees
// LEVEL      X = forward, +Z = up (default)
// SIDEWAYS   X = forward, +Y = up
// FLATFRONT  Z = forward, -X = up
void Compass::setOrientation(uint8_t orientation)
{
  if (orientation == LEVEL)
    sendCommand(ORIENT_LEVEL);
  else if (orientation == SIDEWAYS)
    sendCommand(ORIENT_SIDEWAYS);
  else if (orientation == FLATFRONT)
    sendCommand(ORIENT_FLATFRONT);
}

// Send the I2C command to reset the processor on the HMC6343
void Compass::reset()
{
  sendCommand(RESET);
}

// Send the I2C command to read the OPMode1 status register of the HMC6343
// The register informs you of current calculation status, filter status, modes enabled and what orientation
// Refer to the HMC6343 datasheet for bit specifics
uint8_t Compass::readOPMode1()
{
  uint8_t opmode1 = 0x00;

  sendCommand(POST_OPMODE1);
  delay(1);
  
  Wire.beginTransmission(_addr);
  Wire.requestFrom(_addr,(uint8_t)1);
  opmode1 = (uint8_t)Wire.read();
  Wire.endTransmission();
  
  return opmode1;
}

// Send a command to the HMC6343 to read a specified register of the EEPROM
uint8_t Compass::readEEPROM(uint8_t reg)
{
  uint8_t data = 0x00;
  
  Wire.beginTransmission(_addr);
  Wire.write(READ_EEPROM);
  Wire.write(reg);
  Wire.endTransmission();
  
  delay(10);
  
  Wire.beginTransmission(_addr);
  Wire.requestFrom(_addr,(uint8_t)1);
  data = (uint8_t)Wire.read();
  Wire.endTransmission();
  
  return data;
}

// Send a command to the HMC6343 to write a specified register of the EEPROM
// This is not being used :)
void Compass::writeEEPROM(uint8_t reg, uint8_t data)
{
  Wire.beginTransmission(_addr);
  Wire.write(WRITE_EEPROM);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

// Clears the sensor object's rawData[] array, used before taking new measurements
void Compass::clearRawData()
{
  for (uint8_t i = 0; i < 6; i++)
    rawData[i] = 0;
}
