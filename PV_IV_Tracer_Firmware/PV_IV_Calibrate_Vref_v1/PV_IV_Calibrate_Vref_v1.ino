/*
 PV IV Calibrate Vref 
 
 Overview:
 This code is for calibrating the internal reference in an ATmega328
 When using the internal reference there is a wide variation in the reference tolerance
 For 1.1V reference this can be from 1.1 to 1.3V
 For 2.56V reference this can be from 2.3 to 2.8V.
 
 A constant and accurate reference is applied to A0
 This code is designed to take in 100 samples of the analog input.
 This will give us an averaged reading (Vint).
 If we apply EXACTLY 1V during this process, then this figure can be used to calculate the voltage.
 
 eg: Read in Vint when 1V applied.
 Then store that as the calibration factor (CF)
 
 When we read in new data, if we divide by the CF then we get an output in volts.
 eg:
 CF = 500.
 Apply 2V then the reading will be 1000. V = 1000/500 = 2V
 Apply 0.5V then the reading will be 250. V = 250/500 = 0.5V
 
 This value is then stored in EEPROM, for use by other code.
 The EEPROM can only hold a byte (256) hence we must use 2 EEPROM locations
 We will store this number into EEPROM locations 1000 and 1001.

 
 The connections to the Arduino are as follows:
Arduino    Info
A0         Analog input 0
 
 See www.re-innovation.co.uk for more details
 
 17/10/13 by Matt Little
 Updated:
 30/10/13 Matt Little  Changed for PV IV curve tracer calibration 
 
 This example code is in the public domain.
 */

#include <stdlib.h>
#include <EEPROM.h>

// Analog sensing pin
int VsensePin = A0;    // Reads in the analogue number of voltage

unsigned long int Vint = 0; // Hold the Vint value
unsigned long int calibrationFactor = 0;    // This holds the Vref value in millivolts
float Vinput = 0;

// Varibales for writing to EEPROM
int hiByte;      // These are used to store longer variables into EERPRPROM
int loByte;

#define dump1 9   // Analog output pin that the DUMP LOAD 1 is attached to via FET (PWM)
#define dump2 10  // Analog output pin that the DUMP LOAD 2 is attached to via FET (PWM)


// the setup routine runs once when you press reset:
void setup()  { 
  // Set the dump laods as outputs and OFF
  pinMode(dump1, OUTPUT);
  pinMode(dump2, OUTPUT);  
  digitalWrite(dump1,LOW);
  digitalWrite(dump2,LOW); 
   
  // Start the serial output string - Only for ATTiny85 Version
  Serial.begin(9600);
  delay(100);
  Serial.println("Calibrate Device.....");
   
  analogReference(INTERNAL);  // This sets the internal ref to be 1.1V (or close to this);
  // NOTE: Must NOT have Vref (PIN 21) cnnected to anything to make this work.
  
  // Read in the Voltage Set-point
  hiByte = EEPROM.read(1000);
  loByte = EEPROM.read(1001);
  calibrationFactor = (hiByte << 8)+loByte;  // Get the sensor calibrate value 
  
  // Output the data (if ATTiny85)
  Serial.println("CF from EEPROM:");
  Serial.println(calibrationFactor);  
  
  delay(1000);  // Have a delay while the device settles
  
  // We need to read in 100 samples 

  for(int i=0;i<100;i++)
  {
      // Analogue read. We get 100 readings to average things
      Vint = Vint + analogRead(VsensePin);   // Read the analogue voltage
      delay(10);  // Short delay to slow things down
  } 
  
  // Average the value
  Vint = Vint /100;  
  
  // Calculate the value of Vref:
  
  calibrationFactor = Vint;
  
  delay(100);
  // Store the data to the EEPROM
  EEPROM.write(1000, calibrationFactor >> 8);    // Do this seperately
  EEPROM.write(1001, calibrationFactor & 0xff); 
  delay(100);
  
    // Output the data (if ATTiny85)
    Serial.println("Vint: ");
    Serial.println(Vint);  
  
    Serial.println("CF: ");
    Serial.println(calibrationFactor);  
 
} 

void loop()  { 
  
  delay(500);  // 0.5 Second delay
   // This loop reads in voltages and outputs them as a datastream
   Vint = analogRead(VsensePin);   // Read the analogue voltage
   
   Vinput = ((float)Vint/(float)calibrationFactor);
   
   Serial.print("Vinput= ");
   Serial.println(Vinput);
   
   // Re-read in the EEPROM - just to check it has saved OK
   hiByte = EEPROM.read(1000);
   loByte = EEPROM.read(1001);
   calibrationFactor = (hiByte << 8)+loByte;  // Get the sensor calibrate value 
   
   Serial.println("CF: ");
   Serial.println(calibrationFactor);  

  Serial.print("BYTES: ");   // TESTING
  Serial.print(hiByte);   // TESTING
  Serial.print("  ");   // TESTING
  Serial.println(loByte);   // TESTING   
  
   
}
