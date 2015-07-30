
#include "error.h"
#include "parser.h"
#include "stepper.h"
#include "locator.h"
#include "elevator.h"
#include "sdcard.h"

#include <SPI.h>
#include <SD.h>

// delays in milliseconds
#define delayFunc delayMicroseconds
#define DELAY_INTER 100
#define DELAY_AFTER 100
#define TENTH_MILLISECOND 1L
#define HALF_MILLISECOND  (5L * TENTH_MILLISECOND)
#define MILLISECOND       (2L * HALF_MILLISECOND)
#define HALF_SECOND       (500L * MILLISECOND)
#define SECOND            (1000L * MILLISECOND)

#define DEFAULT_SPEED     5L

// steppers
Stepper stpE0(2, 3, 4, 5, 6, 7);
Stepper stpY(8, 9, 10, 11, 12, 13);
Stepper stpZ(22, 23, 24, 25, 26, 27);
Stepper stpX(28, 29, 30, 31, 32, 33);

#define NUM_STEPPERS 4
Stepper *steppers[NUM_STEPPERS];

// position system
Locator locXY(&stpX, &stpY);
Elevator locZ(&stpZ);

// callback type
typedef void (*Callback)(int state);

// global callbacks
Callback idleCallback, errorCallback;

//
// declaration of functions
//
void setup();
void loop();
void react();
void process();
bool idle();
Stepper *selectStepper(char c);
void readCommands(Stream& input = Serial);
void processFile(File &file);

///// Setup Arduino ////////////////////////////////////////////
void setup() {
  steppers[0] = &stpE0;
  steppers[1] = &stpY;
  steppers[2] = &stpZ;
  steppers[3] = &stpX;
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->setup();
  }
  Serial.begin(9600); //Open Serial connection for debugging

  // sd card setup
  sdcard::begin();

  // global callbacks
  idleCallback = errorCallback = NULL;
}

///// React to external input //////////////////////////////////
void react() {

}

///// Process events ///////////////////////////////////////////
void process() {
	// update location
	locXY.update();
	locZ.update();
	
	// update steppers
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->exec();
  }
  delayFunc(DELAY_INTER);
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->release();
  }
  delayFunc(DELAY_AFTER);
}

///// Check if idle ////////////////////////////////////////////
bool idle() {
  for(int i = 0; i < NUM_STEPPERS; ++i){
    if(steppers[i]->isRunning()) return false;
  }
  return true;
}

///// Reset states and errors //////////////////////////////////
void resetAll(){
  for(int i = 0; i < NUM_STEPPERS; ++i)
    steppers[i]->reset();
  locXY.reset();
  locZ.reset();
  error = ERR_NONE; // clean flag
  // remove callbacks
  idleCallback = NULL;
  errorCallback = NULL;
  Serial.println("Reset.");
}

///// Process commands /////////////////////////////////////////
void readCommands(Stream& input){
  LineParser line(input);
  vec2 delta;
  long dz = 0L;
  while(line.available() && error <= ERR_NONE){
    // create a command parser (subline)
    LineParser command = line.subline();
    
    // command type depends on first character
    char type = command.readChar();
    Serial.print("Command ");
    Serial.print(type);
    Serial.print(" (");
    Serial.print(input.available(), DEC);
    Serial.println(")");
    switch(type){

      // --- line comment
      case '#': {
        Serial.print("#");
        line.skip(true); // we skip the full line
        Serial.println("");
      } break;

      // --- new line
      case '\n':
      case '\r':
        Serial.println("Newline");
        return;
      
      // --- reset all processes (needed in case of error)
      case 'r':
      case 'R': {
        resetAll();
      } break;

      // --- disable switch on steppers
      case 'd':
      case 'D': {
        for(int i = 0; i < NUM_STEPPERS; ++i){
          if(!steppers[i]->isRunning() && steppers[i]->isEnabled()){
            steppers[i]->disable();
            Serial.print("Disabling pin ");
            Serial.println(i, DEC);
          }
        }
      } break;

      // --- microstepping pin mode
      case 'U':
      case 'u': {
        if(command.available()){
          Stepper *stp = selectStepper(command.readFullChar());
          if(!stp) return;
          // choosing the mode
          int mode = Stepper::MS_1_16;
          switch(command.readInt()){
            case 0:
            case 16:
              mode = Stepper::MS_1_16;
              break;
            case 8: mode = Stepper::MS_1_8; break;
            case 4: mode = Stepper::MS_1_4; break;
            case 2: mode = Stepper::MS_1_2; break;
            case 1: mode = Stepper::MS_1_1; break;
            default:
              error = ERR_INVALID_MS_MODE;
              return;
          }
          if(stp)
            stp->microstep(mode);
        }
      } break;

      // --- moveto x y
      // --- moveby dx dy
      case 'M':
      case 'm': {
        vec2 p(
        	command.readLong(),
        	command.readLong()
        );
        if(type == 'M'){
        	p -= locXY.currentTarget();
        }
        delta = p;
      } break;

			// --- elevateto z
			// --- elevateby dz
      case 'Z':
      case 'z': {
      	dz = command.readLong();
      	if(type == 'Z') dz -= stpZ.value();
      } break;
      
      // --- extrude period
      case 'E':
      case 'e': {
        long freq = -command.readLong();
        stpE0.moveFreqTo(freq);
      } break;

      // --- set pin code value
      case 'S':
      case 's': {
      	char c = command.readFullChar();
      	switch(c){
      		// - stepper settings
      		case 'X':
      		case 'x':
      		case 'Y':
      		case 'y':
      		case 'Z':
      		case 'z':
      		case 'E':
      		case 'e': {
      			Stepper *stp = selectStepper(c);
      			if(!stp) return;
      			char c1 = command.readFullChar();
      			char c2 = command.readChar();
      			if(c1 == 'd' && c2 == 'f'){
      				stp->setMaxDeltaFreq(command.readULong());
      			} else if(c1 == 'f' && c2 == 's'){
      				stp->setSafeFreq(command.readULong());
      			} else {
      				error = ERR_INVALID_SETTINGS;
      				return;
      			}
      		} break;
      		
      		// - xy location settings
      		case 'M':
      		case 'm': {
      			char c1 = command.readFullChar();
      			char c2 = command.readChar();
      			if(c1 == 'd' && c2 == 'f'){
      				locXY.setMaxDeltaFreq(command.readULong());
      			} else if(c1 == 'f' && c2 == 'b'){
      				locXY.setBestFreq(command.readULong());
      			} else {
      				char c3 = command.readChar();
      				if(c1 == 'e' && c2 == 'p' && c3 == 's'){
      					locXY.setPrecision(command.readULong());
      				} else{
		    				error = ERR_INVALID_SETTINGS;
		    				return;
      				}
      			}
      		} break;
      		
      		// - z location settings
      		case 'H':
      		case 'h': {
      			char c1 = command.readFullChar();
      			char c2 = command.readChar();
      			if(c1 == 'd' && c2 == 'f'){
      				locZ.setMaxDeltaFreq(command.readULong());
      			} else if(c1 == 'f' && c2 == 'b'){
      				locZ.setBestFreq(command.readULong());
      			} else {
      				error = ERR_INVALID_SETTINGS;
      				return;
      			}
      		} break;
      		
      		default:
      			error = ERR_INVALID_SETTINGS;
      			return;
      	}
      } break;

      // --- wait [time]
      case 'W':
      case 'w': {
        unsigned long time = command.readULong();
        if(!time)
          time = 50L * (type == 'W' ? 10L : 1L); // w0 = 50ms, W0 = 500ms 
        else if(type == 'W')
          time *= 1000L;
        delay(time);
      } break;

      /**
       * Successful commands
       * 
       * e/y1000 5 = 1000 moves at 5 steps speed with delayMicroseconds 100
       */

      // --- list files we can open at root
      case 'L':
      case 'l': {
        // list files on SD card
        sdcard::list();
      } break;

      case 'o': {
        // open and execute file
        int fileID = command.readInt();
        if(fileID > 0){
          File &f = sdcard::open(fileID);
          Serial.print("Opening ");
          Serial.println(f.name());
          
          // execute content
          processFile(f);
          
        } else {
          Serial.println("File ID must be strictly positive.");
        }
      } break;

      default:
        // otherwise
        break;
    }
  }
  
  // should we set a move?
  if(delta.x || delta.y){
  	locXY.setTarget(delta + locXY.target());
  }
  if(delta.z){
  	locZ.setTarget(dz + locZ.target());
  }
}
Stepper *selectStepper(char c){
  Stepper *stp = NULL;
  switch(c){
    case 'X':
    case 'x':
      stp = &stpX;
      break;
    case 'Y':
    case 'y':
      stp = &stpY;
      break;
    case 'Z':
    case 'z':
      stp = &stpZ;
      break;
    case 'E':
    case 'e':
      stp = &stpE0;
      break;
    default:
      error = ERR_INVALID_STEPPER;
      break;
  }
  return stp;
}

///// Processing loop //////////////////////////////////////////
void loop() {
  // show errors
  logError();

  // 1 = react to external events
  react();

  // 2 = process scheduled events
  if(error == ERR_NONE){
    // a) disable motors that won't do anything at this step
    for(int i = 0; i < NUM_STEPPERS; ++i){
      if(!steppers[i]->isRunning() && steppers[i]->isEnabled()){
        steppers[i]->disable();
        Serial.print("Disabling pin ");
        Serial.println(i, DEC);
      }
    }
    // b) process steps
    process();
  } else if(error > ERR_NONE){
    // reset all the motors because of error
    for(int i = 0; i < NUM_STEPPERS; ++i){
      steppers[i]->reset();
    }
    if(errorCallback){
      errorCallback(error);
      errorCallback = NULL;
      return;
    }
    idleCallback = NULL;
  }

  // 3 = read user input
  if(idle()){
    // if there is a special callback, do that only
    if(idleCallback){
      Callback cb = idleCallback;
      idleCallback = NULL;
      cb(0);
    } else {
      // Serial.println("Idle wait.");
      delay(1000);
    }
  }
  readCommands();
  Serial.flush();
}

///// File processing //////////////////////////////////////////
void processFileError(int error){
  // we stop the file processing
  File &file = sdcard::currentFile();
  if(file){
    Serial.print("Closing ");
    Serial.println(file.name());
    file.close();
  }

  // we remove the idle callback, which effectively
  // stops the continuous command processing
  idleCallback = NULL;
}

void processNextLine(int state = 0){
  File &file = sdcard::currentFile();
  Serial.print("Next line of ");
  Serial.println(file.name());
  Serial.println(file.available(), DEC);
  if(!file){
    error = ERR_FILE_UNAVAILABLE;
    return;
  } else if(!file.available()){
    Serial.print("EOF: ");
    Serial.println(file.name());
    file.close();
    return;
  }
  idleCallback = processNextLine; // trigger again for next line on next idle time
  readCommands(file);
}

void processFile(File &file){
  if(!file){
    Serial.println("No file to process!");
    file.close();
    return;
  }

  // TODO global positioning and setup

  // process file lines one by one
  errorCallback = processFileError;
  processNextLine();
}

