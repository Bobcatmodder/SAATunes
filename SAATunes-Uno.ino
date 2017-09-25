/*************************************************************************************

     SAATunes v 1.03 (Original Release version)
   
   A library to play MIDI tunes on the SAA1099 sound generator chip using an arduino
   
   Based on Len Shustek's PlayTune library, which plays a tune on a normal arduino
   using its built in hardware timers.
   You can find his library at: https://github.com/LenShustek/arduino-playtune

---------------------------------------------------------------------------------------
   
   Original code Copyright (c) 2011, 2016, Len Shustek
   SAA1099 modified version Copyright (c) 2017, Jacob Field

   See LISCENSE.TXT for more details
   
**************************************************************************************/

#include "SAATunes.h"

//Include the file with the bytestream to interpret
#include "RagePenny.h"

//Initialize the library
SAATunes pt;


//This is 1/16th of the decay rate for sustained notes, which is 
//normally set to 125 (default if not set) but can be played with
//for a different type of sound, for instance, when set to some
//ridiculously low number like 4, it sounds like a marimba.

//pt.decayRate = 125; 

void setup() {

  //Initalize the WE and A0 pins, on digital pins 9, and 8. (Change to whatever)
  pt.init_pins(9, 8);
  
}

void loop() {

  //A delay to create a small pause between repeats, and between the startup chord and the song being played
  delay(2500);

  //Begin playing the score. 
  pt.tune_playscore(score); //The "score" here should be replaced with whatever the name of the byte array in the .H file is
  
  while(pt.tune_playing){
    //While the score is playing, do nothing. You can put something here, it will just call an interrupt every millisecond.
  }
  
}
