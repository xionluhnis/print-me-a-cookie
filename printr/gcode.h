#pragma once

#include "Arduino.h"
#include "parser.h"
#include "utils.h"
// location and steppers
#include "locator.h"
#include "elevator.h"
#include "stepper.h"

namespace gcode {

  char toupper(char c){
    if(c >= 'a' && c <= 'z')
      return c - ('a' - 'A');
    // already uppercase
    return c;
  }

  bool isValidField(char c){
    // valid field chars
    static const char *fieldChars = "GMTSPXYZIJDHFRQEAN*";
    
    c = toupper(c);
    for(int i = 0; fieldChars[i]; ++i){
      if(c == fieldChars[i]){
        return true;
      }
    }
    return false;
  }

  struct Field {
    char code;
    float value;

    template <typename T>
    Field(char c, T v) : code(c), value(v) {}

    operator bool() const {
      return code && isValidField(code);
    }
    bool operator !() const {
      return code == 0 || !isValidField(code);
    }
  };
  
  class CommandReader {
  public:
  
    CommandReader(Stream &s, Locator *xy, Elevator *z, float f = 1000.0) : input(&s), line(&s), locXY(xy), locZ(z), scale(f) {}

    bool available(){
      return input->available();
    }
    void next(){
      // start new line parser
      line = LineParser(*input);

      // currently parsed command
      Field command, firstCommand;
      Field field;
      while(line.available() && field = readField()){
        switch(field.code){
          
          // implicit movement command
          case 'X': hasX = true; X = convertToUnit(field.value); if(!command) command = Field('G', G); break;
          case 'Y': hasY = true; Y = convertToUnit(field.value); if(!command) command = Field('G', G); break;
          case 'Z': hasZ = true; Z = convertToUnit(field.value); if(!command) command = Field('G', G); break;
          case 'A': hasA = true; A = convertToUnit(field.value); if(!command) command = Field('G', G); break;
          case 'E': hasE = true; E = convertToUnit(field.value); if(!command) command = Field('G', G); break;
          case 'F': hasF = true; F = convertToUnit(field.value); if(!command) command = Field('G', G); break;
          
          // move command
          case 'G':
          // modal command
          case 'M': {
            if(command){
              execCommand(command);
            }
            command = field;
          } break;

          // parameters
          case 'P': P = field.value; break;
          case 'S': S = field.value; break;

          // otherwise we ignore
          default:
            // let's just forget about it
            break;
        }
      }
      // execute pending
      if(command){
        execCommand(command);
      }
    }

    long convertToUnit(float value) const {
      return (long)std::round(value * scale);
    }

  protected:
    void execCommand(const Field &command){
      int id = int(command.value);
      switch(command.type){
        case 'G':
          execMoveCommand(id);
          G = id; // store this command
          break;
        case 'M':
          execModalCommand(id);
          break;
        default:
          error = ERR_INVALID_G_CODE;
          return;
      }
      hasX = hasY = hasZ = hasA = hasE = hasF = false;
      P = S = 0L;
    }
    void execMoveCommand(int id){
      switch(id){
        // --- rapid linear movement
        case 0:
        // --- linear movement
        case 1: {
          if(hasZ){
            long curZ = locZ->target();
            if(absolute && curZ != Z){
              locZ->setTarget(Z);
            } else if(!absolute && Z){
              locZ->setTarget(curZ + Z);
            }
          }
          if(hasX || hasY){
            vec2 xy = locXY->target();
            if(absolute && ((hasX && xy.x != X) || (hasY && xy.y != Y))){
              locXY->setTarget(vec2(X, Y));
            } else if(!absolute && ((hasX && X) || (hasY && Y))){
              locXY->setTarget(xy + vec2(hasX ? X : 0, hasY ? Y : 0));
            }
          }
        } break;

        // --- rotation (NOT SUPPORTED)
        case 2:
        case 3: {
          
        } break;

        // --- dwelling
        case 4: {
          // wait depending on P and S
          if(P){
            if(P < 15L){
              delayMicroseconds(P * 1000);
            } else {
              delay(P); // milliseconds
            }
          } else if(S){
            delay(S * 1000); // seconds
          }
        } break;

        // --- move to origin
        case 28: {
          // TODO implement homing
          // 1 = hit bumpers for hasX, hasY and hasZ
          // 2 = move by some margin
          // 3 = go to reset zero position (from hitting the bumper)
          // => recalibrate positioning
          Serial.println("Homing not implemented yet!");
        } break;

        // --- absolute / relative positioning
        case 90: absolute = true; break;
        case 91: absolute = false; break;
        // --- origin reset
        case 92: {
          if(!hasX && !hasY && !hasZ && !hasE){
            locXY->resetX(0L);
            locXY->resetY(0L);
            locZ->resetZ(0L);
          } else {
            if(hasX){
              locXY->resetX(X);
            }
            if(hasY){
              locXY->resetY(Y);
            }
            if(hasZ){
              locZ->resetZ(Z);
            }
            if(hasE){
              // TODO reset virtual extrusion level
            }
          }
        } break;
      }
    }
    void execModalCommand(int id){
      
    }
    Field readField() {
      Field field;
      field.code = toupper(line.readFullChar());
      field.value = 0.0;
      if(!field.code)
        return field;
      if(field.code == ';'){
        line.skip();
        return field;
      }
      if(!line.available())
        return field;
      char n = line.peekFull();
      if(n == '-' || isDigit(n)){
        field.value = line.readFloat();
      }
      return field;
    }
  
  private:
    // parser
    Stream *input;
    LineParser line;
    
    // positioning system
    Locator *locXY;
    Elevator *locZ;

    // positioning state
    int G;
    long X, Y, Z, A, E, F;
    bool hasX, hasY, hasZ, hasA, hasE, hasF;
    bool absolute;
    // extra parameters
    long P, S;

    // parameters
    float scale;
    
  };

  

}
