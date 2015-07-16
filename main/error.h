#pragma once

// errors
enum ErrorType {
  ERR_NONE  = 0,
  ERR_PARSE = 1,
  ERR_STATE = 2,
  ERR_INPUT = 3
}

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
    case -1:
      return;
    default:
      Serial.println("Unknown error!");
      break;
  }
  error = -1;
}
