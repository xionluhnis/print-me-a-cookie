
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
//Declare pin functions on Arduino
#define stp 2
#define dir 3
#define MS1 4
#define MS2 5
#define MS3 6
#define EN  7

#define stpY 8
#define dirY 9
#define MS1Y 10
#define MS2Y 11
#define MS3Y 12
#define ENY  13

// errors
#define ERR_NONE  0
#define ERR_PARSE 1
int error;

// delays in milliseconds
#define DELAY_INTER 50
#define DELAY_AFTER 30

// state data

#define NUM_PINS 54

struct PinProcess {
  unsigned long total; // total number of iterations
  unsigned long steps; // number of steps to trigger action
  unsigned long count; // accumulation buffer
  int first;
  int second;
};

struct PinProcess processes[NUM_PINS];

byte isPending(int pin) {
  return processes[pin].total > 0;
}

//Declare variables for functions
char user_input;
int x;
int y;
int state;

//Reset Big Easy Driver pins to default states
void resetBEDPins() {
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, LOW);
  digitalWrite(MS2, LOW);
  digitalWrite(MS3, LOW);
  digitalWrite(EN, HIGH);
  digitalWrite(stpY, LOW);
  digitalWrite(dirY, LOW);
  digitalWrite(MS1Y, LOW);
  digitalWrite(MS2Y, LOW);
  digitalWrite(MS3Y, LOW);
  digitalWrite(ENY, HIGH);
}

void setup() {
  error = ERR_NONE;
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(MS3, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(stpY, OUTPUT);
  pinMode(dirY, OUTPUT);
  pinMode(MS1Y, OUTPUT);
  pinMode(MS2Y, OUTPUT);
  pinMode(MS3Y, OUTPUT);
  pinMode(ENY, OUTPUT);
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
      processes[i].total--;
      processes[i].count--;
      if(processes[i].total > processes[i].steps){
        state = STATE_BUSY; // we are busy with one process at least
      }
    }
  }
  delayMilliseconds(DELAY_AFTER);
  return STATE_IDLE;
}

int readInt(){
  int val = 0;
  int sign = 1;
  while(Serial.available()){
    char c = Serial.read();
    int d = (int)(c - '0');
    if(val == 0 && c == '-'){
      sign = -1;
    } else if(d >= 0 && d < 10) {
      val *= 10;
      val += d;
    } else {
      error = ERR_PARSE;
      return val;
    }
  }
  return sign * val;
}

unsigned long readLong(){
  unsigned long val = 0L;
  while(Serial.available()){
    char c = Serial.read();
    if(c == ' '){
      break;
    }
    unsigned long d = (unsigned long)(c - '0');
    if(d < 0 || d > 9){
      error = ERR_PARSE;
      return val;
    }
    val *= 10L;
    val += d;
  }
  return val;
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
          unsigned long total = readLong();
          unsigned long steps = readLong();
          unsigned long count = readLong();
          if(steps == 0){
            steps = 1;
          }
          processes[pin].total = total;
          processes[pin].steps = steps;
          processes[pin].count = count;
        }
      } break;
      // reset all processes (needed in case of error)
      case 'r':
      case 'R': {
        for(int i = 0; i < NUM_PINS; ++i){
          processes[i].total = 0;
        }
        error = ERR_NONE; // clean flag
        Serial.println("Reset.");
      } break;

      // moveby dx dy dz sx [sy sz ix iy iz]
      case 'm': {
        int dx = readInt(),
            dy = readInt(),
            dz = readInt();
        unsigned long sx = readLong(),
                      sy = readLong(),
                      sz = readLong(),
                      ix = readLong(),
                      iy = readLong(),
                      iz = readLong();
        if(sx == 0) sx = 1;
        if(sy == 0) sy = sx;
        if(sz == 0) sz = sx;
        // if(dx) moveBy(stpX, dirX, dx, sx, ix);
        if(dy) moveBy(stpY, dirY, dy, sy, iy);
        // if(dy) moveBy(stpZ, dirZ, dz, sz, iz);
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

// Main loop
void loop() {
  // show errors
  logError();

  // 1 = react to external events
  react();

  // 1 = process scheduled events
  byte state = STATE_IDLE;
  if(error == ERR_NONE){
    state = process();
  }
  // reset all states
  resetBEDPins();

  // 2 = read user input
  if(state == STATE_IDLE){
    readCommands();
  }
}
void moveBy(int stpPin, int dirPin, int move, int speed, int init){
    
  // direction setting
  processes[dirPin].total = 1;
  processes[dirPin].steps = 1;
  processes[dirPin].count = 0;
  processes[dirPin].first = processes[dirY].second = dy > 0 ? LOW : HIGH;
  // stepper movements
  processes[stpPin].total = move * speed;
  processes[stpPin].steps = speed;
  processes[stpPin].count = init;
  processes[stpPin].first = HIGH;
  processes[stpPin].second = LOW;
}

void nothing() {
  while(Serial.available()){
      user_input = Serial.read(); //Read user input and trigger appropriate function
      digitalWrite(EN, LOW); //Pull enable pin low to set FETs active and allow motor control
      digitalWrite(ENY, LOW);
      if (user_input =='1')
      {
         StepForwardDefault();
      }
      else if(user_input =='2')
      {
        DottedLineTest();
      }
      else if(user_input =='3')
      {
        SmallStepMode();
      }
      else if(user_input =='4')
      {
        ForwardBackwardStep();
      }
       else if(user_input =='5')
      {
        ReverseStepSlow();
      }
       else if(user_input =='6')
      {
        SmallStepReverseMode();
      }
      else if(user_input =='7')
      {
        CurrentTestMode();
      }
      else if(user_input =='8')
      {
        StepForwardDefaultY();
      }
       else if(user_input =='9')
       {
        SmallStepY();
       }
        else if(user_input =='0')
       {
        DashedLineTest();
       }
      else
      {
        Serial.println("Invalid option entered.");
      }
      resetBEDPins();
  }
}

//Default microstep mode function
void StepForwardDefault()
{
  Serial.println("Moving forward at default step mode.");
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

void StepForwardDefaultY()
{
  Serial.println("Moving Y forward at default step mode.");
  digitalWrite(dirY, LOW); //Pull direction pin low to move "forward"
  for(x= 1; x<200; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stpY,HIGH); //Trigger one step forward
    delay(2);
    digitalWrite(stpY,LOW); //Pull step pin low so it can be triggered again
    delay(2);
  }
  Serial.println("Enter new option");
  Serial.println();
}

//Reverse default microstep mode function
void ReverseStepSlow()
{
  Serial.println("Moving in reverse at slow step mode.");
  digitalWrite(dir, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<200; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step
    delay(5);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(5);
  }
  Serial.println("Enter new option");
  Serial.println();
}

//Reverse default microstep mode function long then forward to cut strand
void CurrentTestMode()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  digitalWrite(dir, HIGH); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  digitalWrite(dirY, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2Y, HIGH);
  digitalWrite(MS3Y, HIGH);
  for(x= 1; x<2000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    digitalWrite(stpY,HIGH);
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    digitalWrite(stpY,LOW);
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

void Danger()
{
  Serial.println("Moving in reverse at slow step mode.");
  digitalWrite(dir, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<200; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step
    delayMicroseconds(500);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delayMicroseconds(500);
  }
  Serial.println("Enter new option");
  Serial.println();
}

void ReverseStepDefault()
{
  Serial.println("Moving in reverse at default step mode.");
  digitalWrite(dir, HIGH); //Pull direction pin high to move in "reverse"
  for(x= 1; x<1000; x++)  //Loop the stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

// 1/16th microstep foward mode function
void SmallStepMode()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

// 1/16th microstep foward mode function
void SmallStepY()
{
  Serial.println("Stepping at 1/16th microstep mode.");
  digitalWrite(dirY, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2Y, HIGH);
  digitalWrite(MS3Y, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stpY,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stpY,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

// 1/16th microstep foward mode function
void SmallStepReverseMode()
{
  Serial.println("Stepping at 1/16th microstep reverse mode.");
  digitalWrite(dir, HIGH); //Pull direction pin low to move "Reverse"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stp,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
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
      digitalWrite(stp,HIGH); //Trigger one step
      delay(1);
      digitalWrite(stp,LOW); //Pull step pin low so it can be triggered again
      delay(1);
    }
  }
  Serial.println("Enter new option");
  Serial.println();
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
  Serial.println("Enter new option");
  Serial.println();
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
  Serial.println("Enter new option");
  Serial.println();
}
