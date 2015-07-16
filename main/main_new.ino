
/****************************************************************************** 
SparkFun Big Easy Driver Basic Demo
Toni Klopfenstein @ SparkFun Electronics
February 2015
https://github.com/sparkfun/Big_Easy_Driver

Simple demo sketch to demonstrate how 5 digital pins can drive a bipolar stepper motor,
using the Big Easy Driver (https://www.sparkfun.com/products/12859). Also shows the ability to change
microstep size, and direction of motor movement.

Development environment specifics:
Written in Arduino 1.6.0

This code is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!
Distributed as-is; no warranty is given.

Example based off of demos by Brian Schmalz (designer of the Big Easy Driver).
http://www.schmalzhaus.com/EasyDriver/Examples/EasyDriverExamples.html
******************************************************************************/
#include "types.h"
#include "stepper.h"

// errors
#define ERR_NONE  0
#define ERR_PARSE 1
int error;

// delays in milliseconds
#define DELAY_INTER 50
#define DELAY_AFTER 30
#define SECOND      (1000L/80L)

// steppers
Stepper stpE0(2, 3, 4, 5, 6, 7);
Stepper stpY(8, 9, 10, 11, 12, 13);

#define NUM_STEPPERS 2
Stepper *steppers[NUM_STEPPERS];

void setup() {
  steppers[0] = &stpE0;
  steppers[1] = &stpY;
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->setup();
  }
  Serial.begin(9600); //Open Serial connection for debugging
}


// react to external input (non-user)
void react() {

}

// process events
#define STATE_IDLE 1
#define STATE_BUSY 0

byte process() {
  byte state = STATE_IDLE;
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->exec();
  }
  delayMilliseconds(DELAY_INTER);
  for(int i = 0; i < NUM_PINS; ++i){
    steppers[i]->release();
    if(steppers[i]->isRunning()){
      state = STATE_BUSY;
    }
  }
  delayMilliseconds(DELAY_AFTER);
  return state;
}


void resetAll(){
  for(int i = 0; i < NUM_STEPPERS; ++i)
    steppers[i]->reset();
  error = ERR_NONE; // clean flag
  Serial.println("Reset.");
}

// process user input
void readCommands(){
  if(Serial.available()){
    // command type depends on first character
    char type = Serial.read();
    switch(type){
      // pX where X = pin1 t1 s1 c1 pin2 t2 s2 c2 ...
      case 'p': {
        while(Serial.available()){
          int pin = readInt();
          unsigned long moves = readLong();
          unsigned long steps = readLong();
          unsigned long count = readLong();
          if(steps == 0){
            steps = 1;
          }
          steppers[pin]->stepBy(moves, steps, count);
        }
      } break;
      // reset all processes (needed in case of error)
      case 'r':
      case 'R': {
        resetAll();
      } break;

      // moveby dx dy dz sx [sy sz ix iy iz]
      case 'm': {
        long dx = readLong(),
             dy = readLong(),
             dz = readLong();
        unsigned long sx = readULong(),
                      sy = readULong(),
                      sz = readULong(),
                      ix = readULong(),
                      iy = readULong(),
                      iz = readULong();
        if(sx == 0) sx = 1;
        if(sy == 0) sy = sx;
        if(sz == 0) sz = sx;
        // if(dx) stpX.stepBy(dx, sx, ix);
        if(dy) stpY.stepBy(dy, sy, iy);
        // if(dz) stpZ.stepBy(dz, sz, iz);
      } break;

      // extrudeBy total steps init id
      case 'e': {
        long delta = readLong();
        unsigned long speed = readULong(),
                      init  = readULong();
        int id = readInt();
        if(id == 0) stepBy(STP_E0, DIR_E0, delta, speed, init);
        // if(id == 1) stepBy(STP_E1, DIR_E1, delta, speed, init);
      } break;

      default:
        // otherwise
        break;
    }
  }
}

void logError() {
  switch(error){
    case 0:
      return;
    case 1:
      Serial.println("Parse error!");
      break;
    case 2:
      Serial.println("Invalid state!");
      break;
    case -1:
      return;
    default:
      Serial.println("Unknown error!");
      break;
  }
  error = -1;
}

// Main loop
void loop() {
  // show errors
  logError();

  // 1 = react to external events
  react();

  // 2 = process scheduled events
  byte state = STATE_IDLE;
  if(error == ERR_NONE){
    state = process();
  }

  // 3 = read user input
  if(state == STATE_IDLE){
    for(int i = 0; i < NUM_STEPPERS; ++i){
      steppers[i]->reset();
    }
    readCommands();
  }
}

//Default microstep mode function
void StepForwardDefault()
{
  Serial.println("Moving forward at default step mode.");
  stpE0.stepBy(1000L, SECOND);
  /* equivalent to:
  digitalWrite(DIR_E0, LOW); //Pull direction pin low to move "forward"
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  */
}

void StepForwardDefaultY()
{
  Serial.println("Moving Y forward at default step mode.");
  stpY.stepBy(200L, 2L * SECOND, 0L);
  /* equivalent to:
  digitalWrite(DIR_Y, LOW); //Pull direction pin low to move "forward"
  for(x= 1; x<200; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STP_Y,HIGH); //Trigger one step forward
    delay(2);
    digitalWrite(STP_Y,LOW); //Pull step pin low so it can be triggered again
    delay(2);
  }
  */
}

//Reverse default microstep mode function
void ReverseStepSlow()
{
  Serial.println("Moving in reverse at slow step mode.");
  stpE0.stepBy(-200L, 5L * SECOND, 0L);
  /* equivalent to:
  digitalWrite(DIR_E0, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<200; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step
    delay(5);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    delay(5);
  }
  */
}

//Reverse default microstep mode function long then forward to cut strand
void MS_E0_End(int){
  stpE0.microstep(LOW);
}
void MS_Y_End(int){
  stpY.microstep(LOW);
}

void CurrentTestMode()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  stpE0.microstep(HIGH);
  stpY.microstep(HIGH);

  // move
  stpE0.stepBy(-2000L, SECOND, 0L, MS_E0_End);
  stpY.stepBy(2000L, SECOND, 0L, MS_Y_End);
  /*
  digitalWrite(dir, HIGH); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  digitalWrite(DIR_Y, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1_Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2_Y, HIGH);
  digitalWrite(MS3_Y, HIGH);
  for(x= 1; x<2000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step forward
    digitalWrite(STP_Y,HIGH);
    delay(1);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    digitalWrite(STP_Y,LOW);
    delay(1);
  }
  */
}

void Danger()
{
  Serial.println("Moving in reverse at slow step mode.");
  stpE0.stepBy(-200L, SECOND / 2L, 0L);
  /*
  digitalWrite(dir, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<200; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step
    delayMicroseconds(500);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    delayMicroseconds(500);
  }
  */
}

void ReverseStepDefault()
{
  Serial.println("Moving in reverse at default step mode.");
  stpE0.stepBy(-1000L, SECOND, 0L);
  /*
  digitalWrite(dir, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<1000; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step
    delay(1);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  */
}

// 1/16th microstep foward mode function
void SmallStepMode()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  stpE0.microstep(HIGH);
  stpE0.stepBy(1000L, SECOND, 0L, MS_E0_End);
  /*
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  */
}

// 1/16th microstep foward mode function
void SmallStepY()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  stpY.microstep(HIGH);
  stpY.stepBy(1000L, SECOND, 0L, MS_Y_End);
  /*
  digitalWrite(DIR_Y, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1_Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2_Y, HIGH);
  digitalWrite(MS3_Y, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STP_Y,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP_Y,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  */
}

// 1/16th microstep foward mode function
void SmallStepReverseMode()
{
  Serial.println("Stepping at 1/16th microstep reverse mode.");
  stpE0.microstep(HIGH);
  stpE0.stepBy(-1000L, SECOND, 0L, MS_E0_End);
  /*
  digitalWrite(dir, HIGH); //Pull direction pin low to move "Reverse"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(STP_E0,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  */
}

//Forward/reverse stepping function
void ForwardBackwardStep(int i = 0)
{
  Serial.println("Alternate between stepping forward and reverse.");
  if(i >= 4) return; // done
  long dir = i % 2 ? -1L : 1L;
  stpE0.stepBy(dir * 1000L, SECOND, 0L, ForwardBackwardStep, i + 1);
  /*
  for(x= 1; x<5; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    //Read direction pin state and change it
    state=digitalRead(dir);
    if(state == HIGH)
    {
      digitalWrite(dir, LOW);
    }
    else if(state ==LOW)
    {
      digitalWrite(dir,HIGH);
    }
    
    for(y=1; y<1000; y++)
    {
      digitalWrite(STP_E0,HIGH); //Trigger one step
      delay(1);
      digitalWrite(STP_E0,LOW); //Pull step pin low so it can be triggered again
      delay(1);
    }
  }
  */
}

void DashedLineTest(int i = 0){
{
  //SmallStepReverseMode();
  switch(i){
    case 0:
      // microstep mode
      stpE0.microstep(HIGH);
      stpY.microstep(HIGH);
    case 1:
    case 2:
    case 10:
    case 11:
    case 12:
      // move
      stpE0.stepBy(-2000L, SECOND, 0L);
      stpY.stepBy(2000L, SECOND, 0L, DashedLinetest, i + 1);
      break;
    case 3:
    case 4:
    case 13:
    case 14:
    case 15:
      stpE0.stepBy(1000L, SECOND, 0L, DashedLinetest, i + 1);
      break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 16:
      stpY.stepBy(1000L, SECOND, 0L, DashedLinetest, i + 1);
      break;
    case 9:
      stpE0.stepBy(-1000L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 17:
      // disable microstepping
      stpE0.microstep(LOW);
      stpE1.microstep(LOW);
      // done
      Serial.println("Done with dashed line test.");
      return;
    default:
      error = -2;
      break;
  }
  /*
  CurrentTestMode();
  CurrentTestMode();
  CurrentTestMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepY();
  SmallStepY();
  SmallStepY();
  SmallStepY();
  SmallStepReverseMode();
  CurrentTestMode();
  CurrentTestMode();
  CurrentTestMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepY();
  */
}

void DottedLineTest(int i = 0)
{
  switch(i){
    case 0:
      stpE0.microstep(HIGH);
      stpY.microstep(HIGH);
      stpE0.stepBy(-1000L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 1:
      stpE0.stepBy(-2000L, SECOND, 0L);
      stpY.stepBy(2000L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 2:
    case 3:
    case 4:
      stp
  }
  /*
  SmallStepReverseMode();
  CurrentTestMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepY();
   SmallStepReverseMode();
  CurrentTestMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepY();
  SmallStepY();
   SmallStepReverseMode();
  CurrentTestMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepMode();
  SmallStepY();
  */
}
