#pragma once

#include "Arduino.h"
#include <SPI.h>
#include <SD.h>

namespace sdcard {

  // state
  File dir, file;

  void begin() {
    pinMode(SS, OUTPUT);
    SD.begin();
  }

    
  File & currentFile(){
    return file;
  }
  
  void test() {
    // TODO implement this
  }
  
  void list() {
    if(!dir){
      dir = SD.open("/");
      if(!dir){
        Serial.println("Cannot open root directory!");
        return;
      }
    }
    int id = 1;
    while(file = dir.openNextFile()){
      // continue
      if(!file.isDirectory()){
        // show file information
        Serial.print("[");
        Serial.print(id);
        Serial.print("] ");
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(file.size(), DEC);
        Serial.println(")");
        ++id;
      }
      file.close();
    }
    Serial.println("** EOF **");
    dir.rewindDirectory();
    dir.close();
  }
  
  File & open(int fileID = 0){
    if(!dir){
      dir = SD.open("/");
      if(!dir){
        Serial.println("Cannot open root directory!");
        return currentFile();
      }
    }
    while(fileID){
      file.close(); // close previous one
      file = dir.openNextFile(); // open next one
      if(!file){
        Serial.print("File ");
        Serial.print(fileID);
        Serial.println(" does not exist.");
        break;
      } else if(!file.isDirectory()){
        --fileID;
      }
    }
    dir.rewindDirectory();
    dir.close();
    return currentFile();
  }

}

