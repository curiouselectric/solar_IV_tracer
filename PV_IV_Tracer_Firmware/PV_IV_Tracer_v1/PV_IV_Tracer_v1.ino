/*

 Photovoltaic Current-Voltage Tracer (PV I-V Tracer)
 
 This is th code to produce a solar PV current-voltage curve tracer.
 This is based upon the Pedalog v2 PCB (from www.re-innovation.co.uk). This is based upon the arduino 
 but with current and voltage monitoring along with dual MOSFET drivers.
 
 It displays data using an iTEAD TFT LCD screen
 (Example code: http://www.re-innovation.co.uk/web12/index.php/en/blog-75/269-tft-lcd-display)
 
 Voltage is read using a potential divider.
 
 Current is read using an AD211 high side current monitor with a shunt resistor.
 
 The load are two MOSFETs, which are short circuited and controlled via PWM.

 The connections are as follows:
 Arduino:
 D0        Serial 
 D1        Serial
 D2        
 D3        TFT  RESET
 D4        
 D5        TFT  RS
 D6        TFT  Chip Select (CS)  
 D7
 D8        Buzzer output
 D9        MOSFET DUMP 1  (PWM)
 D10       MOSFET DUMP 2  (PWM)  
 D11       TFT  SDA  Serial Data
 D12  
 D13       TFT  SCLK  Serial Clock
 
 A0          Voltage input (via potential divider)
 A1          Current input (via AD211 high side monitor and current shunt resistor)
 A2
 A3
 A4
 A5
 
 Created on 9/10/13 
 by Matt Little
 matt@re-innovation.co.uk
 www.re-innovation.co.uk
 This code is open source and public domain.
 
 Modified:
 9/10/13  Matt Little  Initial code - prove LCD and FET work
 
 
 */

#include <TFT.h>  // Arduino LCD library
#include <SPI.h>
 
// pin definition for the TFT LCD
#define cs   6
#define dc   5
#define rst  3  
#define analogCurrent A1  // I read in on A1
#define analogVoltage A0  // V read in on A0
#define fan 4     // Fan control transistor is on pin 4
#define buzzer 8  // Buzzer attached to D8
#define dump1 9   // Analog output pin that the DUMP LOAD 1 is attached to via FET (PWM)
#define dump2 10  // Analog output pin that the DUMP LOAD 2 is attached to via FET (PWM)

TFT TFTscreen = TFT(cs, dc, rst);

// position of the line on screen
int xPos = 0;

int i = 0;        // An int to ramp up the FET PWM
int counter = 0;  //  A counter to slow down the ramp up of the FET PWM

void setup(){
 
  // initialize the serial port
  Serial.begin(9600);

  // initialize the display
  TFTscreen.begin();

  // clear the screen with a pretty color
  TFTscreen.background(250,16,200);
 
  // Set the dump laods as outputs and OFF
  pinMode(dump1, OUTPUT);
  pinMode(dump2, OUTPUT);  
  digitalWrite(dump1,LOW);
  digitalWrite(dump2,LOW); 
  
  pinMode(fan, OUTPUT);
  digitalWrite(fan,HIGH); 
  pinMode(buzzer, OUTPUT); 
  digitalWrite(buzzer,LOW);
  
}

void loop(){
  // read the sensor and map it to the screen height
  int sensor = analogRead(A1);
  int drawHeight = map(sensor,0,1023,0,TFTscreen.height());
  
  // print out the height to the serial monitor
  Serial.println(drawHeight);
  
  // draw a line in a nice color
  TFTscreen.stroke(250,180,10);
  TFTscreen.line(xPos, TFTscreen.height()-drawHeight, xPos, TFTscreen.height());

  // if the graph has reached the screen edge
  // erase the screen and start again
  if (xPos >= 160) {
    xPos = 0;
    TFTscreen.background(250,16,200); 
  } 
  else {
    // increment the horizontal position:
    xPos++;
  }

  delay(16);
  counter++;
  
  if(i>=50)
  {
    i=0;
  }
  
  if(counter>50)
  {
    i++;    
    analogWrite(dump1, i);    // This outputs the correct PWM
    //analogWrite(dump2, i);    // This outputs the correct PWM
    counter = 0;  
  }
  
}

