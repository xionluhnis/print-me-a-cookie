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

#define stpZ 22
#define dirZ 23
#define MS1Z 24
#define MS2Z 25
#define MS3Z 26
#define ENZ  27


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
  pinMode(stpZ, OUTPUT);
  pinMode(dirZ, OUTPUT);
  pinMode(MS1Z, OUTPUT);
  pinMode(MS2Z, OUTPUT);
  pinMode(MS3Z, OUTPUT);
  pinMode(ENZ, OUTPUT);
  resetBEDPins(); //Set step, direction, microstep and enable pins to default states
  Serial.begin(9600); //Open Serial connection for debugging
  Serial.println("Begin motor control");
  Serial.println();
  //Print function list for user selection
  Serial.println("Enter number for control option:");
  Serial.println("1. Extrude Upward at default mode.");
  Serial.println("2. Dotted Line Test.");
  Serial.println("3. Extrude Upward at 1/16th microstep mode.");
  Serial.println("4. Step Backwards Default Y.");
  Serial.println("5. Extrude Downward slow mode.");
  Serial.println("6. Extrude Downward at 1/16th microstep mode.");
  Serial.println("7. Line Test (16th Microstep)");
  Serial.println("8. Move Forward at Default Microstep Mode Y");
  Serial.println("9. Move Forward at 16th Microstep Y");
  Serial.println("0. Dashed Line Test");
  Serial.println("T. Z Axis Test");
  Serial.println("t. Z Axis Microstep Test");
  Serial.println("D. Z Axis Down.");
  Serial.println();
}

//Main loop
void loop() {
  while(Serial.available()){
      user_input = Serial.read(); //Read user input and trigger appropriate function
      digitalWrite(EN, LOW); //Pull enable pin low to set FETs active and allow motor control
      digitalWrite(ENY, LOW);
      digitalWrite(ENZ, LOW);
      if (user_input =='1')
      {
         ExtrudeUpwardDefault();
      }
      else if(user_input =='2')
      {
        DottedLineTest();
      }
      else if(user_input =='3')
      {
        ExtrudeUpwardMicro();
      }
      else if(user_input =='4')
      {
        StepBackwardsDefaultY();
      }
       else if(user_input =='5')
      {
        ExtrudeDownwardSlow();
      }
       else if(user_input =='6')
      {
        ExtrudeDownwardMicro();
      }
      else if(user_input =='7')
      {
        ExtrudeDownwardMicroStepForwardMicroY();
      }
      else if(user_input =='8')
      {
        StepForwardDefaultY();
      }
      else if(user_input =='9')
      {
        StepForwardMicroY();
      }
      else if(user_input =='0')
      {
        DashedLineTest();
      }
      else if(user_input =='T')
      {
        ZAxisUpwards();
      }
      else if(user_input =='t')
      {
        ZAxisMicrostepUpwards();
      }
      else if(user_input =='D')
      {
        ZAxisDownwards();
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
  digitalWrite(stpZ, LOW);
  digitalWrite(dirZ, LOW);
  digitalWrite(MS1Z, LOW);
  digitalWrite(MS2Z, LOW);
  digitalWrite(MS3Z, LOW);
  digitalWrite(ENZ, HIGH);
}

// 1/16th microstep foward mode function
void ZAxisMicrostepUpwards()
{
  Serial.println("Z Axis up at 1/16th microstep mode.");
  digitalWrite(dirZ, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1Z, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2Z, HIGH);
  digitalWrite(MS3Z, HIGH);
  for(x= 1; x<1000; x++)  //Loop the forward stepping enough times for motion to be visible
  {
    digitalWrite(stpZ,HIGH); //Trigger one step forward
    delay(1);
    digitalWrite(stpZ,LOW); //Pull step pin low so it can be triggered again
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

void ZAxisUpwards()
{
  Serial.println("Z axis up at default step mode.");
  digitalWrite(dirZ, LOW); 
  for(x= 1; x<1000; x++)  
  {
    digitalWrite(stpZ,HIGH); 
    delay(1);
    digitalWrite(stpZ,LOW);
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

void ZAxisDownwards()
{
  Serial.println("Z axis down at default step mode.");
  digitalWrite(dirZ, HIGH); 
  for(x= 1; x<1000; x++)  
  {
    digitalWrite(stpZ,HIGH); 
    delay(1);
    digitalWrite(stpZ,LOW);
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

//Default microstep mode function
void ExtrudeUpwardDefault()
{
  Serial.println("Extruding upward at default step mode.");
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

void StepBackwardsDefaultY()
{
  Serial.println("Moving Y backwards at default step mode.");
  digitalWrite(dirY, HIGH); //Pull direction pin low to move "forward"
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

void StepBackwardsMicroY()
{
  Serial.println("Moving Y backwards at micro step mode.");
  digitalWrite(dirY, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2Y, HIGH);
  digitalWrite(MS3Y, HIGH);
  digitalWrite(dirY, HIGH); //Pull direction pin low to move "forward"
  for(x= 1; x<2000; x++) 
  {
    digitalWrite(stpY,HIGH); 
    delay(1);
    digitalWrite(stpY,LOW); 
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

//Reverse default microstep mode function
void ExtrudeDownwardSlow()
{
  Serial.println("Extrduing downward at slow step mode.");
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

void ExtrudeUpwardsSlow()
{
  Serial.println("Extrduing upwards at slow step mode.");
  digitalWrite(dir, LOW); //Pull direction pin high to move in "reverse"
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
void ExtrudeDownwardMicroStepForwardMicroY()
{
  Serial.println("Step forward and extrude downward 1/16th microstep mode.");
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

void DelayMicrosecondsTest()
{
  Serial.println("Delaymicroseconds test");
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

void ExtrudeDownwardDefault()
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
void ExtrudeUpwardMicro()
{
  Serial.println("Extruding upwards at 1/16th microstep mode.");
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
void StepForwardMicroY()
{
  Serial.println("Moving Y forward at 1/16th microstep mode.");
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
void ExtrudeUpwardDefaultStepForwardMicroY()
{
  Serial.println("Extruding upward and stepping forward at 1/16th microstep mode.");
  digitalWrite(dirY, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2Y, HIGH);
  digitalWrite(MS3Y, HIGH);
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  for(x= 1; x<1000; x++)  
  {
    digitalWrite(stpY,HIGH); 
    digitalWrite(stp,HIGH); 
    delay(1);
    digitalWrite(stpY,LOW); 
    digitalWrite(stp,LOW); 
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

// 1/16th microstep foward mode function
void ExtrudeUpwardDefaultStepBackwardsMicroY()
{
  Serial.println("Extruding upward and stepping forward at 1/16th microstep mode.");
  digitalWrite(dirY, HIGH); //Pull direction pin low to move "forward"
  digitalWrite(MS1Y, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2Y, HIGH);
  digitalWrite(MS3Y, HIGH);
  digitalWrite(dir, LOW); //Pull direction pin low to move "forward"
  digitalWrite(MS1, HIGH); //Pull MS1,MS2, and MS3 high to set logic to 1/16th microstep resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  for(x= 1; x<1000; x++)  
  {
    digitalWrite(stpY,HIGH); 
    digitalWrite(stp,HIGH); 
    delay(1);
    digitalWrite(stpY,LOW); 
    digitalWrite(stp,LOW); 
    delay(1);
  }
  Serial.println("Enter new option");
  Serial.println();
}

// 1/16th microstep foward mode function
void ExtrudeDownwardMicro()
{
  Serial.println("Extruding downward at 1/16th microstep reverse mode.");
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
  ExtrudeDownwardMicro();
  //ExtrudeDownwardMicroStepForwardMicroY();
  ExtrudeDownwardMicroStepForwardMicroY();
  ExtrudeDownwardMicroStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardsSlow();
  //ExtrudeUpwardMicro();
  //ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
  //ExtrudeDownwardMicroStepForwardMicroY();
  ExtrudeDownwardMicroStepForwardMicroY();
  ExtrudeDownwardMicroStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardsSlow();
  ExtrudeUpwardsSlow();
  //ExtrudeUpwardMicro();
  //ExtrudeUpwardDefaultStepForwardMicroY();
  ZAxisDownwards();
  ZAxisDownwards();
  ExtrudeUpwardDefaultStepBackwardsMicroY();
  ExtrudeUpwardDefaultStepBackwardsMicroY();
  ExtrudeUpwardDefaultStepBackwardsMicroY();
  StepBackwardsMicroY();
  StepBackwardsMicroY();
  StepBackwardsMicroY();
  /*StepBackwardsMicroY();
  StepBackwardsMicroY();
  StepBackwardsMicroY();
  StepBackwardsMicroY();*/
  
  Serial.println("Enter new option");
  Serial.println();
}

void DottedLineTest()
{
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
//  ExtrudeDownwardMicro();
  //ExtrudeDownwardMicroStepForwardMicroY();
  //ExtrudeUpwardDefault();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
 // ExtrudeDownwardMicro();
  //ExtrudeDownwardMicroStepForwardMicroY();
  //ExtrudeUpwardDefault();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
  ExtrudeDownwardMicro();
 // ExtrudeDownwardMicro();
  //ExtrudeDownwardMicroStepForwardMicroY();
  //ExtrudeUpwardDefault();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  ExtrudeUpwardDefaultStepForwardMicroY();
  Serial.println("Enter new option");
  Serial.println();
}
