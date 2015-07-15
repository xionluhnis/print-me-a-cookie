
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


//Declare variables for functions
char user_input;
int x;
int y;
int state;

void setup() {
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
  Serial.println("Begin motor control");
  Serial.println();
  //Print function list for user selection
  Serial.println("Enter number for control option:");
  Serial.println("1. Turn at default microstep mode.");
  Serial.println("2. Dotted Line Test.");
  Serial.println("3. Turn at 1/16th microstep mode.");
  Serial.println("4. Step forward and reverse directions.");
  Serial.println("5. Reverse direction at slow microstep mode.");
  Serial.println("6. Turn at 1/16th microstep reverse mode.");
  Serial.println("7. Line Test (16th Microstep)");
  Serial.println("8. Turn at default microstep mode Y");
  Serial.println("9. Small Step Y");
  Serial.println("0. Dashed Line Test");
  Serial.println();
}

//Main loop
void loop() {
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

//Reset Big Easy Driver pins to default states
void resetBEDPins()
{
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
