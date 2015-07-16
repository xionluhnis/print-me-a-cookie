
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

//Declare pin functions on Arduino
// EXTRUDER 0
#define STP_E0  2
#define DIR_E0  3
#define MS1_E0  4
#define MS2_E0  5
#define MS3_E0  6
#define EN_E0   7
// EXTRUDER 1
#define STP_E1  -1
#define DIR_E1  -1
#define MS1_E1  -1
#define MS2_E1  -1
#define MS3_E1  -1
#define EN_E1   -1
// X AXIS STEPPER
#define STP_X   -1
#define DIR_X   -1
#define MS1_X   -1
#define MS2_X   -1
#define MS3_X   -1
#define EN_X    -1
// Y AXIS STEPPER
#define STP_Y   8
#define DIR_Y   9
#define MS1_Y   10
#define MS2_Y   11
#define MS3_Y   12
#define EN_Y    13
// Z AXIS STEPPER
#define STP_Z   -1
#define DIR_Z   -1
#define MS1_Z   -1
#define MS2_Z   -1
#define MS3_Z   -1
#define EN_Z    -1

// errors
#define ERR_NONE  0
#define ERR_PARSE 1
int error;

// delays in milliseconds
#define DELAY_INTER 50
#define DELAY_AFTER 30
#define SECOND      (1000L/80L)

// group sizes
#define NUM_PINS 54
#define NUM_OUTS (6 * 2)
#define NUM_ENS  5
// pin groups
int enPins[NUM_ENS];
int ouPins[NUM_OUTS];

typedef void (*Callback)();

// state data
struct PinProcess {
  unsigned long moves; // total number of action triggers
  unsigned long steps; // number of steps to trigger action
  unsigned long count; // step accumulation buffer
  int first;
  int second;
  int eid; // enable pin id
  Callback callback;
};

struct PinProcess processes[NUM_PINS];

byte isPending(int pin) {
  return processes[pin].moves > 0;
}
byte isEnabler(int pin) {
  for(int i = 0; i < NUM_ENS; ++i)
    if(enPins[i] == pin)
      return 1;
  return 0;
}

// declarations (not needed for Arduino)
void stepBy(int stpPin, int dirPin, long move, unsigned long speed, unsigned long init){

//Reset Big Easy Driver pins to default states
void resetBEDPins() {
  digitalWrite(STP_E0, LOW);
  digitalWrite(DIR_E0, LOW);
  digitalWrite(MS1_E0, LOW);
  digitalWrite(MS2_E0, LOW);
  digitalWrite(MS3_E0, LOW);
  digitalWrite(EN_E0, HIGH);
  digitalWrite(STP_Y, LOW);
  digitalWrite(DIR_Y, LOW);
  digitalWrite(MS1_Y, LOW);
  digitalWrite(MS2_Y, LOW);
  digitalWrite(MS3_Y, LOW);
  digitalWrite(EN_Y, HIGH);
}

void setup() {
  // pin groups
  int enpins[] = { EN_E0, EN_E1, EN_X, EN_Y, EN_Z };
  for(int i = 0; i < NUM_EN; ++i) enPins[i] = enpins[i];
  int oupins[] = { STP_E0, DIR_E0, MS1_E0, MS2_E0, MS3_E0, EN_E0,
                   STP_Y,  DIR_Y,  MS1_Y,  MS2_Y,  MS3_Y,  EN_Y };
  for(int i = 0; i < NUM_OUTS; ++i) ouPins[i] = oupins[i];
  // processes setups
  for(int i = 0; i < NUM_PINS; ++i){
    processes[i].moves = 0;
    processes[i].steps = 1;
    processes[i].count = 0;
    processes[i].first = HIGH;
    processes[i].second = LOW;
    processes[i].eid = 0;
    processes[i].callback = 0;
  }
  // set eid of processes
  int eid = 0;
  for(int i = NUM_OUTS - 1; i >= 0; --i){
    if(isEnabler(i))
      eid = i;
    processes[i].eid = eid;
  }
  // set enabler LOW-HIGH instead of HIGH-LOW
  for(int i = 0; i < 5; ++i){
    processes[enPins[i]].first = LOW;
    processes[enPins[i]].second = HIGH;
  }
  // set outputs
  for(int i = 0; i < 6 * 2; ++i){
    pinMode(ouPins[i], OUTPUT);
  }
  resetBEDPins(); //Set step, direction, microstep and enable pins to default states
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
  for(int i = 0; i < NUM_PINS; ++i){
    if(isPending(i) && processes[i].count == 0){
      digitalWrite(i, processes[i].first);
      processes[i].count = processes[i].steps;
    }
  }
  delayMilliseconds(DELAY_INTER);
  for(int i = 0; i < NUM_PINS; ++i){
    if(isPending(i)){
      if(processes[i].first != processes[i].second){
        digitalWrite(i, processes[i].second);
      }
      // decrement remaining time
      processes[i].count--;

      if(processes[i].moves > 0){
        state = STATE_BUSY; // we are busy with one process at least
      } else if(processes[i].callback != 0){
        processes[i].callback(); // notify that we're done with this process
      }
    }
  }
  delayMilliseconds(DELAY_AFTER);
  return STATE_IDLE;
}


void resetAll(){
  for(int i = 0; i < NUM_PINS; ++i){
    processes[i].moves = 0;
  }
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
          processes[pin].moves = moves;
          processes[pin].steps = steps;
          processes[pin].count = count;
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
        // if(dx) stepBy(STP_X, DIR_X, dx, sx, ix);
        if(dy) stepBy(STP_Y, DIR_Y, dy, sy, iy);
        // if(dy) stepBy(STP_Z, DIR_Z, dz, sz, iz);
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
    case -1:
      return;
    default:
      Serial.println("Unknown error!");
      break;
  }
  error = -1;
}

void begin() {
  for(int i = 0; i < NUM_PINS; ++i){
    int eid = processes[i].eid;
    if(isPending(i) && eid > 0){
      digitalWrite(eid, LOW);
    }
  }
}
void end() {
  int active[NUM_PINS];
  for(int i = 0; i < NUM_PINS; ++i){
    active[i] = 0;
  }
  for(int i = 0; i < NUM_PINS; ++i){
    int eid = processes[i].eid;
    if(isPending(i) && eid > 0){
      active[eid] = 1;
    }
  }
  for(int i = 0; i < NUM_ENS; ++i){
    int eid = enPins[i];
    if(active[eid] == 0){
      digitalWrite(eid, LOW);
    }
  }
}

// Main loop
void loop() {
  // show errors
  logError();

  // 1 = react to external events
  react();

  // 2 = enable outputs that are not idle
  begin();

  // 3 = process scheduled events
  byte state = STATE_IDLE;
  if(error == ERR_NONE){
    state = process();
  }

  // 3 = disable outputs that are idle
  end();

  // 4 = read user input
  if(state == STATE_IDLE){
    readCommands();
  }
}
void impulse(int pin, int first, int second, unsigned long delay = 0, Callback cb = NULL){
  processes[pin].moves = 1;
  processes[pin].steps = 1;
  processes[pin].count = delay;
  processes[pin].first = first;
  processes[pin].second = second;
  if(cb) processes[pin].callback = cb;
}
void stepBy(int stpPin, int dirPin, long move, unsigned long speed, unsigned long init = 0L, Callback cb = NULL){
    
  // direction setting
  int dirValue = move > 0 ? LOW : HIGH;
  change(dirPin, dirValue, dirValue, 0);
  // stepper movements
  processes[stpPin].moves = (unsigned long)(move > 0 ? move : -move) * speed;
  processes[stpPin].steps = speed;
  processes[stpPin].count = init; // must happen after direction is set
  processes[stpPin].first = HIGH;
  processes[stpPin].second = LOW;
  if(cb) processes[stpPin].callback = cb;
}

void microstepBy(int stpPin, int dirPin, int ms1Pin, int ms2Pin, int ms3Pin, long move, unsigned long speed, unsigned long init, Callback cb = NULL){
  impulse(ms1Pin, HIGH, HIGH, 0);
  impulse(ms2Pin, HIGH, HIGH, 0);
  impulse(ms3Pin, HIGH, HIGH, 0);
  stepBy(stpPin, dirPin, move, speed, init, cb);
  int delay = move * speed;
  impulse(ms1Pin, LOW, LOW, delay);
  impulse(ms2Pin, LOW, LOW, delay);
  impulse(ms3Pin, LOW, LOW, delay);
}

//Default microstep mode function
void StepForwardDefault()
{
  Serial.println("Moving forward at default step mode.");
  stepBy(STP_E0, DIR_E0, 1000L, SECOND, 0L);
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
  stepBy(STP_Y, DIR_Y, 200L, 2L * SECOND, 0L);
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
  stepBy(STP_E0, DIR_E0, -200L, 5L * SECOND, 0L);
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
void CurrentTestMode()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  microstepBy(STP_E0, DIR_E0, MS1_E0, MS2_E0, MS3_E0, 2000L, SECOND, 0L);
  microstepBy(STP_Y, DIR_Y, MS1_Y, MS2_Y, MS3_Y, 2000L, SECOND, 0L);
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
  stepBy(STP_E0, DIR_E0, 200L, SECOND / 2L, 0L);
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
  stepBy(STP_E0, DIR_E0, -1000L, SECOND, 0L);
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
  microstepBy(STP_E0, DIR_E0, MS1_E0, MS2_E0, MS3_E0, 1000L, SECOND, 0L);
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
  microstepBy(STP_Y, DIR_Y, MS1_Y, MS2_Y, MS3_Y, 1000L, SECOND, 0L);
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
  microstepBy(STP_E0, DIR_E0, MS1_E0, MS2_E0, MS3_E0, -1000L, SECOND, 0L);
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
void ForwardBackwardStep()
{
  Serial.println("Alternate between stepping forward and reverse.");
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
}

void DashedLineTest()
{
  //SmallStepReverseMode();
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
}

void DottedLineTest()
{
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
}
