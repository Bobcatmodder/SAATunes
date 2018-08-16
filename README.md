# SAATunes v 1.04 
(Same as v1.03 but with one bug fix)
	 
A library to play MIDI tunes on the SAA1099 sound generator chip using an arduino
   
   Demo: https://www.youtube.com/watch?v=ijNp12XhHPQ
	 
	 Based on Len Shustek's PlayTune library, which plays a tune on a normal arduino
	 using its built in hardware timers.
	 You can find his library at: https://github.com/LenShustek/arduino-playtune
	 
   
How to use this library:
   
   	Here's a video tutorial: https://www.youtube.com/watch?v=0UxQgYc3ELU&t=15s
	Instructable coming soon! (ish)
   
Special features and MIDI functions this library supports
	 
	 	- Velocity Data (Which is interpreted as the initial note volume)
	 	- Which channels are off/on, accessed through a boolean array
	 	- Note volume decay (Decays the volume of a note that is held on over a specified rate)
	 
What I would like to add in the future
	 
	 	- Using the SAA1099's built in noise channels for percussion (Which Len's original library supports)
		 - Using the SAA1099's built in envelope generators for different insturments (Maybe also some software generated envelopes, or wiggling the note's frequency back/forth slightly to produce a different sound?)
	 	- Support for sustain pedal being used in a MIDI recording (Not sure if Len's original library supports this or not)
	 
