# SAATunes v 1.03 (Original Release version)
	 
A library to play MIDI tunes on the SAA1099 sound generator chip using an arduino
   
   Demo: https://www.youtube.com/watch?v=ijNp12XhHPQ
	 
	 Based on Len Shustek's PlayTune library, which plays a tune on a normal arduino
	 using its built in hardware timers.
	 You can find his library at: https://github.com/LenShustek/arduino-playtune
	 
   
How to use this library:
   
   	For the moment, I've not documented this yet. I plan to as soon as possible, with an instructable
   	and video. However, if anyone is really impatient, here's a brief overview. 
   
   	1. Get an SAA1099 and a 8Mhz TTL oscillator from eBay, connect it to the Arduino's PORTD (Pins 0-7)
   	2. Install the library in your Arduino IDE, use the example sketch to test it
   	3. Find a MIDI file you can use. Prefereably with no percussion, that can mess it up. If it does have percussion, use a program like Anvil studio to remove the percussion tracks.
   	4. Use Len Shushtek's MIDITONES program (Find it at https://github.com/LenShustek/miditones) to convert your MIDI to a 		bytestream. The best options usually are -v -d, unless your file doesn't include velocity data, in which case neither one is needed.
   	5. Copy the TXT output from the MIDITONES program into a new .H file in the arduino editor.
   	6. Change the line where it says "unsigned char" to "byte"
   	7. Change the line in the main example program that says "RagePenny.h" to say "YourFileNameHere.h"
   	8. Upload it to your arduino.
   
   	As noted before, it'd be best to wait till I have the actual guide out, which I will try to finish ASAP.
   
Special features and MIDI functions this library supports
	 
	 	- Velocity Data (Which is interpreted as the initial note volume)
	 	- Which channels are off/on, accessed through a boolean array
	 	- Note volume decay (Decays the volume of a note that is held on over a specified rate)
	 
What I would like to add in the future
	 
	 	- Using the SAA1099's built in noise channels for percussion (Which Len's original library supports)
		 - Using the SAA1099's built in envelope generators for different insturments (Maybe also some software generated envelopes, or wiggling the note's frequency back/forth slightly to produce a different sound?)
	 	- Support for sustain pedal being used in a MIDI recording (Not sure if Len's original library supports this or not)
	 
