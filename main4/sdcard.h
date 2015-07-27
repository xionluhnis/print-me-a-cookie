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

/*
  Sd2Card card;
  SdVolume volume;
  SdFile root;
  const int chipSelect = 4;  
  void test(){
    // we'll use the initialization code from the utility libraries
    // since we're just testing if the card is working!
    if (!card.init(SPI_HALF_SPEED, chipSelect)) {
      Serial.println("initialization failed. Things to check:");
      Serial.println("* is a card is inserted?");
      Serial.println("* Is your wiring correct?");
      Serial.println("* did you change the chipSelect pin to match your shield or module?");
      return;
    } 
    
    // print the type of card
    Serial.print("\nCard type: ");
    switch(card.type()) {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        Serial.println("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("Unknown");
    }
  
    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) {
      Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
      return;
    }
  
  
    // print the type and size of the first FAT-type volume
    uint32_t volumesize;
    Serial.print("\nVolume type is FAT");
    Serial.println(volume.fatType(), DEC);
    Serial.println();
    
    volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
    volumesize *= volume.clusterCount();       // we'll have a lot of clusters
    volumesize *= 512;                            // SD card blocks are always 512 bytes
    Serial.print("Volume size (bytes): ");
    Serial.println(volumesize);
    Serial.print("Volume size (Kbytes): ");
    volumesize /= 1024;
    Serial.println(volumesize);
    Serial.print("Volume size (Mbytes): ");
    volumesize /= 1024;
    Serial.println(volumesize);
  
    
    Serial.println("\nFiles found on the card (name, date and size in bytes): ");
    root.openRoot(volume);
    
    // list all files in the card with date and size
    root.ls(LS_R | LS_DATE | LS_SIZE);
  }
  */
  void test() {
    // reimplement
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

