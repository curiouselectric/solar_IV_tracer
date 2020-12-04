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
 29/10/13  Matt Little  Draws IV curve
 
 
 To Do:
 Convert V and I readings into actual values
 Include start-up screen
 Sort out colours and graph display
 Have option of adding additional graphs to overwrite
 Serial output for display/storage
 Have a fill-bar to show data is being taken
 
 
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

int TFTscreenX = 160;
int TFTscreenY = 128;

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
unsigned long Voc = 0;  // These hold the important data points
unsigned long Isc = 0;
unsigned long Vmp = 0;
unsigned long Imp = 0;

unsigned long Vdata1 = 0;  // Thes are used for drawing the IV curve
unsigned long Vdata2 = 0;
unsigned long Idata1 = 0;
unsigned long Idata2 = 0;


// Varibales for writing to EEPROM
int hiByte;      // These are used to store longer variables into EERPRPROM
int loByte;

// char array to print to the screen
char sensorPrintout[4];
// char array to print to the screen
char counterDisplay[4];

void setup(){
 
  // initialize the serial port
  Serial.begin(9600);

  // initialize the display
  TFTscreen.begin();

  // clear the screen with a pretty color
  TFTscreen.background(255,255,255);
 
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
      
      //Blank the display
      TFTscreen.background(255,255,255);    // Clear the screen with white     
      // Then lets write what we are doing to the screen
      // set the font color to something
      TFTscreen.stroke(0,0,0);
      // set the font size
      TFTscreen.setTextSize(2);
      // write the text to the top left corner of the screen
      TFTscreen.text("Reading",20,20);
      TFTscreen.text("Values",20,50);  
      
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
         delay(1);   // Have a small delay
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
       
       // Let the user know something is happening
       // Have filled bar 0-100%

       // ********* TO DO ***********************
       
       // convert the reading to a char array
       String(dataPointCounter/4).toCharArray(counterDisplay, 4);
       // set the font size
       TFTscreen.setTextSize(1);       
       // Blank the previous data point with a rectangle
       // set the stroke color to white
       TFTscreen.stroke(255,255,255);
       // set the fill color to white
       TFTscreen.fill(255,255,255);
       TFTscreen.rect(19, 69, 100, 89);

       // Write the new number
       // set the stroke color to black
       TFTscreen.stroke(0,0,0);
       TFTscreen.text(counterDisplay,20,70);
       
       //*******Change the current point*************
       // At the moment this runs from 0 to 160 (0-160/255%)
       // This might need changing
       // Increase the PWM to the FET to increase the current
       analogWrite(dump1, dataPointCounter/4);    // This outputs the correct PWM
       delay(10);  // Wait for it to settle a bit
       
       // Check to see if we are done doing readings:
       if(dataPointCounter>=640)
       {
         analogWrite(dump1, 0);    // Set load to 0
         programState = 3;  // finished drawing a set of data
         Serial.println("State 3");
       }        
       break;     

    case 3:
      Serial.println("State 3");
      // ***********Analyse the data*************
      // We want to go through the data to find the max V and max I
      // We need the Voc, Isc, Vmp and Imp
      // To do this we need to run through the EEPROM stored values
      for(int displayCounter=0;displayCounter<160;displayCounter++)
      {
        hiByte = EEPROM.read(displayCounter*4);
        loByte = EEPROM.read((displayCounter*4)+1);
        voltageSensor = (hiByte << 8)+loByte;  
        hiByte = EEPROM.read((displayCounter*4)+2);
        loByte = EEPROM.read((displayCounter*4)+3);
        currentSensor = (hiByte << 8)+loByte;   
        // Voc will be when current = 0 (reading 0)
        // Isc will be when voltage = 0 (or near there) (reading 160)
        if(displayCounter == 0)
        {
          Voc = voltageSensor;
        }
        if(displayCounter == 159)
        {
          Isc = currentSensor;
        }
        // Find the maximum power points
        // Do this by multiplying V and I and seeing how big that number is
        // ******* TO DO****************
      }     
      
      
      programState = 4;  
      break;
      
      
    case 4:
      Serial.println("State 4");      
      // ********* DRAW the DATA ****************
     // Scaling of the data totally depends upon the analysis
     // Might want to keep the previous graph shown for scale?
     
     TFTscreen.background(255,255,255);    // Clear the screen with white
      
     // The display is 160 pixels wide and 64 pixels high
     // We build the graph by drawing a line from V/I point 1 to V/I point 2
     // These will have to be scaled to fit within the TFT screen
     
     // Our maximum X is 160
     // Our maximum Y is 128
     // We need to scale the data so that max X fits with max V
     // We need to scale the data so that max Y fits with max I
     
     // Voc = max V, Isc = max I
     
     // So scaling factors are:
     // For Voltage:  data x (size of screen Y / Voc)
     // For Current:  data x ( size of sceen X / Isc)
    
      // So we read out each part of the data and display it here
      // We only go to data point 158, as we read two points in each time.
      for(int displayCounter=0;displayCounter<=158;displayCounter++)
      {
         // Read in the Voltage points
        hiByte = EEPROM.read(displayCounter*4);
        loByte = EEPROM.read((displayCounter*4)+1);
        Vdata1 = (hiByte << 8)+loByte; 
        hiByte = EEPROM.read((displayCounter*4)+4);
        loByte = EEPROM.read((displayCounter*4)+5);
        Vdata2 = (hiByte << 8)+loByte;        
  
        hiByte = EEPROM.read((displayCounter*4)+2);
        loByte = EEPROM.read((displayCounter*4)+3);
        Idata1 = (hiByte << 8)+loByte;   
        hiByte = EEPROM.read((displayCounter*4)+6);
        loByte = EEPROM.read((displayCounter*4)+7);
        Idata2 = (hiByte << 8)+loByte; 
      
        // Convert those data points to fit within the screen
        // We use a 20% factor to ensure that the data fits onto the screen
        
        Vdata1 = (Vdata1*TFTscreenY)/(Voc*1.2);
        Vdata2 = (Vdata2*TFTscreenY)/(Voc*1.2);
        Idata1 = TFTscreenX - (Idata1*TFTscreenX )/(Isc*1.2);
        Idata2 = TFTscreenX  - (Idata2*TFTscreenX)/(Isc*1.2);        
                
        // Draw the lines in RED
        TFTscreen.stroke(255,0,0);
        TFTscreen.line(Vdata1,Idata1,Vdata2,Idata2);       
        // Draw the data points in BLUE
        TFTscreen.stroke(0,0,255);
        TFTscreen.point(Vdata1,Idata1);    
      }
      
      programState = 5;
      Serial.println("State = 5");

    break;
    case 5:
       // Add on the important text data to the graph:
       // set the stroke color to black
       TFTscreen.stroke(0,0,0);
       String(Voc).toCharArray(counterDisplay, 4);       // convert the reading to a char array
       // Write the Voc
       TFTscreen.text(counterDisplay,100,108);
       String(Isc).toCharArray(counterDisplay, 4);       // convert the reading to a char array
       // Write the Isc
       TFTscreen.text(counterDisplay,2,2);     
       

  
    
      programState = 0;
      Serial.println("State = 0");
    break;   
    
    default:
      // This is an error state - just in case problem with state machine
      Serial.println("Error - Should no be in this state");
    break;
      
  }
  delay(5);  // SLOW it down
}

