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
 
 A0          Voltage input (via 680k/100k potential divider)
 A1          Current input (via AD8211 high side monitor and 0.05 ohm current shunt resistor)
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
 29/10/13  Matt Little  Convert data into REAL values
 29/10/13  Matt Little  Added Grid to output
 29/10/13  Matt Little  Added Imp, Vmp and highlight point
 
 
 
 To Do:
 Convert V and I readings into actual values - Use a Vref external?
 Include start-up screen
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
//int xPos = 0;

int TFTscreenX = 160;
int TFTscreenY = 128;

//int i = 0;        // An int to ramp up the FET PWM
//int counter = 0;  //  A counter to slow down the ramp up of the FET PWM

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
unsigned long Pmax = 0;  // This is used to calculate the MPP
int dataPmax = 0;  // This stores the EEPROM value where the MPP is stored

unsigned long Vdata1 = 0;  // Thes are used for drawing the IV curve
unsigned long Vdata2 = 0;
unsigned long Idata1 = 0;
unsigned long Idata2 = 0;

// Varibales for writing to EEPROM
int hiByte;      // These are used to store longer variables into EERPRPROM
int loByte;

// char array to print to the screen
char sensorPrintout[6];
// char array to print to the screen
char counterDisplay[6];

void setup(){
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
  
  // initialize the serial port
  Serial.begin(9600);

  // initialize the display
  TFTscreen.begin();
  // Initialise the LCD with an image and text:
  // clear the screen with a pretty color
  TFTscreen.background(255,255,255);
  TFTscreen.stroke(255,255,0);  // Yellow  
  TFTscreen.fill(255,255,0);  // Yellow
  TFTscreen.circle(50,30,20);
  TFTscreen.line(50,5,50,0);
  TFTscreen.line(25,30,25,0);
  TFTscreen.line(50,55,50,128);
  TFTscreen.line(50,30,50,0);
  TFTscreen.line(50,5,50,0);
  TFTscreen.line(50,5,50,0);
  TFTscreen.line(50,5,50,0);
  TFTscreen.line(50,5,50,0);
  TFTscreen.line(50,5,50,0);  
  
  
  TFTscreen.stroke(0,0,0);  // Black
  TFTscreen.setTextSize(2);
  TFTscreen.text("I-V Curve",20,60);  
  TFTscreen.text("Tracer v1.0",20,80);  
  TFTscreen.setTextSize(1);
  TFTscreen.stroke(0,0,0);  // Black
  TFTscreen.text("re-innovation.co.uk",20,100); 

  
  
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
       // ****** Sample Data *********
       // Reset the voltage and current readings, ready for the next sample
       voltageSensor = 0;
       currentSensor = 0;

       
       // Here we read in 10 samples of both V and I
       for(int i=0;i<9;i++)
       {
         //Here we take ten V and I samples
         voltageSensor += analogRead(analogVoltage);
         currentSensor += analogRead(analogCurrent);   
         delay(1);   // Have a small delay
       }
    
       // We then take the average of the 10 samples
       voltageSensor = voltageSensor/10;
       currentSensor = currentSensor/10;

       
       // Convert the values into REAL V and I values using conversion factors
       // A reading of 1023 = 5V
       // Voltage measured by a potential divider 680k and 100k
       // So voltage conversion is (((reading/1024) * 5)/100)*780
       // Lets read in 100's milli volts so multiply by 10
       // Voltage (100's mV) = (reading * 5 * 780 * 10) / (1024*100)
       // Voltage (100's mV) = (reading * 5 * 78) / (1024)    
       voltageSensor = (voltageSensor * 390) / 1024; //  This converts the reading into 100's mV reading
       
       // MAXIMUM CURRENT = 5A - unless Rshunt is changed
       // Current measured by voltage drop over a 0.05ohm sense resistor
       // This is multiplied by 20 
       // So ((reading/1024)*5/20)/0.05
       // / by 0.05 is same as mulitply by 20
       // Current = (reading/1024) * 5 *20)/20
       // Current = (reading*5*20)/20*1024  // This is in amps
       // Current = (reading*5*1000*20)/20*1024 // This is in milliamps

       currentSensor = (currentSensor*5000)/1024;
      
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
       
   
       // Blank the previous data point with a rectangle
       // set the stroke color to white
       TFTscreen.stroke(255,255,255);
       // set the fill color to white
       TFTscreen.fill(255,255,255);
       TFTscreen.rect(19, 69, 100, 89);
       
       // convert the reading to a char array
       String(dataPointCounter/4).toCharArray(counterDisplay, 4);
       // set the font size
       TFTscreen.setTextSize(1);      
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
      
      // First reset the variables we are going to use:
      Pmax = 0;
      dataPmax = 0;  
      
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
          Serial.print("Voc is: ");
          Serial.println(Voc);
        }
        if(displayCounter == 159)
        {
          Isc = currentSensor;
          Serial.print("Isc is: ");
          Serial.println(Isc);
        }
        // Find the maximum power points
        // Do this by multiplying V and I and seeing how big that number is
        if(voltageSensor*currentSensor>Pmax)
        {
          Pmax = voltageSensor*currentSensor;
          Imp = currentSensor;
          Vmp = voltageSensor;
          dataPmax = displayCounter;  // Store the data point for highlighing within the display
        }  
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
     
     // One way to do it:
     // We need to scale the data so that max X fits with max V
     // We need to scale the data so that max Y fits with max I
     // Voc = max V, Isc = max I
     // So scaling factors are:
     // For Voltage:  data x (size of screen Y / Voc)
     // For Current:  data x ( size of sceen X / Isc)
    
      // Or we could just add a grid and always keep the same sacle - this will allow easy comparison
      // Grid of 1A steps up the side and 5V steps along the bottom
      // Here we add the grid:
      // Draw the lines in light grey
      TFTscreen.stroke(210,210,210);
      // Draw the voltage lines
      for(int i = 0; i<=5; i++)
      {
        // We will draw 5 current lines, each = 1A, each 20px apart
        TFTscreen.line((i*30),0,(i*30),TFTscreenY);     
      }
      // Draw the current lines
      for(int i = 0; i<=5; i++)
      {
        // We will draw 5 voltage lines, each = 5V, each 30px apart
        TFTscreen.line(0,(TFTscreenY-(i*20)),TFTscreenX,(TFTscreenY-(i*20)));     
      }     
         
    
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
      
      
//        // This is the OLD Method
//        // Convert those data points to fit within the screen
//        // We use a 20% factor to ensure that the data fits onto the screen
//        Vdata1 = (Vdata1*TFTscreenY)/(Voc*1.2);
//        Vdata2 = (Vdata2*TFTscreenY)/(Voc*1.2);
//        Idata1 = TFTscreenX - (Idata1*TFTscreenX )/(Isc*1.2);
//        Idata2 = TFTscreenX  - (Idata2*TFTscreenX)/(Isc*1.2);        
        
        // We actually want to scale so that:
        // 25V = 150px 
        // 5A = 100px 
        Vdata1 = (Vdata1*150)/(250);  // Reading is in 100's of mV
        Vdata2 = (Vdata2*150)/(250);
        Idata1 = TFTscreenY - (Idata1*1)/(50);  // Reading is in mA.
        Idata2 = TFTscreenY - (Idata2*1)/(50);  

        // Draw the lines in RED
        TFTscreen.stroke(255,0,0);
        TFTscreen.line(Vdata1,Idata1,Vdata2,Idata2);       
        // Draw the data points in BLUE
        TFTscreen.stroke(0,0,255);
        TFTscreen.point(Vdata1,Idata1); 
         
        // Also want to mark the MPP with a bigger dot?
        if(displayCounter==dataPmax)
        {
          // Draw a small circle in green
          TFTscreen.stroke(0,255,0);
          TFTscreen.fill(0,255,0);
          TFTscreen.circle(Vdata1,Idata1,2);         
        }
      } 
      programState = 5;
      Serial.println("State = 5");

    break;
    case 5:
       // Add on the important text data to the graph:
       
       // Blank the grid behind it
       // set the stroke color to grey
       TFTscreen.stroke(100,100,100);
       // set the fill color to white
       TFTscreen.fill(255,255,255);
       TFTscreen.rect(0, 0, TFTscreenX, 28);
       
       // set the stroke color to black
       TFTscreen.stroke(0,0,0);
       
       String(Isc).toCharArray(sensorPrintout, 5);       // convert the reading to a char array
       // Write the Isc
       TFTscreen.text("Isc:",20,2);
       TFTscreen.text(sensorPrintout,50,2);  
       
       String(Voc).toCharArray(sensorPrintout, 5);       // convert the reading to a char array
       // Write the Voc
       TFTscreen.text("Voc:",80,2);
       TFTscreen.text(sensorPrintout,110,2);
       
       String(Imp).toCharArray(sensorPrintout, 5);       // convert the reading to a char array
       // Write the Imp
       TFTscreen.text("Imp:",20,16);
       TFTscreen.text(sensorPrintout,50,16); 
     
       String(Vmp).toCharArray(sensorPrintout, 5);       // convert the reading to a char array
       // Write the Vmp
       TFTscreen.text("Vmp:",80,16);
       TFTscreen.text(sensorPrintout,110,16); 

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

