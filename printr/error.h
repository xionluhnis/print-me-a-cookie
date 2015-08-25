#pragma once

#include "Arduino.h"

// errors
enum ErrorType {
  ERR_NONE  = 0,
  ERR_PARSE = 1,
  ERR_STATE = 2,
  ERR_INPUT = 3,
  ERR_FILE_UNAVAILABLE = 4,
  ERR_CMD_UNSUPPORTED  = 5,
  ERR_INVALID_MS_MODE  = 6,
  ERR_INVALID_MS_STEPS = 7,
  ERR_INVALID_STEPPER  = 8,
  ERR_INVALID_ACCESSOR = 9,
  ERR_INVALID_ROUNDING = 10,
  ERR_INVALID_SETTINGS = 11,
  ERR_INVALID_G_CODE   = 12,
  ERR_FILE_PROC_STATE  = 13,
  ERR_BOUNDARY_TYPE    = 14,
  ERR_MISSING_RANGE    = 15,
  ERR_INVALID_DELTA_F  = 16
};

int error;

void logError() {
  switch(error){
    case 0:
      return;
    case ERR_PARSE:
      Serial.println("Parse error!");
      break;
    case ERR_STATE:
      Serial.println("Invalid state!");
      break;
    case ERR_INPUT:
      Serial.println("Invalid input!");
      break;
    case ERR_FILE_UNAVAILABLE:
      Serial.println("File unavailable!");
      break;
    case ERR_CMD_UNSUPPORTED:
      Serial.println("Command not supported!");
      break;
    case ERR_INVALID_MS_MODE:
      Serial.println("Wrong multistepping mode!");
      break;
    case ERR_INVALID_MS_STEPS:
      Serial.println("Wrong multistepping number!");
      break;
    case ERR_INVALID_STEPPER:
      Serial.println("Wrong stepper selection!");
      break;
    case ERR_INVALID_ACCESSOR:
    	Serial.println("Accessing data out of bound!");
    	break;
    case ERR_INVALID_ROUNDING:
    	Serial.println("Invalid rounding parameter!");
    	break;
    case ERR_INVALID_SETTINGS:
    	Serial.println("Invalid setting command!");
    	break;
    case ERR_INVALID_G_CODE:
      Serial.println("Invalid GCode command!");
      break;
    case ERR_FILE_PROC_STATE:
      Serial.println("File processing state is not recognized.");
      break;
    case ERR_BOUNDARY_TYPE:
      Serial.println("Unsupported boundary event.");
      break;
    case ERR_MISSING_RANGE:
      Serial.println("Cannot calibrate without stepper range values.");
      break;
    case ERR_INVALID_DELTA_F:
      Serial.println("Cannot have a null delta frequency!");
      break;
    case -1:
      return;
    default:
      Serial.println("Unknown error!");
      break;
  }
  error = -1;
}


