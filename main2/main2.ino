
#include "error.h"
#include "types.h"
#include "stepper.h"

// delays in milliseconds
#define delayFunc delayMicroseconds
#define DELAY_INTER 100
#define DELAY_AFTER 100
#define SECOND      (1L)
#define MICROSECOND (1L)

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
void process() {
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->exec();
  }
  delayFunc(DELAY_INTER);
  for(int i = 0; i < NUM_STEPPERS; ++i){
    steppers[i]->release();
  }
  delayFunc(DELAY_AFTER);
}

// check if idle
int idle() {
  for(int i = 0; i < NUM_STEPPERS; ++i){
    if(steppers[i]->isRunning()) return 0;
  }
  return 1;
}

// reset al steppers and the error state
void resetAll(){
  for(int i = 0; i < NUM_STEPPERS; ++i)
    steppers[i]->reset();
  error = ERR_NONE; // clean flag
  Serial.println("Reset.");
}

// process user input
void readCommands(){
  while(Serial.available() && error <= ERR_NONE){
    // command type depends on first character
    char type = Serial.read();
    switch(type){
      // pX where X = pin delta speed init
      case 'p': {
        if(Serial.available()){
          int pin = readInt();
          unsigned long moves = readLong();
          unsigned long steps = readLong();
          unsigned long count = readLong();
          if(steps == 0){
            steps = SECOND;
          }
          if(pin < 0 || pin >= NUM_STEPPERS){
            error = ERR_INPUT;
            break;
          }
          steppers[pin]->stepBy(moves, steps, count);
        }
      } break;
      // reset all processes (needed in case of error)
      case 'r':
      case 'R': {
        resetAll();
      } break;

      // microstepping on/off
      case '+':
      case '-': {
        if(Serial.available()){
          int pin = readInt();
          if(pin < 0 || pin >= NUM_STEPPERS){
            error = ERR_INPUT;
            break;
          }
          steppers[pin]->microstep(type == '+' ? LOW : HIGH);
        }
      } break;

      // moveby dx dy dz sx [sy sz ix iy iz]
      case 'm': {
        long dx = readLong(),
             dy = readLong(),
             dz = readLong();
        unsigned long sx = readULong() * MICROSECOND,
                      sy = readULong() * MICROSECOND,
                      sz = readULong() * MICROSECOND,
                      ix = readULong(),
                      iy = readULong(),
                      iz = readULong();
        if(sx == 0L) sx = SECOND;
        if(sy == 0L) sy = sx;
        if(sz == 0L) sz = sx;
        // if(dx) stpX.stepBy(dx, sx, ix);
        if(dy) stpY.stepBy(dy, sy, iy);
        // if(dz) stpZ.stepBy(dz, sz, iz);
      } break;

      case 'Y':
      case 'y': {
        if(type == 'y'){
          stpY.microstep(HIGH);
        } else {
          stpY.microstep(LOW);
        }
        long delta = readLong();
        unsigned long speed = readULong() * MICROSECOND,
                      init  = readULong();
        if(speed == 0L) speed = SECOND;
        if(delta) stpY.stepBy(delta, speed, init);
      }

      // extrudeBy total steps init id
      case 'E':
      case 'e': {
        if(type == 'e'){
          stpE0.microstep(HIGH);
        } else {
          stpE0.microstep(LOW);
        }
        long delta = -readLong();
        unsigned long speed = readULong(),
                      init  = readULong();
        if(speed == 0L) speed = SECOND;
        int id = readInt();
        if(id == 0) stpE0.stepBy(delta, speed, init);
        // if(id == 1) stpE1.stepBy(delta, speed, init);
      } break;

      /**
       * Successful commands
       * 
       * e/y1000 5 = 1000 moves at 5 steps speed with delayMicroseconds 100
       */

      default:
        // otherwise
        break;
    }
  }
  // remove any extra
  Serial.flush();
}

// Main loop
void loop() {
  // show errors
  logError();

  // 1 = react to external events
  react();

  // 2 = process scheduled events
  if(error == ERR_NONE){
    process();
  }

  // 3 = read user input
  if(error > ERR_NONE){
    for(int i = 0; i < NUM_STEPPERS; ++i){
      steppers[i]->reset();
    }
  }
  if(idle()){
    Serial.println("Idle wait.");
    delay(1000);
  }
  readCommands();
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

void DashedLineTest(int i = 0) {
  //SmallStepReverseMode();
  switch(i){
    case 0:
      // microstep mode
      stpE0.microstep(HIGH);
      stpY.microstep(HIGH);
    case 4:
      // move
      stpE0.stepBy(-2000L * 3L, SECOND, 0L);
      stpY.stepBy(2000L * 3L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 1:
      stpE0.stepBy(1000L * 2L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 2:
      stpY.stepBy(1000L * 4L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 3:
      stpE0.stepBy(-1000L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 5:
      stpE0.stepBy(1000L * 3L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 6:
      stpY.stepBy(1000L, SECOND, 0L, DashedLineTest, i + 1);
      break;
    case 7:
      // disable microstepping
      stpE0.microstep(LOW);
      stpY.microstep(LOW);
      // done
      Serial.println("Done with dashed line test.");
      return;
    default:
      error = -2;
      break;
  }
  /*
  CurrentTestMode(); // 0
  CurrentTestMode();
  CurrentTestMode();
  SmallStepMode(); // 1
  SmallStepMode();
  SmallStepY(); // 2
  SmallStepY();
  SmallStepY();
  SmallStepY();
  SmallStepReverseMode(); // 3
  CurrentTestMode(); // 4
  CurrentTestMode();
  CurrentTestMode();
  SmallStepMode(); // 5
  SmallStepMode();
  SmallStepMode();
  SmallStepY(); // 6
  */
}

void DottedLineTest(int i = 0)
{
  switch(i){
    case 0:
      stpE0.microstep(HIGH);
      stpY.microstep(HIGH);
    case 4:
    case 8:
      stpE0.stepBy(-1000L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 1:
    case 5:
    case 9:
      stpE0.stepBy(-2000L, SECOND, 0L);
      stpY.stepBy(2000L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 2:
    case 6:
    case 10:
      stpE0.stepBy(1000L * 3L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 3:
    case 11:
      stpY.stepBy(1000L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 7:
      stpY.stepBy(1000L, SECOND, 0L, DottedLineTest, i + 1);
      break;
    case 12:
      // disable microstepping
      stpE0.microstep(LOW);
      stpY.microstep(LOW);
      // done
      Serial.println("Done with dashed line test.");
      return;
    default:
      error = -2;
      break;
  }
  /*
  SmallStepReverseMode(); // 0
  CurrentTestMode(); // 1
  SmallStepMode(); // 2
  SmallStepMode();
  SmallStepMode();
  SmallStepY(); // 3
   SmallStepReverseMode(); // 4
  CurrentTestMode(); // 5
  SmallStepMode(); // 6
  SmallStepMode();
  SmallStepMode();
  SmallStepY(); // 7
  SmallStepY();
   SmallStepReverseMode(); // 8
  CurrentTestMode(); // 9
  SmallStepMode(); // 10
  SmallStepMode();
  SmallStepMode();
  SmallStepY(); // 11
  */
}
