/*

 Photovoltaic Current-Voltage Tracer (PV I-V Tracer)
 
 This is the code to produce a solar PV current-voltage curve tracer.
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
 D2        Switch 1
 D3        TFT  RESET
 D4        Switch 2
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
 9/10/13   Matt Little  Initial code - prove LCD and FET work
 28/10/13  Matt Little  Added code to draw a graph. 
 
 
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
#define switch1 2  // An input switch
#define switch2 4  // An input switch

TFT TFTscreen = TFT(cs, dc, rst);

// position of the line on screen
int xPos = 0;

int i = 0;        // An int to ramp up the FET PWM
int counter = 0;  //  A counter to slow down the ramp up of the FET PWM

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

boolean runTrace = LOW;  // This flag runs the IV curve trace

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
  
  pinMode(switch1,INPUT);
  pinMode(switch2,INPUT);  
  
}

void loop(){
  
  
  // ******** Read Buttons *****************************
  // Check if start button (button 1 ) pressed
  // We need to debounce this input
  // read the state of the switch into a local variable:
  int switchReading = digitalRead(switch1); 
  
  // If the switch changed, due to noise or pressing:
  if (switchReading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (switchReading != buttonState) {
      buttonState = switchReading;
      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        runTrace = HIGH;  // Set the runTrace flag high - it will be set low after finishing the trace
      }
    }
  }
  // Store the previous stae for next time
  lastButtonState = switchReading;
  
  
  //********** IV Curve Trace ****************
  // Only do this if the runTrace flag has been set high
  if(runTrace==HIGH)
  {
    
    
    while(counter<=255)
    {
    
    
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
  else
  { 
     // clear the screen with a pretty color
     TFTscreen.background(250,16,200);
     xPos = 0;
  }
  
}

