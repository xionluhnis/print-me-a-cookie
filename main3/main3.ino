
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

#define NUM_STEPPERS 2
Stepper *steppers[NUM_STEPPERS];

void setup() {
  steppers[0] = &stpE0;
  steppers[1] = &stpY;
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->setup();
  }
  Serial.begin(9600); //Open Serial connection for debugging

  // sd card setup
  sdcard::begin();
}


// react to external input (non-user)
void react() {

}

// process events
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

// check if idle
boolean idle() {
  for(int i = 0; i < NUM_STEPPERS; ++i){
    if(steppers[i]->isRunning()) return false;
  }
  return true;
}

// reset al steppers and the error state
void resetAll(){
  for(int i = 0; i < NUM_STEPPERS; ++i)
    steppers[i]->reset();
  error = ERR_NONE; // clean flag
  Serial.println("Reset.");
}

// process user input
void readCommands(Stream& input = Serial){
  while(input.available() && error <= ERR_NONE){
    // command type depends on first character
    char type = input.read();
    switch(type){
      
      // --- sX where X = pin delta speed init
      case 's': {
        if(input.available()){
          int pin = readInt(input);
          unsigned long moves = readLong(input);
          unsigned long steps = readLong(input);
          unsigned long count = readLong(input);
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
        if(input.available()){
          int pin = readInt(input);
          if(pin < 0 || pin >= NUM_STEPPERS){
            error = ERR_INPUT;
            break;
          }
          steppers[pin]->microstep(type == '+' ? Stepper::MS_FULL : Stepper::MS_1_16);
        }
      } break;

      // --- moveby dx dy dz sx [sy sz ix iy iz]
      case 'm': {
        long dx = readLong(input),
             dy = readLong(input),
             dz = readLong(input);
        unsigned long sx = readULong(input),
                      sy = readULong(input),
                      sz = readULong(input),
                      ix = readULong(input),
                      iy = readULong(input),
                      iz = readULong(input);
        if(sx == 0L) sx = HALF_MILLISECOND;
        if(sy == 0L) sy = sx;
        if(sz == 0L) sz = sx;
        // if(dx) stpX.stepBy(dx, sx, ix);
        if(dy) stpY.stepBy(dy, sy, iy);
        // if(dz) stpZ.stepBy(dz, sz, iz);
      } break;

      // --- moveby dy sy iy
      case 'Y':
      case 'y': {
        if(type == 'y'){
          stpY.microstep(HIGH);
        } else {
          stpY.microstep(LOW);
        }
        long delta = readLong(input);
        unsigned long speed = readULong(input),
                      init  = readULong(input);
        if(speed == 0L) speed = HALF_MILLISECOND;
        if(delta) stpY.stepBy(delta, speed, init);
      }

      // extrude delta steps init eid
      case 'E':
      case 'e': {
        if(type == 'e'){
          stpE0.microstep(HIGH);
        } else {
          stpE0.microstep(LOW);
        }
        long delta = -readLong(input);
        unsigned long speed = readULong(input),
                      init  = readULong(input);
        if(speed == 0L) speed = HALF_MILLISECOND;
        int id = readInt();
        if(id == 0) stpE0.stepBy(delta, speed, init);
        // if(id == 1) stpE1.stepBy(delta, speed, init);
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
        int fileID = readInt();
        if(fileID > 0){
          File &f = sdcard::open(fileID);
          Serial.print("Opening ");
          Serial.println(f.name());
          
          // TODO execute content
          
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
  // remove any extra
  Serial.flush();
}

// Main loop
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
  }
  if(idle()){
    Serial.println("Idle wait.");
    delay(1000);
  }
  readCommands();
}




