//M101
#pragma once

//OBD INTERFACE HEX VALUES FOR QUERIES
//Refer to https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_01

//USED TO CHECK FOR COMMAND AVAILABILITY
#define OBD_PIDS_A	"00"		//RETURNS 4B Bitmask
#define OBD_PIDS_B	"20"		//RETURNS 4B Bitmask
#define OBD_PIDS_C	"40"		//RETURNS 4B Bitmask

//A BLOCK
#define OBD_LOAD		  "04"	//RETURNS 1B percent (100/255)
#define OBD_COOLANT		"05"	//RETURNS 1B celsius +40
#define OBD_RPM			  "0C"	//RETURNS 2B 4*rpm ((256A+B)/4)
#define OBD_SPEED		  "0D"	//RETURNS 1B km/h
#define OBD_THROTTLE	"11"	//RETURNS 1B percent (100/255)

//B BLOCK
#define OBD_FUEL_LEVEL	"2F"	//RETURNS 1B percent (100/255)

//C BLOCK
#define OBD_OIL_TEMP	"5C"	//RETURNS 1B celsius +40
