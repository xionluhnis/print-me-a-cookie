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
  ERR_INVALID_ACCESSOR = 8
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
    case -1:
      return;
    default:
      Serial.println("Unknown error!");
      break;
  }
  error = -1;
}
