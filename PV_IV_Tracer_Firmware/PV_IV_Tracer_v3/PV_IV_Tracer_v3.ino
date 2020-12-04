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
 29/10/13  Matt Little  Changed code structure to be a state machine
 
 
 */

#include <TFT.h>  // Arduino LCD library
#include <SPI.h>
#include <EEPROM.h>        // For writing values to the EEPROM
#include <avr/eeprom.h>    // For writing values to EEPROM
 
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


int dataPointCounter = 0;  // This counts through the 255 data points taken

int programState = 0;  // The program works like a state machine. This holds the state.

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int switchReading = LOW;
// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

unsigned long voltageSensor = 0;  // Holds the voltage data - used for an average, hence needs to be long
unsigned long currentSensor = 0;  // Holds the current data

// Varibales for writing to EEPROM
int hiByte;      // These are used to store longer variables into EERPRPROM
int loByte;

// char array to print to the screen
char sensorPrintout[4];
String diaplayPoint;

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
  
  // This whole program works as a state machine
  // The state is initialised to 0 at switch on
  //
  // See www.re-innovation.co.uk for the full state diagram
  
  switch(programState)
  {
    case 0:
      // ******** Read Buttons *****************************
      // Check if start button (button 1 ) pressed
      // We need to debounce this input
      // Read the state of the switch:
      switchReading = digitalRead(switch1); 
      // If the switch changed, due to noise or pressing:
      if (switchReading != lastButtonState) 
      {
        // reset the debouncing timer
        lastDebounceTime = millis();
      }
    
      if ((millis() - lastDebounceTime) > debounceDelay) 
      {
        // whatever the reading is at, it's been there for longer
        // than the debounce delay, so take it as the actual current state:
    
        // if the button state has changed:
        if (switchReading != buttonState) 
        {
          buttonState = switchReading;
          // only toggle the LED if the new button state is HIGH
          if (buttonState == HIGH) 
          {
            programState = 1;  // Move to the next state - start IV trace
            Serial.println("State 1");
          }
        }
      }
      // Store the previous state for next time
      lastButtonState = switchReading;        
      break;
    
    case 1:
          Serial.println("State 1");
      // ******* Start of the VI test routine ******
      // First we reset the data point counter
      dataPointCounter = 0;  
      
      // Then lets write what we are doing to the screen
      // set the font color to white
      TFTscreen.stroke(255,255,255);
      // set the font size
      TFTscreen.setTextSize(1);
      // write the text to the top left corner of the screen
      TFTscreen.text("Reading Values...",1,1);
      
      programState = 2;  // Move to the next state
      Serial.println("State 2");
      break;
      
      
    case 2:
          Serial.println("State 2");
       // ****** Sample Data *********
       // Here we read in 10 samples of both V and I
       for(int i=0;i<9;i++)
       {
         //Here we take ten V and I samples
         voltageSensor += analogRead(A0);
         currentSensor += analogRead(A1);   
         delay(10);   // Have a small delay
         Serial.println(voltageSensor);
         Serial.println(currentSensor);
       }
    
       // We then take the average of the 10 samples
       voltageSensor = voltageSensor/10;
       currentSensor = currentSensor/10;
       
        // We store it into the correct EEPROM space
        // On the ATmega328 there are 1024 EEPROM locations.
        // We are going to take 160 readings to fit the screen
        // Each reading is a long, so takes up 2 x EEPROM spaces
        // Each reading has a value of V and value of I
        // Hence we need 2 x 2 x 160 = 640 EEPROM locations
        
        // Write the voltage
        EEPROM.write(dataPointCounter, voltageSensor >> 8);    // Do this seperately
        EEPROM.write(dataPointCounter+1, voltageSensor & 0xff);   
        // Write the current  
        EEPROM.write(dataPointCounter+2, currentSensor >> 8);    // Do this seperately
        EEPROM.write(dataPointCounter+3, currentSensor & 0xff);             
            
       // Then increase to dataPointCounter, up to 255 samples
       dataPointCounter = dataPointCounter+4;
       
       // Display the data point we are reading
       // convert the reading to a char array
       dataPointCounter.toCharArray(displayPoint, 4);
      
       TFTscreen.text(displayPoint,20,20);
      
       if(dataPointCounter>=640)
       {
         programState = 3;  // finished drawing a set of data
         Serial.println("State 3");
       }        
       break;     

    case 3:
      Serial.println("State 3");
      // ***********Analyse the data*************
      // We want to go through the data to find the max V and max I
      
      programState = 4;  // finished drawing a set of data
      break;
      
      
    case 4:
      Serial.println("State 4");      
      // ********* DRAW the DATA ****************
     
     TFTscreen.background(250,16,200);    // Clear the screen
      
      // The display is 160 pixels wide
      xPos = 0;  // Reset the xPosition for redrawing the display
      
      // So we read out each part of the data and display it here
      for(int displayCounter=0;displayCounter<=160;displayCounter++)
      {
         // Read in the Voltage Set-point
        hiByte = EEPROM.read(displayCounter*4);
        loByte = EEPROM.read((displayCounter*4)+1);
        voltageSensor = (hiByte << 8)+loByte;  
        hiByte = EEPROM.read((displayCounter*4)+2);
        loByte = EEPROM.read((displayCounter*4)+3);
        currentSensor = (hiByte << 8)+loByte;   
  
         // TEsting  - here we draw it
        int drawHeight = map(voltageSensor,0,1023,0,TFTscreen.height());      
        // draw a line in a nice color
        TFTscreen.stroke(250,180,10);
        TFTscreen.line(xPos, TFTscreen.height()-drawHeight, xPos, TFTscreen.height());       
        xPos++;   
      }
      programState = 0;
      Serial.println("State = 0");

    break;
    
//    
//      // if the graph has reached the screen edge
//      // erase the screen and start again
//      if (xPos >= 160) 
//      {
//        xPos = 0;
//        programState = 0;  // Reset the program state

//      } 
//      else 
//      {
//        // increment the horizontal position:
//        xPos++;
//      }
//    
//      delay(16);
//      counter++;
//      
//      if(i>=50)
//      {
//        i=0;
//      }
//
//      if(counter>50)
//      {
//         
//        i++;    
//        analogWrite(dump1, i);    // This outputs the correct PWM
//        //analogWrite(dump2, i);    // This outputs the correct PWM
//        counter = 0;  
//      }
  }
  delay(5);  // SLOW it down
}

