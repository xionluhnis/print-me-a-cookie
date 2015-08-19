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

    Field() : code('\0'), value(0.0) {}

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

    CommandReader() : input(NULL), locXY(NULL), locZ(NULL), stpE(NULL) {}
    CommandReader(Stream &s, Locator *xy, Elevator *z, Stepper *e, float f = 1000.0) : input(&s), line(s), locXY(xy), locZ(z), stpE(e), scale(f), metric(true) {
      X = Y = Z = A = E = F = P = S = 0;  
    }
    

    bool available(){
      return input && input->available();
    }
    void next(){
      bool idle = true;
      while(input->available() && idle){
        // start new line parser
        line = LineParser(*input);

        Serial.print(". ");
  
        // currently parsed command
        Field command;
        while(line.available()){
          Field field = readField();
          if(!field) break;
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
                bool res = execCommand(command);
                if(res) idle = false;
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
          bool res = execCommand(command);
          if(res) idle = false;
        }
      }
    }

    long convertToUnit(float value) const {
      float factor = metric ? 1.0 : 25.4;
      float mmToSteps = 5000.0 / 56.0;
      return (long)std::round(factor * value * scale * mmToSteps); 
    }

  protected:
    bool execCommand(const Field &command){
      Serial.print('exec '); Serial.print(command.code); Serial.println(int(command.value), DEC);
      int id = int(command.value);
      bool res = false;
      switch(command.code){
        case 'G':
          res = execMoveCommand(id);
          G = id; // store this command
          break;
        case 'M':
          res = execModalCommand(id);
          break;
        default:
          error = ERR_INVALID_G_CODE;
          return false;
      }
      hasX = hasY = hasZ = hasA = hasE = hasF = false;
      P = S = 0L;
      return res;
    }
    bool execMoveCommand(int id){
      switch(id){
        // --- rapid linear movement
        case 0:
        // --- linear movement
        case 1: {
          // extrusion
          if(hasE){
            lastE = E; // relative extrusion level
            if(E == 0L){
              stpE->moveToFreq(0L);
            } else if(E > 0){
              stpE->moveToFreq(10L);
            } else {
              stpE->moveToFreq(-10L);
            }
          } else if(hasA){
            long dE = lastE - A; // relative extrusion level
            lastE = A; // absolute extrusion level
            if(dE == 0L){
              stpE->moveToFreq(0L);
            } else if(dE > 0){
              stpE->moveToFreq(10L);
            } else {
              stpE->moveToFreq(-10L);
            }
          } else if(A){
            // stop extrusion?
            stpE->moveToFreq(Stepper::IDLE_FREQ);
          }

          // speed
          if(hasF){
            // TODO change XYZ speed accordingly
          }
          
          // movement
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
        } return hasX || hasY || hasZ;

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

        // --- metric system
        case 20: metric = false; break; // set to inches
        case 21: metric = true; break; // set to millimeters

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
              // reset virtual extrusion level
              lastE = E = 0L;
            }
          }
        } break;
      }
      return false;
    }
    bool execModalCommand(int id){
      return false;
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
      char n = line.fullPeek();
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
    Stepper *stpE;

    // positioning state
    int G;
    long X, Y, Z, A, E, F;
    bool hasX, hasY, hasZ, hasA, hasE, hasF;
    long lastE;
    bool absolute, metric;
    // extra parameters
    long P, S;

    // parameters
    float scale;
    
  };

  

}
