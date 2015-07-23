
#include "error.h"
#include "types.h"
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
    // command type depends on first character
    char type = line.readChar();
    switch(type){

      // --- line comment
      case '#': {
        Serial.print("#");
        line.skip(true);
      } break;

      // --- new line
      case '\n':
      case '\r':
        return;
      
      // --- sX where X = pin delta speed init
      case 's': {
        if(line.available()){
          int pin = line.readInt();
          unsigned long moves = line.readLong();
          unsigned long steps = line.readLong();
          unsigned long count = line.readLong();
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
        if(line.available()){
          int pin = line.readInt();
          if(pin < 0 || pin >= NUM_STEPPERS){
            error = ERR_INPUT;
            break;
          }
          steppers[pin]->microstep(type == '+' ? Stepper::MS_FULL : Stepper::MS_1_16);
        }
      } break;

      // --- moveby dx dy dz sx [sy sz ix iy iz]
      case 'm': {
        long dx = line.readLong(),
             dy = line.readLong(),
             dz = line.readLong();
        unsigned long sx = line.readULong(),
                      sy = line.readULong(),
                      sz = line.readULong(),
                      ix = line.readULong(),
                      iy = line.readULong(),
                      iz = line.readULong();
        if(sx == 0L) sx = HALF_MILLISECOND;
        if(sy == 0L) sy = sx;
        if(sz == 0L) sz = sx;
        if(dx) stpX.stepBy(dx, sx, ix);
        if(dy) stpY.stepBy(dy, sy, iy);
        if(dz) stpZ.stepBy(dz, sz, iz);
      } break;

      // --- moveby dy sy iy
      case 'Y':
      case 'y': {
        if(type == 'y'){
          stpY.microstep(HIGH);
        } else {
          stpY.microstep(LOW);
        }
        long delta = line.readLong();
        unsigned long speed = line.readULong(),
                      init  = line.readULong();
        if(speed == 0L) speed = HALF_MILLISECOND;
        if(delta) stpY.stepBy(delta, speed, init);
      }

      // extrude delta steps init eid
      case 'E':
      case 'e': {
        char id = line.readChar();
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
        long delta = -line.readLong();
        unsigned long speed = line.readULong(),
                      init  = line.readULong();
        if(speed == 0L) speed = HALF_MILLISECOND;
        stp.stepBy(delta, speed, init);
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
        int fileID = line.readInt();
        if(fileID > 0){
          File &f = sdcard::open(fileID);
          Serial.print("Opening ");
          Serial.println(f.name());
          
          // TODO execute content
          processFile(f);
          
          f.close(); // we can close it now
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
      idleCallback(0);
      idleCallback = NULL;
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

