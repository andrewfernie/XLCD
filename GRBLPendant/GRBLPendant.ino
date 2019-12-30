//=========================================================
//Project: GRBL Pendant
//Module:  serialspy.ino        
//=========================================================
//
// Author: Andrew Fernie
// Source code freely released - do with it what you like!
//
//----------------------------------------------------------
// This code started from the XLCD project by Frank Herrmann
//----------------------------------------------------------

//
// ===============================================
// Includes
// ===============================================
//

#include <Keypad.h>
#include <Key.h>
#include <EEPROM.h>
#include <simpleThread.h>
#include <Encoder.h>


//
// ===============================================
// Defines
// ===============================================
//

// MODE ------------------------------------------
#define MODE_PROXY   // works as proxy between PC and GRBL
//#define MODE_SPY   // works only as spy on the TX line 
					 // (no buttons, no serial console, no interaction with grbl)

// Normal serial for debug ----------------------------------
#define PC_SERIAL       115200  

// Serial to GRBL
#define grblSerial      Serial2
#define GRBL_SERIAL     115200 

// G-Code sender. Must be as fast as grbl!
#define gsSerial        Serial3
#define GS_SERIAL       115200 

const int BUFFER_SIZE = 70;

// LCD -------------------------------------------
#define LCD_ADDR		   0x27  // I2C LCD Address
//#define LCD_4BIT

#define LCD_cols			20
#define LCD_rows			4

#if(LCD_cols==16)
#define LCD_EMPTY   F("                ")
#else
#define LCD_EMPTY   F("                    ")
#endif

#if defined(LCD_4BIT)
#define LCD_EN          12
#define LCD_RW          11
#define LCD_D4          4
#define LCD_D5          5
#define LCD_D6          6
#define LCD_D7          7
#elif defined(LCD_ADDR)
#define LCD_EN          2
#define LCD_RW          1
#define LCD_RS          0
#define LCD_D4          4
#define LCD_D5          5
#define LCD_D6          6
#define LCD_D7          7
#endif


#if defined(MODE_PROXY)
   // Rotary Encoder --------------------------------
#define ENC_A              2     // Encoder interrupt pin
#define ENC_B              3     // Encoder second pin
#define ENC_S              4     // Encoder select pin

// EEPROM addresses
#define EEPROM_BUTTONS		100
#define EEPROM_INTERVAL		150
#endif

// ========================== END Defines ========

// only for debugging
#define DEBUG

//#define DEBUG_IO

#ifdef DEBUG_IO
#define DEBUG_TXT(str) Serial.println(str);
#else
#define DEBUG_TXT(str); 
#endif

#define VERSION         0.1

// Makros
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// GRBL Serial connect pins ----------------------
#define GRBL_RX			8
#define GRBL_TX			9 

#define IDLE            1
#define QUEUE           2
#define RUN             3
#define HOLD            4
#define HOME            5
#define ALARM           6
#define CHECK           7


//
// ===============================================
// Inits
// ===============================================
 //


 // Add Threads to refresh status informations from GRBL
#define _sT_cnt  _sT_cnt_3    // count of threads(?, $G)
simpleThread_init(_sT_cnt);   // init threads
simpleThread_new_timebased_dynamic(_sT_P1, _sT_millis, 5000, _sT_start, getPositions);	// get position info (?)
simpleThread_new_timebased_dynamic(_sT_P2, _sT_millis, 5000, _sT_start, getStates);	// get state info ($G) (not supported from UniversalGcodeSender)
simpleThread_new_timebased_dynamic(_sT_P3, _sT_millis, 200, _sT_start, readButtons);	// get button value

// make a group
simpleThread_group_init(group_one, 2) {
	simpleThread_group(getPositions),
		simpleThread_group(getStates)
};


// All inits for LCD control
#if defined(LCD_4BIT)
#include <LiquidCrystal.h>   // LCD
LiquidCrystal myLCD(LCD_EN, LCD_RW, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#elif defined(LCD_ADDR)
#include <Wire.h>                // I2C Communication
#include <LiquidCrystal_I2C.h>   // LCD over I2C
// Set the pins on the I2C chip used for LCD connections:
//                         addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C myLCD(LCD_ADDR, LCD_EN, LCD_RW, LCD_RS, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 3, POSITIVE);  // Set the LCD I2C address
#endif


#ifdef ENC_A
	// RotaryEncoder
Encoder encoder(ENC_A, ENC_B);
long encoderPosition = -999;
bool encoderSwitch = false;
#endif

// LCD Menu
// look in lcd_menu.ino for build menus
// LCDMenu menu(&myLCD, LCD_cols, LCD_rows);

long lastStatusRXTime = 0;
long lastStateRXTime = 0;

// System Timers
// --------------
unsigned long 	fast_loopTimer;				// Time in miliseconds of main control loop
unsigned long 	fast_loopTimeStamp;			// Time Stamp when fast loop was complete
uint8_t 		delta_ms_fast_loop; 		// Delta Time in miliseconds
int 			mainLoop_count;

unsigned long 	medium_loopTimer;			// Time in miliseconds of medium loop
byte 			medium_loopCounter;			// Counters for branching from main control loop to slower loops
uint8_t			delta_ms_medium_loop;

unsigned long 	slow_loopTimer;			// Time in miliseconds of medium loop
byte 			slow_loopCounter;
uint8_t 		delta_ms_slow_loop; 		// Delta Time in miliseconds
byte 			superslow_loopCounter;
byte			counter_one_herz;

// Keypad Setup
// --------------
const byte rows = 4; //four rows
const byte cols = 4; //four columns
char keys[rows][cols] = {
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','0','A','B'},
  {'C','D','E','F'}
};
byte rowPins[rows] = { 14, 15, 16, 17 }; //connect to the row pinouts of the keypad
byte colPins[cols] = { 20, 21, 22, 23 }; //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

// CNC Status Data 
// ---------------

struct AxisData
{ 
	float x;
	float y;
	float z;
};
AxisData currentPosition;
char     positionCoordSystem[10];

float currentSpindleSpeed;

//
// ===============================================
// Main
// ===============================================
 //



 //  ---------- Setup  ----------- //

void setup()
{
	Serial.begin(PC_SERIAL);   // open serial to PC


#ifdef ENC_S
	// set Select pin from Rotary Encoder to input
	pinMode(ENC_S, INPUT);      // sets the encoder select digital pin
#endif

   // init threads //
	simpleThread_initSetup(_sT_cnt);
	//simpleThread_dynamic_setLoopTime(getPositions,	EEPROMReadInt(EEPROM_INTERVAL));
	//simpleThread_dynamic_setLoopTime(getStates,		EEPROMReadInt(EEPROM_INTERVAL));

	myLCD.begin(LCD_cols, LCD_rows);

	// This is the serial connect to PC, we get some commands
	// but we can also print some additional information about this module
	// and the parser from Client program will ignore this
	Serial.print(F("<GRBLPendant "));
	Serial.print(VERSION);
	Serial.println(F(">"));
	Serial.println(F("<All commands for XLCD start with a colon ':'>"));
	Serial.println(F("<Call help with ':?'>"));

	// Old LCD Screens
	myLCD.begin(LCD_cols, LCD_rows); // letter, row

	myLCD.setCursor(0, 0); // letter, row
	myLCD.print(F("XLCD "));
	myLCD.print(VERSION);
	myLCD.setCursor(0, 1); // letter, row
	myLCD.print(F("Connect ... "));

	delay(2000);

	// open serial port to G Code senser 
	gsSerial.begin(GS_SERIAL);

	// open serial port to GRBL
	grblSerial.begin(GRBL_SERIAL);
	// reset grbl device (ctrl-X) for Universal Gcode Sender
	grblSerial.write(0x18);


	myLCD.clear();
}//SETUP



//  ---------- Loop ----------- //
int pc = 0;
int gr = 0;
int pc_chars = 0;
int gr_chars = 0;
int loopCount = 0;
char pcserial[BUFFER_SIZE];
char grserial[BUFFER_SIZE];
char lcdRowString[LCD_cols];
uint32_t  lastLCDOut = 0;


void loop()
{
	// Jobs
	// simpleThread_run(_sT_priority);

	serial_io_grbl();
	serial_io_gs();

	// We want this to execute at 50Hz if possible
	// -------------------------------------------
	if (millis() - fast_loopTimer > 19) {
		delta_ms_fast_loop = millis() - fast_loopTimer;

		fast_loopTimer = millis();

		mainLoop_count++;

		// Execute the fast (nominal 50Hz) loop
		// ------------------------------------
		fast_loop();

		// Execute the medium loop (internally sub-banded by 5 to a nominal 10Hz)
		// -----------------------------------------------------------------------
		medium_loop();

		// Execute the slow (nominal 1Hz) loop
		// ------------------------------------
		counter_one_herz++;
		if (counter_one_herz >= 50)
		{
			one_second_loop();
			counter_one_herz = 0;
		}

		fast_loopTimeStamp = millis();
	}

}//LOOP


// ---------- Subroutines -----------

//
// split a string on one or more delimiter
// and return split at index
// split(char* string[] = "1,2;3")
//
char* split(char* string, char* delimiter, int index)
{
	char* ptr;

	char buffer[BUFFER_SIZE];
	strcpy(buffer, string);

	// init and create first cut
	ptr = strtok(buffer, delimiter);
	int x = 0;
	while (ptr != NULL) {
		if (x++ >= index)
			break;
		// next one
		ptr = strtok(NULL, delimiter);
	}

	return ptr;
}


// State set or get state from machine
int status = 0;

int state(char* tmp)
{
	if (strcmp(tmp, "Idle") == 0)    status = IDLE;
	if (strcmp(tmp, "Queue") == 0)   status = QUEUE;
	if (strcmp(tmp, "Run") == 0)     status = RUN;
	if (strcmp(tmp, "Hold") == 0)    status = HOLD;
	if (strcmp(tmp, "Home") == 0)    status = HOME;
	if (strcmp(tmp, "Alarm") == 0)   status = ALARM;
	if (strcmp(tmp, "Check") == 0)   status = CHECK;

	return status;
}

int state()
{
	return status;
}

// Main loop
void fast_loop()
{
	// This is the fast loop
	// ---------------------

}

void medium_loop()
{
	// This is the start of the medium loop 
	// -----------------------------------------

	char key;

	switch (medium_loopCounter)
	{
	case 0:
		medium_loopCounter++;

		delta_ms_medium_loop = millis() - medium_loopTimer;
		medium_loopTimer = millis();

		encoderPosition = ReadEncoder();
		encoderSwitch = ReadEncoderSwitch();

		break;

		//---------------------------------------------
	case 1:
		medium_loopCounter++;

		key = keypad.getKey();

		if (key != NO_KEY)
		{
			ProcessKey(key);
		}
		break;

		//------------------------------
	case 2:
		medium_loopCounter++;

		break;

		//--------------------------------------------
	case 3:
		medium_loopCounter++;

		break;

		// This case controls the slow loop
		//---------------------------------
	case 4:
		medium_loopCounter = 0;

		slow_loop();

		break;
	}
}

void slow_loop()
{
	// This is the slow (2Hz) loop pieces
	//----------------------------------------
	switch (slow_loopCounter) {
	case 0:
		slow_loopCounter++;

		delta_ms_slow_loop = millis() - slow_loopTimer;
		slow_loopTimer = millis();

		display_state();

		break;

	case 1:
		slow_loopCounter++;

		if (millis() - lastStateRXTime > 1000)
		{
			grblSerial.print("$G\n");
		}

		break;

	case 2:
		slow_loopCounter++;
		if (millis() - lastStatusRXTime > 1000)
		{
			grblSerial.print("?\n");
		}


		break;

	case 3:
		slow_loopCounter++;

		break;

	case 4:
		slow_loopCounter = 0;

		break;
	}
}

void one_second_loop()
{
	//char tmpStr[80];

	//sprintf(tmpStr, "Current Position: %f, %f, %f\n", currentPosition.x, currentPosition.y, currentPosition.z);
	//Serial.print(tmpStr);
}
