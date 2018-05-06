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
int ELMQuery(String query,int timeout) {
  String tmpRXString="";
  byte tmpByte = 0;

  unsigned long startTime = millis();
  
  //TX
  Serial.println(query);

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
  
  tmpRXString.replace(command,"");
  tmpRXString.replace(" ","");
  tmpRXString.replace("OK","");
  
  //Some of these look like errors that ought to be handled..
  tmpRXString.replace("STOPPED","");
  tmpRXString.replace("SEARCHING","");
  tmpRXString.replace("NO DATA","");
  tmpRXString.replace("?","");
  tmpRXString.replace(",","");
  
  return tmpRXString;
}

long ELMMEssagePayload(int expectedCommandLength) {
  if (expectedCommandLength < 0){
    String command = tmpRXString.substring(2,4);
    if (strcmp(command,OBD_RPM)) {expectedCommandLength = 2*2;}
    if (expectedCommandLength==0 && (strcmp(command,OBD_PIDS_A)||strcmp(command,OBD_PIDS_B)||strcmp(command,OBD_PIDS_C)||strcmp(command,OBD_PIDS_D))) {expectedCommandLength = 4*2;}
  }
  int dataEND = 6 + expectedCommandLength;
  tmpWorkString = tmpRXString.substring(4,dataEND);
  return strtol(tmpWorkString,NULL,16);
}

String HandleELMMEssage(String message, String query){
  long payload = ELMMessagePayload(message,strlen(query));
  String res = "";
  switch(query){
    case OBD_SPEED:
      res = String(payload)+ " km/h";
    break;
    case OBD_RPM:
      res = String(payload/4)+ " rpm";
    break;
    case OBD_FUEL_LEVEL:
    case OBD_LOAD:
      res = String(payload*100/255)+ " %";
    break;
    case OBD_COOLANT:
    case OBD_OIL_TEMP:
      res = String(payload-40)+ " C";
    break;
  }
  return res;
}

//Loop function
void loop() {
  unsigned long start = millis();
  unsigned long finish = start;
  unsigned long waitTime = 0;
  String result = "";
  //Query a subset of elements for each cycle
  //Got to go fast
  switch(state){
    case 1:
      result = HandleELMMEssage(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      lcd.setCursor(0,0);
      lcd.print(result+"  ");
      result = HandleELMMEssage(ELMQuery(OBD_RPM,1000),OBD_RPM);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("RPM: "+result+);
      state = 2;
    break;
    case 2:
      result = HandleELMMEssage(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      lcd.setCursor(0,0);
      lcd.print(result+"  ");
      result = HandleELMMEssage(ELMQuery(OBD_FUEL_LEVEL,1000),OBD_FUEL_LEVEL);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Fuel: "+result);
      state = 3;
    break;
    case 3:
      result = HandleELMMEssage(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      lcd.setCursor(0,0);
      lcd.print(result+"  ");
      result = HandleELMMEssage(ELMQuery(OBD_LOAD,1000),OBD_LOAD);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("ENG Load: "+result);
      state = 4;
    break;
    case 4:
      result = HandleELMMEssage(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      lcd.setCursor(0,0);
      lcd.print(result+"  ");
      result = HandleELMMEssage(ELMQuery(OBD_RPM,1000),OBD_RPM);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("RPM: "+result);
      state = 5;
    break;
    case 5:
      result = HandleELMMEssage(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      lcd.setCursor(0,0);
      lcd.print(result+"  ");
      result = HandleELMMEssage(ELMQuery(OBD_OIL_TEMP,1000),OBD_OIL_TEMP);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Oil: "+result);
      state = 6;
    break;
    case 6:
      result = HandleELMMEssage(ELMQuery(OBD_SPEED,1000),OBD_SPEED);
      lcd.setCursor(0,0);
      lcd.print(result+"  ");
      result = HandleELMMEssage(ELMQuery(OBD_COOLANT,1000),OBD_COOLANT);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Coolant: "+result);
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
