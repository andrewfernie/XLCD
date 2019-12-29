//=========================================================
//Project: GRBL Pendant
//Module:  lcd_screens.ino        
//=========================================================
//
// Author: Andrew Fernie
// Source code freely released - do with it what you like!
//
//----------------------------------------------------------
// This code started from the XLCD project by Frank Herrmann
//----------------------------------------------------------


unsigned long lastCall = 0;

// State codes for Grbl 1.1.   Source https://github.com/gnea/grbl/wiki/Grbl-v1.1-Commands
// 
// Motion Mode                 G0, G1, G2, G3, G38.2, G38.3, G38.4, G38.5, G80
enum class MotionMode { Undefined, Rapid, Linear, CW, CCW, Probe_2, Probe_3, Probe_4, Probe_5, Cancel };
MotionMode currentMotionMode = MotionMode::Undefined;

// Coordinate System Select    G54, G55, G56, G57, G58, G59
enum class CoordinateSystemSelect { Undefined, WCS1, WCS2, WCS3, WCS4, WCS5, WCS6 };
CoordinateSystemSelect currentCoordinateSystemSelect = CoordinateSystemSelect::Undefined;

// Plane Select                G17, G18, G19
enum class PlaneSelect { Undefined, XY, ZX, YZ };
PlaneSelect currentPlaneSelect = PlaneSelect::Undefined;

// Distance Mode               G90, G91
enum class DistanceMode { Undefined, Absolute, Incremental };
DistanceMode currentDistanceMode = DistanceMode::Undefined;

// Arc IJK Distance Mode       G91.1
enum class ArcDistanceMode { Off, On };
ArcDistanceMode currentArcDistanceMode = ArcDistanceMode::On;

// Feed Rate Mode              G93, G94
enum class FeedRateMode { Undefined, Inverse, Normal };
FeedRateMode currentFeedRateMode = FeedRateMode::Undefined;

// Units Mode                  G20, G21
enum class UnitsMode { Undefined, Inches, mm };
UnitsMode currentUnitsMode = UnitsMode::Undefined;

// Cutter Radius Compensation  G40
enum class CutterRadiusCompensation { Off, On };
CutterRadiusCompensation currentCutterRadiusCompensation = CutterRadiusCompensation::Off;

// Tool Length Offset          G43.1, G49
enum class ToolLengthOffsetMode { Undefined, Dynamic, Cancel };
ToolLengthOffsetMode currentToolLengthOffsetMode = ToolLengthOffsetMode::Undefined;

// Program Mode                M0, M1, M2, M30
enum class ProgramMode { Undefined, Stop, Optional, End, Rewind };
ProgramMode currentProgramMode = ProgramMode::Undefined;

// Spindle State               M3, M4, M5
enum class SpindleState { Undefined, CW, CCW, Off };
SpindleState currentSpindleState = SpindleState::Undefined;

// Coolant State               M7, M8, M9
enum class CoolantState { Undefined, Mist, Flood, Off };
CoolantState currentCoolantState = CoolantState::Undefined;

// Tool
char currentTool[] = "   ";

// Feed rate
char currentFeedRate[] = "          ";


void parse_status_line(char* line)
{
	// Typical status lines formats
	//
	// <Idle | WPos:73.000, 0.000, -20.000 | FS : 0, 0>
	// <Idle | WPos:73.000, 0.000, -20.000 | FS : 0, 0 | WCO : -75.000, 0.000, -358.000>
	// <Idle | WPos:73.000, 0.000, -20.000 | FS : 0, 0 | Ov : 100, 100, 100>

	// Display on LCD ...
	// lcd screen
	// |--------------|
	// State  Z:555.529
	// 000.000  000.000

	// XXX: to use a switch to display work position or other things :)
	//myLCD.clear();

	char delim[] = "<,:|";
	char tmpStr[LCD_cols];
	size_t len;
	char* temp;

	lastStatusRXTime = millis();

	// First Line
	// State ..
	temp = split(line, delim, 0);
	myLCD.setCursor(0, 0); // letter, row
	sprintf(tmpStr, "%-5s", temp);
	myLCD.print(tmpStr);
	state(temp);

	// Z Machine position ...
	temp = split(line, delim, 4);
	sprintf(tmpStr, "%9.9s", temp);
	len = strlen(tmpStr);
	myLCD.setCursor((LCD_cols - len), 0);
	myLCD.print(tmpStr);

	// Second Line
	// X Machine position ...
	temp = split(line, delim, 2);
	myLCD.setCursor(0, 1);
	sprintf(tmpStr, "%-9s", temp);
	myLCD.print(tmpStr);

	// Y Machine position ...
	temp = split(line, delim, 3);
	sprintf(tmpStr, "%9.9s", temp);
	len = strlen(tmpStr);
	myLCD.setCursor((LCD_cols - len), 1);
	myLCD.print(tmpStr);

	lastCall = millis();
}


// send every second the command $G
void parse_state_line(char* stateLine)
{
	// Typical state line
	// [GC:G0 G54 G17 G21 G90 G94 M5 M9 T0 F0 S0]
	//


	char delim[] = "[ ]:";

	//             mm                   TNr Feed
	// G0 G54 G17 G21 G90 G94 M0 M5 M9 T0 F500.000

	char* thisToken;

	char buffer[BUFFER_SIZE];

	lastStateRXTime = millis();

	strcpy(buffer, stateLine);

	// init and create first cut
	thisToken = strtok(buffer, delim);

	while (thisToken != NULL)
	{
		if (thisToken[0] == 'G')
		{
			// Motion Mode
			if (strcmp(thisToken, "G0") == 0)
				currentMotionMode = MotionMode::Rapid;
			else if (strcmp(thisToken, "G1") == 0)
				currentMotionMode = MotionMode::Linear;
			else if (strcmp(thisToken, "G2") == 0)
				currentMotionMode = MotionMode::CW;
			else if (strcmp(thisToken, "G3") == 0)
				currentMotionMode = MotionMode::CCW;
			else if (strcmp(thisToken, "G38.2") == 0)
				currentMotionMode = MotionMode::Probe_2;
			else if (strcmp(thisToken, "G38.3") == 0)
				currentMotionMode = MotionMode::Probe_3;
			else if (strcmp(thisToken, "G38.4") == 0)
				currentMotionMode = MotionMode::Probe_4;
			else if (strcmp(thisToken, "G38.5") == 0)
				currentMotionMode = MotionMode::Probe_5;
			else if (strcmp(thisToken, "G80") == 0)
				currentMotionMode = MotionMode::Cancel;

			// Plane Select
			else if (strcmp(thisToken, "G17") == 0)
				currentPlaneSelect = PlaneSelect::XY;
			else if (strcmp(thisToken, "G18") == 0)
				currentPlaneSelect = PlaneSelect::ZX;
			else if (strcmp(thisToken, "G19") == 0)
				currentPlaneSelect = PlaneSelect::YZ;

			// Units Mode
			else if (strcmp(thisToken, "G20") == 0)
				currentUnitsMode = UnitsMode::Inches;
			else if (strcmp(thisToken, "G21") == 0)
				currentUnitsMode = UnitsMode::mm;

			// Coordinate System Select    
			else if (strcmp(thisToken, "G54") == 0)
				currentCoordinateSystemSelect = CoordinateSystemSelect::WCS1;
			else if (strcmp(thisToken, "G55") == 0)
				currentCoordinateSystemSelect = CoordinateSystemSelect::WCS2;
			else if (strcmp(thisToken, "G56") == 0)
				currentCoordinateSystemSelect = CoordinateSystemSelect::WCS3;
			else if (strcmp(thisToken, "G57") == 0)
				currentCoordinateSystemSelect = CoordinateSystemSelect::WCS4;
			else if (strcmp(thisToken, "G58") == 0)
				currentCoordinateSystemSelect = CoordinateSystemSelect::WCS5;
			else if (strcmp(thisToken, "G59") == 0)
				currentCoordinateSystemSelect = CoordinateSystemSelect::WCS6;

			// Distance Mode
			else if (strcmp(thisToken, "G90") == 0)
				currentDistanceMode = DistanceMode::Absolute;
			else if (strcmp(thisToken, "G91") == 0)
				currentDistanceMode = DistanceMode::Incremental;

			// Arc Distance Mode
			// Doesn't seem to make sense - need to look into this one in more detail
			else if (strcmp(thisToken, "G91.1") == 0)
				currentArcDistanceMode = ArcDistanceMode::On;

			// FeedRate Mode
			else if (strcmp(thisToken, "G93") == 0)
				currentFeedRateMode = FeedRateMode::Inverse;
			else if (strcmp(thisToken, "G94") == 0)
				currentFeedRateMode = FeedRateMode::Normal;

			// Cutter Radius Compensation
			else if (strcmp(thisToken, "G40") == 0)
				currentCutterRadiusCompensation = CutterRadiusCompensation::On;


			// Tool Length Offset          G43.1, G49
			else if (strcmp(thisToken, "G43.1") == 0)
				currentToolLengthOffsetMode = ToolLengthOffsetMode::Dynamic;
			else if (strcmp(thisToken, "G49") == 0)
				currentToolLengthOffsetMode = ToolLengthOffsetMode::Cancel;

		}
		else if (thisToken[0] == 'M')
		{
			// Program Mode                M0, M1, M2, M30
			if (strcmp(thisToken, "M0") == 0)
				currentProgramMode = ProgramMode::Stop;
			else if (strcmp(thisToken, "M1") == 0)
				currentProgramMode = ProgramMode::Optional;
			else if (strcmp(thisToken, "M2") == 0)
				currentProgramMode = ProgramMode::End;
			else if (strcmp(thisToken, "M30") == 0)
				currentProgramMode = ProgramMode::Rewind;

			// Spindle
			else if (strcmp(thisToken, "M3") == 0)
				currentSpindleState = SpindleState::CW;
			else if (strcmp(thisToken, "M4") == 0)
				currentSpindleState = SpindleState::CCW;
			else if (strcmp(thisToken, "M5") == 0)
				currentSpindleState = SpindleState::Off;

			// Coolant
			else if (strcmp(thisToken, "M7") == 0)
				currentCoolantState = CoolantState::Mist;
			else if (strcmp(thisToken, "M8") == 0)
				currentCoolantState = CoolantState::Flood;
			else if (strcmp(thisToken, "M9") == 0)
				currentCoolantState = CoolantState::Off;
		}
		else if (thisToken[0] == 'T')
		{
			strcpy(currentTool, thisToken);
		}
		else if (thisToken[0] == 'F')
		{
			strcpy(currentFeedRate, &(thisToken[1]));
		}

		// next one
		thisToken = strtok(NULL, delim);
	}
}

void display_state()
{

	// Display on LCD ...
	// |--------------|
	// S1 T1 F1000
	// MM LIN XY M1

	char tmpStr[LCD_cols];

	myLCD.setCursor(0, 2); //third row

	switch (currentMotionMode)
	{
	case MotionMode::Rapid:
		myLCD.print("RAP ");
		break;
	case MotionMode::Linear:
		myLCD.print("LIN ");
		break;
	case MotionMode::CW:
		myLCD.print("CW  ");
		break;
	case MotionMode::CCW:
		myLCD.print("CCW ");
		break;
	case MotionMode::Probe_2:
		myLCD.print("PRB2");
		break;
	case MotionMode::Probe_3:
		myLCD.print("PRB3");
		break;
	case MotionMode::Probe_4:
		myLCD.print("PRB4");
		break;
	case MotionMode::Probe_5:
		myLCD.print("PRB5");
		break;
	default:
		myLCD.print("--- ");
	}

	// Feed
	sprintf(tmpStr, "%4.4s", currentFeedRate);
	myLCD.setCursor((LCD_cols - strlen(tmpStr) - 4), 2);
	myLCD.print(strcat(tmpStr, "mm/s"));
	
	// next line
	myLCD.setCursor(0, 3);

	// Units
	switch (currentUnitsMode)
	{
	case UnitsMode::Inches:
		myLCD.print("in");
		break;
	case UnitsMode::mm:
		myLCD.print("mm");
		break;
	default:
		myLCD.print("--");
	}

	myLCD.print(" ");

	// Plane
	switch (currentPlaneSelect)
	{
	case PlaneSelect::XY:
		myLCD.print("XY");
		break;
	case PlaneSelect::ZX:
		myLCD.print("ZX");
		break;
	case PlaneSelect::YZ:
		myLCD.print("YZ");
		break;
	default:
		myLCD.print("--");
	}

	myLCD.print(" ");

	// Tool
	sprintf(tmpStr, "%3.3s", currentTool);
	myLCD.print(tmpStr);
	myLCD.print(" ");

	// Program Mode
	switch (currentProgramMode)
	{
	case ProgramMode::Stop:
		strcpy(tmpStr,"Pause");
		break;
	case ProgramMode::Optional:
		strcpy(tmpStr,"Pause");
		break;
	case ProgramMode::End:
		strcpy(tmpStr,"End");
		break;
	case ProgramMode::Rewind:
		strcpy(tmpStr,"Rwnd");
		break;
	default:
		strcpy(tmpStr,"-----");
	}

    myLCD.setCursor((LCD_cols - strlen(tmpStr)), 3);
    myLCD.print(tmpStr);
}
