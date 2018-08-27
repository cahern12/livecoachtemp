#include <IridiumSBD.h>
#include <LowPower.h>

#define SOFT_SERIAL
#ifdef SOFT_SERIAL
#include <SoftwareSerial.h>

SoftwareSerial debugport(8, 9);  // Degub port = serial port on 8/9
#endif
HardwareSerial &nss(Serial);
IridiumSBD isbd(nss, 5);    // RockBLOCK SLEEP pin on 5

#define HEARTPIN   3    //  Raspberry PI heartbeat
#define PIPOWERPIN 4    //  Mirror SMUXPIN
#define LEDPIN    13    //  Flashes when PI is getting messages
#define SMUXPIN   7    //  SMUXPIN = 0 when Arduino looking for messages, 1 when messages routed to Raspberry PI
#define PIPIN     6    // PIPIN is HIGH only when Raspberry PI going down until power to PI is cut off from SMUX
#define BUTLOWPIN A2   // LOW driver for button
#define BUTTONPIN A3   // Input pin for button input_pullup
#define MAXBUFFER 200

bool BUTTONPUSHED = false;

void sleep_delay(int timer)
{
  int count = timer / 8000;
#ifdef SOFT_SERIAL
    debugport.print("ARDUINO PRO v3 - sleep delay count: ");
    debugport.println(count);
#endif
  if ( (count < 2) || (count > 200) )   // BAD DATA, too short or too long
  {
    count = 52;    // set to seven minutes
#ifdef SOFT_SERIAL
    debugport.print("ARDUINO PRO v3 - sleep delay count reset to: ");
    debugport.println(count);
#endif
  }
  for (int i = 0; i < count; i++)
  {
      LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, 
                SPI_OFF, USART0_OFF, TWI_OFF);
  }
  // NON-LOW POWER Option is to use delay(timer); instead of sleep_delay(timer);
}

void setup()
{
  int signalQuality = -1;

  // Initialize Arduino INPUT and OUTPUT PINS
  pinMode(SMUXPIN, OUTPUT);
  digitalWrite(SMUXPIN, LOW);
  pinMode(PIPOWERPIN, OUTPUT);
  digitalWrite(PIPOWERPIN, LOW);
  pinMode(PIPIN, INPUT);
  pinMode(HEARTPIN, INPUT);
  pinMode(BUTTONPIN, INPUT_PULLUP);
  pinMode(BUTLOWPIN, OUTPUT);
  digitalWrite(BUTLOWPIN, LOW);

  nss.begin(19200);
#ifdef SOFT_SERIAL
  debugport.begin(9600);
  debugport.println("ARDUINO PRO v3 - Starting setup");
  debugport.println("ARDUINO PRO v3 - Push button in next 10 seconds to automatically boot system");
  isbd.attachConsole(debugport);
  isbd.attachDiags(debugport);
#endif

  isbd.begin();
  isbd.setPowerProfile(1);
  
  delay(4000);
  int buttoncount = 0;
  int buttontimer = 6000;
  while (buttontimer > 0)
  {
    if (digitalRead(BUTTONPIN) == LOW)
    {
      buttoncount++;
    }
    buttontimer = buttontimer - 50;
    delay(50);
  }
  if (buttoncount > 5)
  {
    BUTTONPUSHED = true;
#ifdef SOFT_SERIAL
    debugport.println("ARDUINO PRO v3 - Button Pushed, bypassing iridium... ");
#endif
  }
  else   // NORMAL IRIDIUM CONTROL
  {
  int err = isbd.getSignalQuality(signalQuality);
  if (err != 0)
  {
#ifdef SOFT_SERIAL
    debugport.print("ARDUINO PRO v3 - SignalQuality failed: error ");
    debugport.println(err);
#endif
    return;
  }
  
#ifdef SOFT_SERIAL
  debugport.print("ARDUINO PRO v3 - Signal quality is ");
  debugport.println(signalQuality);
#endif

  }  // end else for button pushed
}

void loop()
{
  int err;
  char *command_string;
  uint8_t buffer[MAXBUFFER] = 
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
  size_t bufferSize = sizeof(buffer);

 if (BUTTONPUSHED == false)  // if button not pushed
 {
  err = isbd.sendReceiveSBDText(NULL, buffer, bufferSize);

  if (err != 0)
  {
#ifdef SOFT_SERIAL
    debugport.print("ARDUINO PRO v3 - sendReceiveSBDText failed: error ");
    debugport.println(err);
#endif
    return;
  }

#ifdef SOFT_SERIAL
  debugport.print("ARDUINO PRO v3 - Inbound buffer size is ");
  debugport.println(bufferSize);
  if (bufferSize > 0)
  {
    debugport.print("ARDUINO PRO v3 - Message received: ");
    for (int i = 0; i < bufferSize; i++)
    {
      debugport.write(buffer[i]);
    }
    debugport.println("  [END OF MESSAGE]");
  }
  debugport.print("ARDUINO PRO v3 - Messages left: ");
  debugport.println(isbd.getWaitingMessageCount());
  debugport.println("ARDUINO PRO v3 - Exiting message refresh");
#endif
 }   // if button not pushed end
 
  if ( (bufferSize > 0) && (buffer[0] == 'b') || BUTTONPUSHED)  // Got a boot command or button pushed
  {
#ifdef SOFT_SERIAL
    debugport.println("ARDUINO PRO v3 - MESSAGES:  Booting up Raspberry PI");
#endif  
    BUTTONPUSHED = false;
    digitalWrite(SMUXPIN, HIGH);
    digitalWrite(PIPOWERPIN, HIGH);
    digitalWrite(LEDPIN, HIGH);
    delay(30000);   //  Wait 30 seconds for the PI to boot up fully and time to do something
    int PiTimer = 100;
    int HeartTimer = 30000;
    while ( (PiTimer > 0) && (HeartTimer > 0) )
    {
      if (digitalRead(HEARTPIN) == HIGH)
      {
        HeartTimer = 30000;
#ifdef SOFT_SERIAL
       debugport.println("ARDUINO PRO v3 - HEARTBEAT Raspberry PI");
#endif
      }
      else
      {
        HeartTimer--;
      }
      if (digitalRead(PIPIN) == HIGH)
        {
          PiTimer--;
          digitalWrite(LEDPIN, LOW);  // LED FLASHES quickly when possibly shutting down PI
          delay(10);
          digitalWrite(LEDPIN, HIGH);
        }
        else
        {
          PiTimer = 100;
          delay(10);
        }
    }

    digitalWrite(SMUXPIN, LOW);
    digitalWrite(PIPOWERPIN, LOW);
    digitalWrite(LEDPIN, LOW);
    
    if (PiTimer > 0)   //  This means NO HEARTBEAT Received from Raspberry PI
    {
       err = isbd.sendReceiveSBDText("NO HEARTBEAT FOR OVER 5 MINUTES, SHUTDOWN", buffer, bufferSize);
#ifdef SOFT_SERIAL
       debugport.println("ARDUINO PRO v3 - NO HEARTBEAT from Raspberry PI, powering it off");
#endif     
    }
    
#ifdef SOFT_SERIAL
    debugport.println("ARDUINO PRO v3 - Back from Raspberry PI, wait for 7 minutes");
#endif
    isbd.sleep();            //  SLEEP the Rockblock
    sleep_delay(470000);     // Wait 7 minutes before rechecking for messages in Queue
    isbd.begin();            // Wake Rockblock
  }
  else
  {
#ifdef SOFT_SERIAL
    debugport.println("ARDUINO PRO v3 - No messages, wait for 7 minutes");
#endif
    isbd.sleep();            //  SLEEP the Rockblock
    sleep_delay(470000);     //  Wait 7 minutes and check again for messages in Queue
    isbd.begin();            // Wake Rockblock
#ifdef SOFT_SERIAL
    debugport.println("ARDUINO PRO v3 - Check for messages again");
#endif
  }
}

