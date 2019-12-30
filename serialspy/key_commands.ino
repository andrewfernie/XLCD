//=========================================================
//Project: GRBL Pendant
//Module:  key_commands.ino        
//=========================================================
//
// Author: Andrew Fernie
// Source code freely released - do with it what you like!
//
//----------------------------------------------------------
// This code started from the XLCD project by Frank Herrmann
//---------------------------------------------------------

void ProcessKey(char key)
{
	switch (key)
	{
	case '1':

		break;

	case '2':
		break;

	case '3':
		break;

	case '4':
		// Unlock
		grblSerial.print("$X\n");		
		break;

	case '5':
		break;

	case '6':
		break;

	case '7':
		break;

	case '8':
		// Home
		grblSerial.print("$H\n");
		break;

	case '9':
		break;

	case '0':
		break;

	case 'A':
		break;

	case 'B':
		break;

	case 'C':
		break;

	case 'D':
		break;

	case 'E':
		break;

	case 'F':
		break;

		//default:
	}
}
