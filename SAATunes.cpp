/*********************************************************************************************

     SAATunes v 1.04
	 
	 A library to play MIDI tunes on the SAA1099 sound generator chip using an arduino
	 
	 Based on Len Shustek's PlayTune library, which plays a tune on a normal arduino
	 using its built in hardware timers.
	 You can find his library at: https://github.com/LenShustek/arduino-playtune
	 
	 Mine doesn't use timers, just a once-per-millisecond intterupt to turn notes
	 on/off and other things. All the code for addressing the SAA1099 and converting the 
	 bytestream is my own, and I've added a few tidbits that weren't in Len's original
	 library.
	 
	 Sepcial features and MIDI functions this library supports
	 
	 - Velocity Data (Which is interpreted as the initial note volume)
	 - Which channels are off/on, accessed through a boolean array
	 - Note volume decay (Decays the volume of a note that is held on over a specified rate)
	 
	 What I would like to add in the future
	 
	 - Using the SAA1099's built in noise channels for percussion (Which Len's original library supports)
	 - Using the SAA1099's built in envelope generators for different insturments (Maybe also some software
		generated envelopes, or wiggling the note's frequency back/forth slightly to produce a different sound?)
	 - Support for sustain pedal being used in a MIDI recording (Not sure if Len's original library supports 
		this or not)
	 

  ------------------------------------------------------------------------------------
   The MIT License (MIT)
   Original code Copyright (c) 2011, 2016, Len Shustek
   SAA1099 modified version Copyright (c) 2018, Jacob Field

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
  Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
**************************************************************************************/
/*
  Change log
   25 September 2017, Jacob Field
     - Original (release) library version, v1.03
	16 August 2018, Jacob Field
	- Fixed reported bug: first letter of <arduino.h> in the #include line not being capitalized was breaking library on linux.
	- Released as v1.04
  -----------------------------------------------------------------------------------------*/

#include <arduino.h>

#include "SAATunes.h"

//Volume (Velocity) variables
#define DO_VOLUME 1         // generate volume-modulating code? Needs -v on Miditones.
#define ASSUME_VOLUME  0    // if the file has no header, should we assume there is volume info?
#define DO_PERCUSSION 1     // generate percussion sounds? Needs DO_VOLUME, and -pt on Miditones
#define DO_DECAY 1     		// Do decay on sustained notes? We'll assume yes. Later we might make it toggleable in program.

static boolean volume_present = ASSUME_VOLUME;
struct file_hdr_t {  // the optional bytestream file header
  char id1;     // 'P'
  char id2;     // 't'
  unsigned char hdr_length; // length of whole file header
  unsigned char f1;         // flag byte 1
  unsigned char f2;         // flag byte 2
  unsigned char num_tgens;  // how many tone generators are used by this score
} file_header;
#define HDR_F1_VOLUME_PRESENT 0x80
#define HDR_F1_INSTRUMENTS_PRESENT 0x40
#define HDR_F1_PERCUSSION_PRESENT 0x20

//Percussion variables
#if DO_PERCUSSION
#define NO_DRUM 0xff
static byte drum_chan_one = NO_DRUM; // channel playing drum now
static byte drum_tick_count; // count ticks before bit change based on random bit
static byte drum_tick_limit; // count to this
static int drum_duration;   // limit on drum duration
#endif

const byte *score_start = 0;
const byte *score_cursor = 0;
volatile boolean SAATunes::tune_playing = false;
volatile unsigned long wait_toggle_count;      /* countdown score waits */
byte WR;
byte AO;

//Decay variables. Variable 0 in array is Channel 0, etc.
#if DO_DECAY
byte decayTimer[] = {0, 0, 0, 0, 0, 0};
byte decayVolume[] = {0, 0, 0, 0, 0, 0};
boolean doingDecay[] = {false, false, false, false, false, false};

//Modifyable delay rate variable (In MS, and each time the specified number of MS has passed, decays volume by 1/16).
unsigned int SAATunes::decayRate = 125; //Originally 125, which is what I determined is /roughly/ correct for how long piano notes sustain while held down
#endif

byte prevOctaves[] = {0, 0, 0, 0, 0, 0}; //Array for storing the last octave value for each channel

boolean SAATunes::channelActive[] = {0, 0, 0, 0, 0, 0}; //Array for storing whether a channel is active or not

void tune_playnote (byte chan, byte note, byte volume);
void tune_stopnote (byte chan);
void tune_stepscore (void);


// ******** Code Blocks ******** \\


//Pulses the WR pin connected to the SA1099 to load an address
void writeAddress() {
	
  digitalWrite(WR, LOW);
  delayMicroseconds(5);
  digitalWrite(WR, HIGH);
  
}


//Initiate the WR and AO pins, as well as reset/enable all sound channels
void SAATunes::init_pins (byte AZ, byte WE) {
	
	AO = AZ;
	WR = WE;
	
	DDRD = B11111111;
    pinMode(AO, OUTPUT);
    pinMode(WR, OUTPUT);
	digitalWrite(WR, HIGH);
	
	//Reset/Enable all the sound channels
	digitalWrite(AO, HIGH);
    PORTD = 0x1C;
    writeAddress();

    digitalWrite(AO, LOW);
    PORTD = 0x02;
    writeAddress();

    digitalWrite(AO, LOW);
    PORTD = 0x00;
    writeAddress();
	
	digitalWrite(AO, HIGH);
    PORTD = 0x1C;
    writeAddress();
    digitalWrite(AO, LOW);
    PORTD = B00000001;
    writeAddress();
	
	//Disable the noise channels
	digitalWrite(AO, HIGH);
	PORTD = 0x15;
	writeAddress();

	digitalWrite(AO, LOW);
	PORTD = B00000000;
	writeAddress();
	
	//Disable envelopes on Channels 2 and 5
	digitalWrite(AO, HIGH);
	PORTD = 0x18;
	writeAddress();

	digitalWrite(AO, LOW);
	PORTD = B00000000;
	writeAddress();
	
	digitalWrite(AO, HIGH);
	PORTD = 0x19;
	writeAddress();

	digitalWrite(AO, LOW);
	PORTD = B00000000;
	writeAddress();
	
	// Set up the interrupt
	OCR0A = 0xAF;
	TIMSK0 |= _BV(OCIE0A);
	
	//Play a little startup sound (Sounds cool, and gets rid of any random startup noise on the channels!)
	
	tune_stopnote(0);
	tune_stopnote(1);
	tune_stopnote(2);
	tune_stopnote(3);
	tune_stopnote(4);
	tune_stopnote(5);
	
	tune_playnote(3, 24, 64);
	delay(32);
	tune_playnote(0, 48, 64);
	delay(32);
	tune_playnote(1, 52, 64);
	delay(32);
	tune_playnote(2, 55, 64);
	delay(32);
	tune_playnote(3, 60, 64);
	delay(32);
	tune_playnote(4, 64, 64);
	delay(1024);
	tune_stopnote(0);
	tune_stopnote(1);
	tune_stopnote(2);
	tune_stopnote(3);
	tune_stopnote(4);
	tune_stopnote(5);
	
}


// Start playing a note on a particular channel

void tune_playnote (byte chan, byte note, byte volume) {
	
  SAATunes::channelActive[chan] = false;
	
  //Percussion code, in this version we're ignoring percussion. 
  if (note > 127) { // Notes above 127 are percussion sounds.
	note = 60; //Set note to some random place
	volume = 0; //Then set it to 0 volume
  }
  
  byte noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243}; // The 12 note-within-an-octave values for the SAA1099, starting at B
  byte octaveAdr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
  byte channelAdr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies

  //Shift the note down by 1, since MIDI octaves start at C, but octaves on the SAA1099 start at B
  note += 1; 

  byte octave = (note / 12) - 1; //Some fancy math to get the correct octave
  byte noteVal = note - ((octave + 1) * 12); //More fancy math to get the correct note

  prevOctaves[chan] = octave; //Set this variable so we can remember /next/ time what octave was /last/ played on this channel

  //Octave addressing and setting code:
  digitalWrite(AO, HIGH);
  PORTD = octaveAdr[chan / 2];
  writeAddress();

  digitalWrite(AO, LOW);
  if (chan == 0 || chan == 2 || chan == 4) {
    PORTD = octave | (prevOctaves[chan + 1] << 4); //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
  }
  
  if (chan == 1 || chan == 3 || chan == 5) {   
    PORTD = (octave << 4) | prevOctaves[chan - 1]; //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
  }
  
  writeAddress();
  
  //Note addressing and playing code
  //Set address to the channel's address
  digitalWrite(AO, HIGH);
  PORTD = channelAdr[chan];
  writeAddress();

  //EXPERIEMNTAL WARBLE CODE
  noteAdr[noteVal] += random(-2, 2); //a plus/minus value of 15 gives a really out of tune version
  
  
  //Write actual note data
  digitalWrite(AO, LOW);
  PORTD = noteAdr[noteVal];
  writeAddress();

  //Volume updating
  //Set the Address to the volume channel
  digitalWrite(AO, HIGH);
  PORTD = chan, HEX;
  writeAddress();
  
  #if DO_DECAY
	//Decay channel updating
	doingDecay[chan] = true;
	decayTimer[chan] = 0;
  #endif

  #if DO_VOLUME
	
	//Velocity is a value from 0-127, the SAA1099 only has 16 levels, so divide by 8.
     byte vol = volume / 8;

	digitalWrite(AO, LOW);
	PORTD = (vol << 4) | vol;
	writeAddress();
	
	#if DO_DECAY
		//Update the beginning volume for the decay controller
		decayVolume[chan] = vol;
	#endif
	
  #else
		
	//If we're not doing velocity, then just set it to max.
	digitalWrite(AO, LOW);
	PORTD = B11111111;
	writeAddress();
	
	#if DO_DECAY
		//Update the beginning volume for the decay controller
		decayVolume[chan] = 16;
	#endif
      
  #endif
  
  SAATunes::channelActive[chan] = true;

}


//-----------------------------------------------
// Stop playing a note on a particular channel
//-----------------------------------------------

void tune_stopnote (byte chan) {
	
	SAATunes::channelActive[chan] = false;
	
	/* CURRENTLY MODIFIED FOR HACKED SUSTAIN
	
	if (drum_chan_one == chan){ //If drum is active on this channel, then run special code to disable
		drum_chan_one = NO_DRUM;
		
		//Set noise generator mode
		digitalWrite(AO, HIGH);
		PORTD = 0x16;
		writeAddress();

		if (chan < 3){
			digitalWrite(AO, LOW);
			PORTD = PORTD | (0 >> 4);
			writeAddress();
		} else {
			digitalWrite(AO, LOW);
			PORTD = PORTD | (0 << 4);
			writeAddress();
		}
		
		//Enable tone generator
		digitalWrite(AO, HIGH);
		PORTD = 0x14;
		writeAddress();

		digitalWrite(AO, LOW);
		bitSet(PORTD, chan);
		writeAddress();
	
		//Disable noise generator
		digitalWrite(AO, HIGH);
		PORTD = 0x15;
		writeAddress();

		digitalWrite(AO, LOW);
		bitClear(PORTD, chan);
		writeAddress();
	
	} 
    
	byte volAddress[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

	digitalWrite(AO, HIGH);
	PORTD = volAddress[byte(chan)];
	writeAddress();

	digitalWrite(AO, LOW);
	PORTD = 0x00;
	writeAddress();

	
	#if DO_DECAY
		doingDecay[chan] = false;
		decayVolume[chan] = 16;
		decayTimer[chan] = 0;
	#endif
	
	*/
	
}


//-----------------------------------------------
// Start playing a score
//-----------------------------------------------

void SAATunes::tune_playscore (const byte *score) {
	
	volume_present = ASSUME_VOLUME;
	
	// look for the optional file header
	memcpy_P(&file_header, score, sizeof(file_hdr_t)); // copy possible header from PROGMEM to RAM
	if (file_header.id1 == 'P' && file_header.id2 == 't') { // validate it
		volume_present = file_header.f1 & HDR_F1_VOLUME_PRESENT;
		score_start += file_header.hdr_length; // skip the whole header
	}
	
    score_start = score;
    score_cursor = score;
    tune_stepscore();  /* execute initial commands */
    SAATunes::tune_playing = true; //Release the intterupt routine 
}


/* Do score commands until a "wait" is found, or the score is stopped.
This is called initially from tune_playscore, but then is called
from the interrupt routine when waits expire.
*/

#define CMD_PLAYNOTE	0x90	/* play a note: low nibble is generator #, note is next byte */
#define CMD_INSTRUMENT  0xc0 /* change instrument; low nibble is generator #, instrument is next byte */
#define CMD_STOPNOTE	0x80	/* stop a note: low nibble is generator # */
#define CMD_RESTART	0xe0	/* restart the score from the beginning */
#define CMD_STOP	0xf0	/* stop playing */
/* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit big-endian number of msec to wait */

void tune_stepscore (void) {
    byte cmd, opcode, chan, note, vol;
    unsigned duration;

    while (1) {
        cmd = pgm_read_byte(score_cursor++);
		
        if (cmd < 0x80) { /* wait count in msec. */
            duration = ((unsigned)cmd << 8) | (pgm_read_byte(score_cursor++));
            wait_toggle_count = duration; //((unsigned long) wait_timer_frequency2 * duration + 500) / 1000
		if (wait_toggle_count == 0) wait_toggle_count = 1;
            break;
        }
		
        opcode = cmd & 0xf0;
        chan = cmd & 0x0f; //Should erase the low nibble?
        if (opcode == CMD_STOPNOTE) { /* stop note */
            tune_stopnote (chan);
        }
        else if (opcode == CMD_PLAYNOTE) { /* play note */
            note = pgm_read_byte(score_cursor++); // argument evaluation order is undefined in C!
#if DO_VOLUME
      vol = volume_present ? pgm_read_byte(score_cursor++) : 127;
      tune_playnote (chan, note, vol);
#else
      if (volume_present) ++score_cursor; // skip volume byte
      tune_playnote (chan, note);
#endif
        }
        else if (opcode == CMD_RESTART) { /* restart score */
            score_cursor = score_start;
        }
        else if (opcode == CMD_STOP) { /* stop score */
            SAATunes::tune_playing = false;
            break;
        }
    }
}


//-----------------------------------------------
// Stop playing a score
//-----------------------------------------------

void SAATunes::tune_stopscore (void) {
    int i;
    for (i=0; i<6; ++i)
        tune_stopnote(i);
    SAATunes::tune_playing = false;
}


//Timer stuff
// Interrupt is called once a millisecond
 
SIGNAL(TIMER0_COMPA_vect) {
     
	//Begin new note code
	if (SAATunes::tune_playing && wait_toggle_count && --wait_toggle_count == 0) {
        // end of a score wait, so execute more score commands
        tune_stepscore ();
    }
	
	#if DO_DECAY
		//Note that this (the for loop) isn't ideal. Wish I didn't have to check every one, somehow.
		for(byte x = 0; x <= 5; x++){
		
			if(doingDecay[x] == true){
			
				//Add time to the timer
				decayTimer[x] += 1;
			
				if(decayTimer[x] >= SAATunes::decayRate){ //Check to see if we've met or exceeded the decay rate
					//Set the Address to the volume channel
					digitalWrite(AO, HIGH);
					PORTD = x, HEX;
					writeAddress();
				
					//Read the current volume, subtract one
					byte volume = decayVolume[x];
					decayVolume[x] -= 1;
				
					//Then, write modified volume
					digitalWrite(AO, LOW);
					PORTD = (volume << 4) | volume;
					writeAddress();
				
					decayTimer[x] = 0;
				
					if(volume == 1){
						doingDecay[x] = false;
					}
				}
			
			}
		}
	#endif
}




