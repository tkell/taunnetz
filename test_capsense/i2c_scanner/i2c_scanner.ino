// --------------------------------------
// i2c_scanner
//
// Version 1
//    This program (or code that looks like it)
//    can be found in many places.
//    For example on the Arduino.cc forum.
//    The original author is not know.
// Version 2, Juni 2012, Using Arduino 1.0.1
//     Adapted to be as simple as possible by Arduino.cc user Krodal
// Version 3, Feb 26  2013
//    V3 by louarnold
// Version 4, March 3, 2013, Using Arduino 1.0.3
//    by Arduino.cc user Krodal.
//    Changes by louarnold removed.
//    Scanning addresses changed from 0...127 to 1...119,
//    according to the i2c scanner by Nick Gammon
//    http://www.gammon.com.au/forum/?id=10896
// 
//
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.
//


#include <Wire.h>


// Joe's code
int xres = 13;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 13

//define values for slip coding
byte escapeChar = 101;
byte delimiterChar = 100;

// I2C adresses
#define I2C_ADDR0 0x00

// some CY8C201xx registers
#define INPUT_PORT0 0x00
#define INPUT_PORT1 0x01
#define CS_ENABLE0 0x06
#define CS_ENABLE1 0x07
#define I2C_DEV_LOCK 0x79
#define I2C_ADDR_DM 0x7C
#define COMMAND_REG 0xA0

// Secret codes for locking/unlocking the I2C_DEV_LOCK register
byte I2CDL_KEY_UNLOCK[3] = {0x3C, 0xA5, 0x69};
byte I2CDL_KEY_LOCK[3] = {0x96, 0x5A, 0xC3};


void setup()
{
  Wire.begin();
  
  // Joe/s code
  // set pin modes
  pinMode(xres, OUTPUT);
  
  // chip #1: put into reset mode
  digitalWrite(xres, HIGH);
  delay(100);

    // let the chip #1 wake up again
    digitalWrite(xres, LOW);
    delay(200);

    // chip #1: switch to setup mode
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(COMMAND_REG);
    Wire.write(0x08);
    Wire.endTransmission();

    // chip #1: setup CS_ENABLE0 register
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(CS_ENABLE0);
    Wire.write(B00001111);
    Wire.endTransmission();

    // chip #1: setup CS_ENABLE1 register
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(CS_ENABLE1);
    Wire.write(B00001111);
    Wire.endTransmission();

    // chip #1: switch to normal mode
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(COMMAND_REG);
    Wire.write(0x07);
    Wire.endTransmission();
  

  Serial.begin(9600);
  Serial.println("\nI2C Scanner");
  

}


void loop()
{
  byte error, address;
  int nDevices;
  
  
  
  nDevices = 0;
  for(address = 0; address < 1; address++ ) 
  {
    Serial.println(address);
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(2000);           // wait 5 seconds for next scan
}
