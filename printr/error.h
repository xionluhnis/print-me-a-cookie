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
  ERR_INVALID_STEPPER  = 7,
  ERR_INVALID_ACCESSOR = 8,
  ERR_INVALID_ROUNDING = 9,
  ERR_INVALID_SETTINGS = 10,
  ERR_INVALID_G_CODE   = 11,
  ERR_FILE_PROC_STATE  = 12,
  ERR_MAX_LISTENERS    = 13
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
    case ERR_MAX_LISTENERS:
      Serial.println("Too many listeners on event source.");
      break;
    case -1:
      return;
    default:
      Serial.println("Unknown error!");
      break;
  }
  error = -1;
}
