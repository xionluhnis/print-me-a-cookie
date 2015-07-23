
#include "error.h"
#include "parser.h"
#include "stepper.h"
#include "sdcard.h"

#include <SPI.h>
#include <SD.h>

// delays in milliseconds
#define delayFunc delayMicroseconds
#define DELAY_INTER 100
#define DELAY_AFTER 100
#define TENTH_MILLISECOND 1L
#define HALF_MILLISECOND  5L
#define MILLISECOND       10L
#define HALF_SECOND       (500L * MILLISECOND)
#define SECOND            (1000L * MILLISECOND)

// steppers
Stepper stpE0(2, 3, 4, 5, 6, 7);
Stepper stpY(8, 9, 10, 11, 12, 13);
Stepper stpZ(22, 23, 24, 25, 26, 27);
Stepper stpX(28, 29, 30, 31, 32, 33);

#define NUM_STEPPERS 4
Stepper *steppers[NUM_STEPPERS];

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
boolean idle();
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
boolean idle() {
  for(int i = 0; i < NUM_STEPPERS; ++i){
    if(steppers[i]->isRunning()) return false;
  }
  return true;
}

///// Reset states and errors //////////////////////////////////
void resetAll(){
  for(int i = 0; i < NUM_STEPPERS; ++i)
    steppers[i]->reset();
  error = ERR_NONE; // clean flag
  Serial.println("Reset.");
}

///// Process commands /////////////////////////////////////////
void readCommands(Stream& input){
  LineParser line(input);
  while(line.available() && error <= ERR_NONE){
    // create a command parser (subline)
    LineParser command = line.subline();
    
    // command type depends on first character
    char type = command.readChar();
    Serial.print('Command ');
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
      
      // --- sX where X = pin delta speed init
      case 's': {
        if(command.available()){
          int pin = command.readInt();
          unsigned long moves = command.readLong();
          unsigned long steps = command.readLong();
          unsigned long count = command.readLong();
          if(steps == 0){
            steps = HALF_MILLISECOND;
          }
          if(pin < 0 || pin >= NUM_STEPPERS){
            error = ERR_INPUT;
            break;
          }
          steppers[pin]->stepBy(moves, steps, count);
        }
      } break;
      
      // --- reset all processes (needed in case of error)
      case 'r':
      case 'R': {
        resetAll();
      } break;

      // --- microstepping mode
      case '+':
      case '-': {
        if(command.available()){
          char c = command.readChar();
          while(isBlankSpace(c))
            c = command.readChar();
          Stepper *stp = NULL;
          switch(c){
            case 'x':
            case 'X':
              stp = &stpX;
              break;
            case 'y':
            case 'Y':
              stp = &stpY;
              break;
            case 'z':
            case 'Z':
              stp = &stpZ;
              break;
            default:
              error = ERR_INPUT;
              return;
          }
          if(stp)
            stp->microstep(type == '+' ? Stepper::MS_FULL : Stepper::MS_1_16);
        }
      } break;

      // --- moveby dx dy dz sx [sy sz ix iy iz]
      case 'm': {
        long dx = command.readLong(),
             dy = command.readLong(),
             dz = command.readLong();
        unsigned long sx = command.readULong(),
                      sy = command.readULong(),
                      sz = command.readULong(),
                      ix = command.readULong(),
                      iy = command.readULong(),
                      iz = command.readULong();
        if(sx == 0L) sx = HALF_MILLISECOND;
        if(sy == 0L) sy = sx;
        if(sz == 0L) sz = sx;
        if(dx) stpX.stepBy(dx, sx, ix);
        if(dy) stpY.stepBy(dy, sy, iy);
        if(dz) stpZ.stepBy(dz, sz, iz);
      } break;

      // --- moveby dy [sy iy]
      case 'Y':
      case 'y': {
        if(type == 'y'){
          stpY.microstep(Stepper::MS_1_16);
        } else {
          stpY.microstep(Stepper::MS_1_1);
        }
        long delta = command.readLong();
        unsigned long speed = command.readULong(),
                      init  = command.readULong();
        if(speed == 0L) speed = HALF_MILLISECOND;
        if(delta) stpY.stepBy(delta, speed, init);
      } break;

      case 'X':
      case 'x': {
        if(type == 'x'){
          stpX.microstep(Stepper::MS_1_16);
        } else {
          stpX.microstep(Stepper::MS_1_1);
        }
        long delta = command.readLong();
        unsigned long speed = command.readULong(),
                      init  = command.readULong();
        if(speed == 0L) speed = HALF_MILLISECOND;
        if(delta) stpX.stepBy(delta, speed, init);
      } break;

      // extrude[X] delta [steps init]
      case 'E':
      case 'e': {
        char id = command.readChar();
        if(id == '1'){
          error = ERR_CMD_UNSUPPORTED;
          return;
        }
        Stepper &stp = stpE0; // TODO add stpE1
        if(type == 'e'){
          stp.microstep(HIGH);
        } else {
          stp.microstep(LOW);
        }
        long delta = -command.readLong();
        unsigned long speed = command.readULong(),
                      init  = command.readULong();
        if(speed == 0L) speed = HALF_MILLISECOND;
        stp.stepBy(delta, speed, init);
      } break;

      // wait [time]
      case 'W':
      case 'w': {
        int time = command.readInt();
        if(!time){
          if(type == 'W')
            time = HALF_SECOND;
          else
            time = HALF_MILLISECOND;
        } else {
          if(type == 'W')
            time *= SECOND;
          else
            time *= MILLISECOND;
        }
        delayFunc(time);
      } break;

      /**
       * Successful commands
       * 
       * e/y1000 5 = 1000 moves at 5 steps speed with delayMicroseconds 100
       */

     // --- test sd card and list all files
      case 'L': {
        sdcard::test();
      } break;

      // --- list files we can open at root
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
          
          // TODO execute content
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
}

///// Processing loop //////////////////////////////////////////
void loop() {
  // show errors
  logError();

  // 1 = react to external events
  react();

  // 2 = process scheduled events
  if(error == ERR_NONE){
    process();
  }

  // 3 = read user input
  if(error > ERR_NONE){
    for(int i = 0; i < NUM_STEPPERS; ++i){
      steppers[i]->reset();
    }
    if(errorCallback){
      errorCallback(error);
      errorCallback = NULL;
      return;
    }
  }
  if(idle()){
    // if there is a special callback, do that only
    if(idleCallback){
      Callback cb = idleCallback;
      idleCallback = NULL;
      cb(0);
    } else {
      Serial.println("Idle wait.");
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

