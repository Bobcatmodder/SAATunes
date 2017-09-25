/*********************************************************************************************

     SAATunes v 1.03 (Original Release version)
	 
	 A library to play MIDI tunes on the SAA1099 using an arduino. 
	 For documentation, see SAATunes.cpp
	 
   Copyright (c) 2011, 2016, Len Shustek
   SAA1099 version Copyright (c) 2017, Jacob Field
   
**************************************************************************************/
/*
  Change log
   25 September 2017, Jacob Field
     - Original (release) library version, v1.03
  -----------------------------------------------------------------------------------------*/

#ifndef SAATunes_h
#define SAATunes_h

#include <arduino.h>

class SAATunes
{
public:
 void init_pins(byte AZ, byte WE); 				// Define the pins that WE and AO are connected to (Naming conflict with AO, which is why it's AZ)
 void tune_playscore (const byte *score);		// Start playing a score
 volatile static boolean tune_playing;			// Is the score still playing?
 static unsigned int decayRate;					// 1/16 of the rate in Milliseconds in which to decay notes
 static boolean channelActive[];        		// An array which stores which channels are active and which are not
 void tune_stopscore (void);					// Stop playing the score
};

#endif