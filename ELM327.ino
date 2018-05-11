/*
 * ELM 327 GUIDE by Marcvs101
 * 
 * Commands are issued and recieved as strings
 * Upon powerup the ELM 327 will send a string containing ELM327 v[VERSION]
 * Use this string to check if the connection is ok
 * If no string is set, use AT Z command to reset (Z) the ELM327
 * Commands intended for the ELM start with AT
 * Commands intended for the vehicle only have hexadecimal characters 0-9 A-F
 * All commands must be terminated by a carriage return (0D)
 * Syntax errors will be signaled by a '?'
 * When the ELM is ready to recieve a command, it will send '>'
 * If only a carriage return is sent, the ELM will repeat the last command
 * OBD requests are split into MODE (1 byte) and PID (1-2 bytes)
 * Use PID 00 to show supported PIDs for that mode
 * Mode 01 PID 00 is supported by all
 * 
 * MODE 01 OPERATION
 * 41 means response from mode 01
 * Second byte usually repeats requested PID
 * Next bytes represent data
 * For a 00 PID, a four byte bitmask is returned
 * 
 */

//Defines to operate the ELM interface
#include "OBD_defines.h"
#include "ELM327_defines.h"

//Custom defines
#define RPM_MAX 3500
#define RPM_MIN 1500

//Import LCD
#include <liquidcrystal_I2C.h>
#include <Wire.h>
//Initialize the library with the numbers of the interface pins
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  //Set the LCD I2C address

//Process Variables
int state = 0;
int delayTime = 400;

//Setup function
void setup() {
  String tmpString = "";
  //Set up the LCD's number of columns and rows: 
  lcd.begin(16,2);
  //Display Welcome Message
  lcd.setCursor(0,0);
  lcd.print("Welcome");
  lcd.setCursor(0,1);
  lcd.print("Connecting <-x->");

  Serial.begin(9600);   //initialize Serial (Bluetooth)

  //Reset ELM Device
  tmpString = ELMQuery(ELM_RESET,5000);
  if (tmpString.substring(1,7)=="ELM327") {
    lcd.setCursor(0,0);
    lcd.print("Connection");
    lcd.setCursor(0,1);
    lcd.print("Successful!");
  } else {
    lcd.setCursor(0,0);
    lcd.print("ERROR!");
    lcd.setCursor(0,1);
    lcd.print("CHECK CONNECTION!");
    while (1) {delay(10000);}  //Break control flow
  }

  //Enable fast mode for ELM
  tmpString = ELMQuery(ELM_FAST_MODE,2000);
  
  delay(3000);
  lcd.clear();
}

//AUX Functions

//Standard query, get packet string
String ELMQuery(String query,int timeout) {
  String tmpRXString="";
  byte tmpByte = 0;
  bool waiting = true;

  unsigned long startTime = millis();
  
  //TX
  Serial.println("01"+query);

  //RX
  while(waiting){
    if (millis()-startTime>timeout){return -1;}
    else if{Serial.available() > 0}{waiting = false;}
    delay(10);
  }
  
  while(Serial.available() > 0) {
    tmpByte = 0;
    tmpByte = Serial.read();
    tmpRXString = tmpRXString + char(tmpByte);
  }
  
  tmpRXString.replace("01"+query,"");
  tmpRXString.replace(" ","");
  tmpRXString.replace("OK","");
  
  //Errors on the bus mapped to empty string
  tmpRXString.replace("STOPPED","");
  tmpRXString.replace("SEARCHING","");
  tmpRXString.replace("NO DATA","");
  tmpRXString.replace("?","");
  tmpRXString.replace(",","");
  
  return tmpRXString;
}

//Extract payload as a long int
long ELMMEssagePayload(String message, String query) {
  String tmpString = "";
  int cmdlen = 2;
  switch(query){
    case OBD_RPM:
      cmdlen = 4;
    break;
    case OBD_PIDS_A:
    case OBD_PIDS_B:
    case OBD_PIDS_C:
    case OBD_PIDS_D:
      cmdlen = 8;
    break;
  }
  int dataEND = 6 + cmdlen;
  tmpString = message.substring(4,dataEND);
  return HandleELMMessage(strtol(tmpString,NULL,16),query);
}

//Value formatting according to OBD formulas
long HandleELMMessage(long payload, String query){
  long res = 0;
  switch(query){
    case OBD_SPEED:
      res = payload
    break;
    case OBD_RPM:
      res = payload/4;
    break;
    case OBD_FUEL_LEVEL:
    case OBD_LOAD:
      res = payload*100/255;
    break;
    case OBD_COOLANT:
    case OBD_OIL_TEMP:
      res = payload-40;
    break;
  }
  return res;
}

//String formatting for the lazy
String StringifyELMMEssage(long payload, String query){
  String res = "";
  switch(query){
    case OBD_SPEED:
      res = String(payload)+ " km/h";
    break;
    case OBD_FUEL_LEVEL:
    case OBD_LOAD:
      res = String(payload)+ "%";
    break;
    case RPM:
    case OBD_COOLANT:
    case OBD_OIL_TEMP:
      res = String(payload);
    break;
  }
  return res;
}

//Loop function
void loop() {
  unsigned long start = millis();
  unsigned long finish = start;
  unsigned long waitTime = 0;
  String resultSTR = "";
  long resultNMBR = 0;
  //Query a subset of elements for each cycle
  //Got to go fast
  switch(state){
    
    //Print speed and advise if gear shift is necessary
    case 1:
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      if (resultNMBR < 10){
        lcd.noDisplay(); //For I2C use lcd.noBacklight
        break;
      } else {
        lcd.display();   //For I2C use lcd.backlight
      }
      resultSTR = StringifyELMMEssage(resultNMBR,2);
      lcd.setCursor(6,0);
      lcd.print(resultSTR+"  ");
      //
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_RPM,1000),OBD_RPM);
      lcd.setCursor(0,0);
      lcd.print("    ");
      if (resultNMBR > RPM_MAX){
        lcd.setCursor(0,0);
        lcd.print("*UP*");
      } else if (resultNMBR < RPM_MIN){
        lcd.setCursor(0,0);
        lcd.print("*DN*");
      } else {
        resultSTR = StringifyELMMEssage(resultNMBR,OBD_RPM);
        lcd.setCursor(0,1);
        lcd.print(resultSTR.substring(1,3));
      }
      state = 2;
    break;

    //Print speed and display fuel tank value on lower right
    case 2:
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      if (resultNMBR < 10){
        lcd.noDisplay();
        break;
      } else {
        lcd.display();
      }
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_SPEED);
      lcd.setCursor(6,0);
      lcd.print(resultSTR+"  ");
      //
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_FUEL_LEVEL,1000),OBD_FUEL_LEVEL);
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_FUEL_LEVEL);
      lcd.setCursor(7,0);
      lcd.print("        ");
      lcd.setCursor(9,1);
      lcd.print("FL "+resultSTR);
      state = 3;
    break;

    //Print speed and display oil temperarue on lower left
    case 3:
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      if (resultNMBR < 10){
        lcd.noDisplay();
        break;
      } else {
        lcd.display();
      }
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_SPEED);
      lcd.setCursor(6,0);
      lcd.print(resultSTR+"  ");
      //
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_OIL_TEMP,1000),OBD_OIL_TEMP);
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_OIL_TEMP);
      lcd.setCursor(0,1);
      lcd.print("        ");
      lcd.setCursor(0,1);
      lcd.print("OIL "+resultSTR);
      state = 4;
    break;

    //Print speed and advise if gear shift is necessary
    case 4:
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      if (resultNMBR < 10){
        lcd.noDisplay();
        break;
      } else {
        lcd.display();
      }
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_SPEED);
      lcd.setCursor(6,0);
      lcd.print(resultSTR+"  ");
      //
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_RPM,1000),OBD_RPM);
      lcd.setCursor(0,0);
      lcd.print("    ");
      if (resultNMBR > RPM_MAX){
        lcd.setCursor(0,0);
        lcd.print("*UP*");
      } else if (resultNMBR < RPM_MIN){
        lcd.setCursor(0,0);
        lcd.print("*DN*");
      } else {
        resultSTR = StringifyELMMEssage(resultNMBR,OBD_RPM);
        lcd.setCursor(0,1);
        lcd.print(resultSTR.substring(1,3));
      }
      state = 5;
    break;

    //Print speed and display high load warning
    case 5:
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      if (resultNMBR < 10){
        lcd.noDisplay();
        break;
      } else {
        lcd.display();
      }
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_SPEED);
      lcd.setCursor(6,0);
      lcd.print(resultSTR+"  ");
      //
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_LOAD,1000),OBD_LOAD);
      if (resultNMBR > 80){
        lcd.setCursor(0,1);
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print("*HIGH ENG LOAD!*");
      }
      state = 6;
    break;

    //Print speed and display coolant temperature on lower left
    case 6:
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      if (resultNMBR < 10){
        lcd.noDisplay();
        break;
      } else {
        lcd.display();
      }
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_SPEED);
      lcd.setCursor(6,0);
      lcd.print(resultSTR+"  ");
      //
      resultNMBR = ELMMessagePayload(ELMQuery(OBD_COOLANT,1000),OBD_COOLANT);
      resultSTR = StringifyELMMEssage(resultNMBR,OBD_COOLANT);
      lcd.setCursor(0,1);
      lcd.print("        ");
      lcd.setCursor(0,1);
      lcd.print("TMP "+resultSTR);
      state = 1;
    break;
  }

  //Keep constant minimum frequency
  finish = millis();
  waitTime = (finish-start);
  if (waitTime<delayTime){
    delay(delayTime-waitTime);
  }
}
