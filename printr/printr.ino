
#include <SPI.h>
#include <SD.h>
#include "sdcard.h"
#include "error.h"
#include "parser.h"
#include "stepper.h"
#include "locator.h"
#include "elevator.h"
#include "gcode.h"

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
Stepper stpE0(2, 3, 4, 5, 6, 7, 'e');
Stepper stpY(8, 9, 10, 11, 12, 13, 'y');
Stepper stpZ(22, 23, 24, 25, 26, 27, 'z');
Stepper stpX(28, 29, 30, 31, 32, 33, 'x', HIGH); // /!\ reversed X

#define NUM_STEPPERS 4
Stepper *steppers[NUM_STEPPERS];

// position system
Locator locXY(&stpX, &stpY);
Elevator locZ(&stpZ);

// callback type
typedef void (*Callback)(int state);

// global callbacks
Callback idleCallback = NULL;    // called when idle
Callback errorCallback = NULL;   // called when an error occurs
Callback switchCallback = NULL;  // called upon hitting a switch

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
void processFile(File &file, bool gcode, float scale);
void calibrateHome(Callback cb, int switchEvent);

////////////////////////////////////////////////////////////////
///// Setup Arduino ////////////////////////////////////////////
////////////////////////////////////////////////////////////////
void setup() {
  steppers[0] = &stpE0;
  steppers[1] = &stpY;
  steppers[2] = &stpZ;
  steppers[3] = &stpX;
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->setup();
  }
  Serial.begin(250000); // Open Serial connection for debugging

  // sd card setup
  sdcard::begin();

  // axis ranges
  stpX.setRange(28067UL);
  stpY.setRange(13693UL);
  stpZ.setRange(122100UL); // to be set at print time to be close to plate

  // global callbacks
  idleCallback = errorCallback = NULL;
  // switchCallback = resetToHome;
}

////////////////////////////////////////////////////////////////
///// React to external input //////////////////////////////////
////////////////////////////////////////////////////////////////
#define SWITCH_FIRST 11
#define SWITCH_X_MIN 11
#define SWITCH_X_MAX 12
#define SWITCH_Y_MIN 13
#define SWITCH_Y_MAX 14
#define SWITCH_Z_MIN 15
#define SWITCH_LAST  15
#define NUM_SWITCHES (SWITCH_LAST - SWITCH_FIRST + 1)
// buffer for time delaying checks
long lastSwitchCheck[NUM_SWITCHES];
int switchThreshold = 0;
bool switchDebug = false;
void react() {
  long thisTime = millis();
  for(int i = SWITCH_FIRST; i <= SWITCH_LAST; ++i){
    if(thisTime - lastSwitchCheck[i - SWITCH_FIRST] < 300L){
      continue;
    }
    // /!\ the switches randomly generate low values from time to time
    //  => cannot fully be trusted unless the value is high enough
    int s = analogRead(i);
    if(s > switchThreshold){
      lastSwitchCheck[i - SWITCH_FIRST] = thisTime; // remember time so we don't check too soon again
      switch(i){
        case SWITCH_X_MIN:
          stpX.setMinValue(stpX.value());
          break;
        case SWITCH_X_MAX:
          stpX.setMaxValue(stpX.value());
          break;
        case SWITCH_Y_MIN:
          stpY.setMinValue(stpY.value());
          break;
        case SWITCH_Y_MAX:
          stpY.setMaxValue(stpY.value());
          break;
        case SWITCH_Z_MIN:
          stpZ.setMinValue(stpZ.value());
          break;
        default:
          Serial.println("AnalogRead: invalid entry!");
          continue;
      }
      if(switchCallback){
        switchCallback(i);
      }
      if(switchDebug){
        Serial.print("Switch#"); Serial.print(i, DEC); Serial.print(" ~"); Serial.println(s, DEC);
      }
    }
  }
}

////////////////////////////////////////////////////////////////
///// Process events ///////////////////////////////////////////
////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////
///// Check if idle ////////////////////////////////////////////
////////////////////////////////////////////////////////////////
bool idle() {
  for(int i = 0; i < NUM_STEPPERS; ++i){
    if(steppers[i]->isRunning()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////
///// Reset states and errors //////////////////////////////////
////////////////////////////////////////////////////////////////
void resetAll(int state = 0){
  for(int i = 0; i < NUM_STEPPERS; ++i)
    steppers[i]->reset();
  locXY.reset();
  locZ.reset();
  error = ERR_NONE; // clean flag
  // remove callbacks
  idleCallback = NULL;
  errorCallback = NULL;
  // switchCallback = resetAll;
  Serial.println("Reset.");
}
void setHome(int){
  long minX, minY;
  stpX.resetPosition(0L); minX = stpX.minValue(); stpX.reset(); stpX.setMinValue(minX);
  stpY.resetPosition(0L); minY = stpY.minValue(); stpY.reset(); stpY.setMinValue(minY);
  locXY.reset();
}
void resetToHome(int switchEvent) {
  // 1 = reset everything
  resetAll();

  // 2 = go to center (while trusting the stepper positions)
  calibrateHome(setHome, switchEvent);
}

////////////////////////////////////////////////////////////////
///// Process commands /////////////////////////////////////////
////////////////////////////////////////////////////////////////
void readCommands(Stream& input){
  while(input.available() && error <= ERR_NONE){

    // full line parser
    LineParser line(input);
    // create a command parser (subline)
    LineParser command = line.subline();
  
    // command type depends on first character
    char type = command.readChar();
    Serial.print("[");
    Serial.print(type);
    Serial.print("] size=");
    Serial.println(input.available(), DEC);
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
        char c = command.readFullChar();
        switch(c){
          case 'X':
          case 'x':
            stpX.reset();
            break;
          case 'Y':
          case 'y':
            stpY.reset();
            break;
          case 'Z':
          case 'z':
            stpZ.reset();
            break;
          case 'E':
          case 'e':
            stpE0.reset();
            break;
          case 'M':
          case 'm':
            locXY.reset();
            break;
          case 'H':
          case 'h':
            locZ.reset();
            break;
          default:
            resetAll();
            break;
        }
      } break;

      // --- disable switch on steppers
      case '-': {
        for(int i = 0; i < NUM_STEPPERS; ++i){
          if(!steppers[i]->isRunning() && steppers[i]->isEnabled()){
            steppers[i]->disable();
            Serial.print("Disabling pin ");
            Serial.println(i, DEC);
          }
        }
      } break;

      // --- toggle systems on/off
      case '*': {
        char c = command.readFullChar();
        if(c == 'm' || c == 'M'){
          locXY.toggle();
        } else if(c == 'h' || c == 'H'){
          locZ.toggle();
        }
      } break;

      // --- debug pin
      case 'D':
      case 'd': {
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
            stp->debug();
          } break;

          case 'M':
          case 'm':
            locXY.debug();
            break;

          case 'H':
          case 'h':
            locZ.debug();
            break;

          default:
            Serial.print("Cannot debug '");
            Serial.print(c);
            Serial.println("'");
            break;
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
      // --- transitto x y
      // --- transitby dx dy
      case 'M':
      case 'm':
      case 'T':
      case 't': {
        long x = command.readLong();
        long y = command.readLong();
        vec2 p(x, y);
        Serial.print(type); Serial.print(" x="); Serial.print(p.x, DEC); Serial.print(", y="); Serial.println(p.y, DEC);
        if(type == 'm' || type == 't'){
          p += locXY.target(); // relative to absolute
        }
        locXY.setTarget(p, type == 'M' || type == 'm');
      } return; // release input reading

      // --- elevateto z
      // --- elevateby dz
      case 'H':
      case 'h': {
        long z = command.readLong();
        if(type == 'h') z += locZ.target(); // relative to absolute
        Serial.print("H "); Serial.println(z, DEC);
        locZ.setTarget(z);
      } return; // release input reading
      
      // --- extrude period
      case 'X':
      case 'x':
      case 'Y':
      case 'y':
      case 'Z':
      case 'z':
      case 'E':
      case 'e': {
        Stepper *stp = selectStepper(type);
        long freq = command.readLong();
        Serial.print(type); Serial.print(" "); Serial.println(freq, DEC);
        stp->moveToFreq(freq);
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
            if(c1 == 'd' || c1 == 'D'){
              char c2 = command.fullPeek();
              if(isDigit(c2)){
                stp->setDebugMode(command.readInt());
              }else if(c2 == 'f'){
                command.readChar(); // consume it
                stp->setDeltaFreq(command.readULong());
              } else {
                error = ERR_INVALID_SETTINGS;
                return;
              }
            } else {
              char c2 = command.readChar();
              if(c1 == 'f' && c2 == 's'){
                stp->setSafeFreq(command.readULong());
              } else if(c1 == 'r' && c2 == 'g'){
                unsigned long range = command.readULong();
                if(range){
                  stp->setRange(range);
                }
                Serial.print("Range of "); 
                Serial.print(c);
                Serial.print(": ");
                Serial.println(stp->range(), DEC);
              } else {
                error = ERR_INVALID_SETTINGS;
                return;
              }
            }
          } break;
          
          // - xy location settings
          case 'M':
          case 'm': {
            char c1 = command.readFullChar();
            if(c1 == 'd'){
              char c2 = command.fullPeek();
              if(isDigit(c2)){
                locXY.setDebugMode(command.readInt());
              } else if(c2 == 'f') {
                c2 = command.readChar(); // consume it
                locXY.setMaxDeltaFreq(command.readULong());
              } else {
                error = ERR_INVALID_SETTINGS;
                return;
              }
            } else {
              char c2 = command.readChar();
              if(c1 == 'f' && c2 == 'b'){
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
            }
          } break;
          
          // - z location settings
          case 'H':
          case 'h': {
            char c1 = command.readFullChar();
            if(c1 == 'd' || c1 == 'D'){
              char c2 = command.fullPeek();
              if(isDigit(c2)){
                locZ.setDebugMode(command.readInt());
              } else if(c2 == 'f'){
                c2 = command.readChar(); // consume it
                locZ.setMaxDeltaFreq(command.readULong());
              } else {
                error = ERR_INVALID_SETTINGS;
                return;
              }
            } else {
              char c2 = command.readChar();
              if(c1 == 'f' && c2 == 'b'){
                locZ.setBestFreq(command.readULong());
              } else {
                error = ERR_INVALID_SETTINGS;
                return;
              }
            }
          } break;

          // - switch settings
          case 'S':
          case 's': {
            char c1 = command.readFullChar();
            if(c1 == 't' || c1 == 'T'){
              switchThreshold = command.readInt();
              Serial.print("New switch threshold: ");
              Serial.println(switchThreshold, DEC);
            } else if(c1 == 'd' || c1 == 'D'){
              switchDebug = !!command.readInt();
            } else {
              error = ERR_INVALID_SETTINGS;
            }
          } break;

          // - gcode settings
          case 'G':
          case 'g': {
            char c1 = command.readFullChar();
            if(c1 == 'd' || c1 == 'D'){
              gcode::debug = !!command.readInt();
            } else {
              error = ERR_INVALID_SETTINGS;
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

      // --- calibrate / homing
      case 'C':
      case 'c':
        calibrateHome(NULL, 0);
        break;

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

      case 'G':
      case 'g': {
        char c = command.fullPeek();
        if(c == 'G' || c == 'g'){
          command.readChar();
          int fileID = command.readInt();
          float scale = command.readFloat();
          if(scale == 0.0){
            scale = 1.0;
          }
          if(fileID > 0){
            File &f = sdcard::open(fileID);
            gcode::CommandReader gcode(f, &locXY, &locZ, &stpE0, scale);
            gcode::Description desc = gcode.simulate();
            Serial.print("--- Simulation (scale="); Serial.print(scale, DEC); Serial.println(") ---");
            Serial.print("BBox: "); Serial.print(desc.min.x, DEC); Serial.print(", "); Serial.print(desc.min.y, DEC);
            Serial.print(" to "); Serial.print(desc.max.x, DEC); Serial.print(", "); Serial.println(desc.max.y, DEC);
            Serial.print("Path from "); Serial.print(desc.start.x, DEC); Serial.print(", "); Serial.print(desc.start.y, DEC);
            Serial.print(" to "); Serial.print(desc.end.x, DEC); Serial.print(", "); Serial.println(desc.end.y, DEC);
            Serial.println("------");
          }
          break;
        } else if(c == 'r' || c == 'R'){
          command.readChar();
          gcode::CommandReader gcode(input, &locXY, &locZ, &stpE0, 1.0);
          gcode.next();
          break;
        }
      }
      case 'O':
      case 'o': {
        // open and execute file
        int fileID = command.readInt();
        float scale = command.readFloat();
        if(scale == 0.0){
          scale = 1.0;
        }
        if(fileID > 0){
          File &f = sdcard::open(fileID);
          Serial.print("Opening ");
          Serial.println(f.name());
          
          // execute content
          processFile(f, type == 'g' || type == 'G', scale);
          
        } else {
          Serial.println("File ID must be strictly positive.");
        }
      } break;

      default:
        // otherwise
        break;
    }
  }
 // Serial.println("Read command.");
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

////////////////////////////////////////////////////////////////
///// Processing loop //////////////////////////////////////////
////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////
///// File processing //////////////////////////////////////////
////////////////////////////////////////////////////////////////
gcode::CommandReader gcodeReader;
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
    locXY.setCallback(NULL);
    locZ.setCallback(NULL);
    return;
  }
  // idleCallback = processNextLine; // trigger again for next line on next idle time
  switch(state){
    case 0: // normal commands
      readCommands(file);
      break;
    case 1: // gcode commands
      gcodeReader.next();
      break;
    default:
      error = ERR_FILE_PROC_STATE;
      return;
  }
}

void processFile(File &file, bool gcode, float scale){
  if(!file){
    Serial.println("No file to process!");
    file.close();
    return;
  }

  // process file lines one by one
  // => set callbacks and states
  errorCallback = processFileError;
  locXY.setCallback(processNextLine); locXY.setState(gcode ? 1 : 0);
  // locZ.setCallback(processNextLine); locZ.setState(gcode ? 1 : 0);
  // initialize potential gcode reader
  if(gcode){
    gcodeReader = gcode::CommandReader(file, &locXY, &locZ, &stpE0, scale);
  }

  // read first line
  processNextLine(gcode ? 1 : 0);
}

////////////////////////////////////////////////////////////////
///// Homing ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
int homeStatus;
Callback homeCallback;
void homeCheckEvent(int which){
  homeStatus |= which;
  // Serial.print("Home reached "); Serial.println(which);
  if(homeStatus == B11){
    Serial.println("Home!");
    locXY.debug(); stpX.debug(); stpY.debug();
    locZ.debug(); stpZ.debug();
    stpZ.microstep(Stepper::MS_1_16);
    if(homeCallback){
      Callback cb = homeCallback;
      homeCallback = NULL;
      cb(which);
    }
  }
}
void boundaryEvent(int which){
  if(switchDebug){
    Serial.print("Hit boundary "); Serial.println(which, DEC);
  }
  switch(which){
    case SWITCH_X_MAX:
      homeStatus |= 1 << 0;
      break;
    case SWITCH_Y_MAX:
      homeStatus |= 1 << 1;
      break;
    case SWITCH_Z_MIN:
      homeStatus |= 1 << 2;
      break;
    default:
      //error = ERR_BOUNDARY_TYPE;
      return;
  }
  // if all switches have been hit, we're done
  if((homeStatus & B111) == B111){
    homeStatus = 0; // reset homing status
    switchCallback = NULL; // prevent more boundaryEvent calls
    // renable XYZ
    // TODO move to the correct location!
    vec2 homeLoc(stpX.minValue() + stpX.range() / 2L, stpY.minValue() + stpY.range() / 2L);
    locXY.enable(); locXY.setState(1 << 0);  locXY.setTarget(homeLoc);
    locZ.enable();  locZ.setState(1 << 1);   locZ.setTarget(stpZ.maxValue());
    stpZ.microstep(Stepper::MS_1_2); // to go at max frequency using H
    // set callbacks
    locXY.setCallback(homeCheckEvent);
    locZ.setCallback(homeCheckEvent);
    // homeCheckEvent(1 << 1);
  }
  /* TODO this creates a strong noise, so we keep hitting the bumpers instead
  else if(homeStatus == B11){
    Serial.println("shifting");
    homeStatus |= 1 << 3; // mask for this specific event
    // change direction
    stpX.moveToFreq(-3L);
    stpY.moveToFreq(-3L);
    // shift to avoid switch events
    vec2 shift(stpX.maxValue() - 100L, stpY.maxValue() - 100L);
    locXY.enable();
    locXY.setTarget(shift);
    locXY.setCallback(NULL);
  }
  */
}
void calibrateHome(Callback cb, int switchEvent){
  homeStatus = 0;
  if(switchEvent > 0){
    // we can trust the positions
    // => we just move to home
    locXY.setTarget(vec2(0, 0));
    if(cb){
      locXY.setCallback(cb);
    }
  } else {
    if(!stpX.hasRange() || !stpY.hasRange() || !stpZ.hasRange()){
      error = ERR_MISSING_RANGE;
      return;
    }
    // set callbacks
    switchCallback = boundaryEvent;
    homeCallback = cb;
    // disable normal positiong
    locXY.disable();
    locZ.disable();
    // go to switches in both X and Y
    stpX.moveToFreq(1L);
    stpY.moveToFreq(2L);
    stpZ.microstep(Stepper::MS_1_2);
    stpZ.moveToFreq(-2L);
  }
}

