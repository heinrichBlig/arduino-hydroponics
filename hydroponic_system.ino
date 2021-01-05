#include <DallasTemperature.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include <OneWire.h>
#include <Wire.h>
#include <Sodaq_DS3231.h>
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
 
char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// constants won't change. Used here to set a pin number:
const int PumpPin =  3;// the number of the Pump pin 5
const int GrowLed = 4;   // GrowLED Relay set to digital pin 4
const int Fan = 6 ;  // Fan Relay set to digital pin 6 
const int lightSens  = A0; // select the input pin for LDR

// Variables will change:
int PumpState = LOW;             // PumpState used to set the Pump
int lightSensValue = 0;  // variable to store the value coming from the sensor


unsigned long previousMillis = 0;        // will store last time Pump was updated 
// constants won't change:
const long interval = 120000;           // interval at which to change Pump (milliseconds) 
//
// Hardware configuration
//
struct MyData {
  byte t;
};
MyData data;
// Set up nRF24L01 radio on SPI bus plus pins 8 & 7

RF24 radio(8,7);

// sets the role of this unit in hardware.  Connect to GND to be the 'led' board receiver
// Leave open to be the 'remote' transmitter
const int role_pin = A4;
//
// Topology
//

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes in this
// system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
//
// This is done through the role_pin
//

// The various roles supported by this sketch
typedef enum { role_remote = 1, role_led } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Hydro Main", "Hydro Screen"};

// The role of the current running sketch
role_e role;

void setup() {
  role = role_remote;
  Serial.begin(57600);
  Wire.begin();
  rtc.begin();
  pinMode(PumpPin, OUTPUT);
  pinMode(GrowLed, OUTPUT);
  pinMode(Fan, OUTPUT);
 sensors.begin();  // Start up the library
 
 printf_begin();
  printf("\n\Hydro System\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);

  //
  // Setup and configure rf radio
  //

  radio.begin();
  
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(108);
 
  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens a single pipes for these two nodes to communicate
  // back and forth.  One listens on it, the other talks to it.


    if ( role == role_remote )
  {
    radio.openWritingPipe(pipe);
  }
  else
  {
    radio.openReadingPipe(1,pipe);
  }
  radio.printDetails();
  

}


void loop() {
  
  
  watteringRelay();         // watering timer in the watteringRelay function
  tempController();       //Controle the temp with tempController function
  tempSend();
   timedRelay();
 
}
void tempSend(){


        // Send the command to get temperatures
 sensors.requestTemperatures();
 data.t = sensors.getTempCByIndex(0);

bool ok = radio.write( &data, sizeof(MyData) );
      Serial.println("Send ");
      Serial.println (data.t);
     if (ok)
        printf("ok\n\r");
      else
       printf("failed\n\r");
  
  
}
// here's where the magic happens for the watering of the plants:

void watteringRelay()
{
   // check to see if it's time to change the Pump; that is, if the difference
  // between the current time and last time you cgange the Pump is bigger than
  // the interval at which you want to change the Pump.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you change the Pump
    previousMillis = currentMillis;

    // if the Pump is off turn it on and vice-versa:
    if (PumpState == LOW) {
      PumpState = HIGH;
    } else {
      PumpState = LOW;
    }

    // set the Pump with the PumpState of the variable:
    digitalWrite(PumpPin, PumpState);
  }
  if(PumpState == 0){
      Serial.println("Pump On");
    }else {
      Serial.println("Pump off");
    }
  
}
// here's where the magic happens for the temperature:
void tempController()
{

  // Send the command to get temperatures
  sensors.requestTemperatures(); 

  //print the temperature in Celsius
  Serial.print("Temperature: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.print("C  |  ");
    
  if (sensors.getTempCByIndex(0) >= 25.00){
    digitalWrite(Fan, LOW);
    Serial.println("FAN On");
  }else if (sensors.getTempCByIndex(0) <= 20.00){
    digitalWrite(Fan, LOW);
    Serial.println("FAN On");
  }else {
    digitalWrite(Fan, HIGH);
    Serial.println(F("FAN Off"));
  }
  
    
}


// here's where the magic happens for the grow lights:

void timedRelay()
{
 
  DateTime now = rtc.now(); //get the current date-time
  Serial.print(now.hour());
  Serial.print(':');
  Serial.print(now.minute());
  Serial.print(':');
  Serial.print(now.second());

 
  switch (now.hour())
    {
     
      case 05: //when the clock reads 05 hours (Once a minute at the halfway mark)
               digitalWrite(GrowLed, LOW);   // turn the Relay on (LOW is the voltage level)  
             Serial.println("grow light on");
             
        
         break; 
 
 
 
        case 17: //when the clock reads 17 hours
            digitalWrite(GrowLed, HIGH);   // turn the Relay on (LOW is the voltage level)  
            Serial.println("grow light off");  

           break;
  }
  
           
       if (now.hour()<17 ){
                         lightSensValue = analogRead(lightSens); // read the value from the sensor
           Serial.println(lightSensValue);
              if( lightSensValue <= 700){ 
              digitalWrite(GrowLed, LOW);   // turn the Relay on (LOW is the voltage level)  
                         Serial.println("grow light on");
                         
                } else {
                 digitalWrite(GrowLed, HIGH);   // turn the Relay on (LOW is the voltage level)  
                         Serial.println("grow light off");
                           
              } 

   
          }else {
            digitalWrite(GrowLed, HIGH);   // turn the Relay on (LOW is the voltage level)  
            Serial.println("grow light off");  
          }

 
 
}
