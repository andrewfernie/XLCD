//=========================================================
//Project: GRBL Pendant
//Module:  serial_io.ino        
//=========================================================
//
// Author: Andrew Fernie
// Source code freely released - do with it what you like!
//
//----------------------------------------------------------
// This code started from the XLCD project by Frank Herrmann
//----------------------------------------------------------


// Get data from GRBL ==> PC
void serial_io_grbl()
{
	while (grblSerial.available())
	{
		char c = grblSerial.read();

		gsSerial.print(c);

		gr_chars++;

		// wait for a complete line 
		// and parse it
		if (c == '\n') {
			parseGrblLine(grserial);

			gr = 0;
			memset(&grserial[0], 0, sizeof(grserial));
			grserial[0] = '\0';
		}
		else {
			if (gr < BUFFER_SIZE)
				grserial[gr++] = c;
		}

		//// dont send data from $G to Serial, 
		//// cuz UGS don't understand this
		//// dont send data if string empty
		//if(grserial[0] != '['){
		//   Serial.print(c);
		//}

	}
}

// Get data from PC ==> GRBL
void serial_io_gs()
{
	while (gsSerial.available())
	{
		char c = gsSerial.read();

		grblSerial.print(c);

		pc_chars++;


		//   // wait for a complete line
		//   // and parse it
		//   if(c == '\n'){
		//      parsePCCommand(pcserial);
		//      pc = 0;
		//      memset(&pcserial[0], 0, sizeof(pcserial));
		//      pcserial[0] = '\0';
		//   } else {
		//      // if to big ...
		//      if(pc < BUFFER_SIZE){
		//      	pcserial[pc++] = c;
		//      }

		//   }

		//   // dont send serial commands (:char) to grbl
		//   if(pcserial[0] != ':'){
		   //    grblSerial.print(c);
		//   }

	}
}


// Analyze every command (from PC => Xlcd) and choose an action
void parsePCCommand(char* line)
{
	char* c2 = strrchr(line, '\r');
	*c2 = ' ';

	// All commands with an ':' at start can control XLCD 
	if (line[0] == ':')  parse_command_line(line);
}

// Analyze every line and choose an action
void parseGrblLine(char* line)
{
	char* c2 = strrchr(line, '\r');
	*c2 = ' ';
	//	Serial.println(line);

	if (line[0] == '<')
	{
		parse_status_line(line);
	}
	else if (line[0] == '[')
	{
		parse_state_line(line);
	}

}