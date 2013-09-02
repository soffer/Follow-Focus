// Movement of Optical encoder is translated to moving stepper
// THIS IS A WORKING VERSION 22.1.13

//      Adi Soffer  2013       //
//    for more info visit      //
// http://adisoffer.tumblr.com //

#include <AccelStepper.h>

//encoder/motor/driver setup
int easyDriverMicroSteps = 1; 
int rotaryEncoderSteps = 75; 
int motorStepsPerRev = 200; 

int MinPulseWidth = 50; //too low and the motor will stall, too high and it will slow it down

int easyDriverStepPin = 4;
int easyDriverDirPin = 5;
int enablePin = 6;

volatile long encoderValue = 0;
byte dataReceive = 0;
long lastencoderValue = 0;

//Values for focus points
int inPoint;
int outPoint;

//value for variable speed
int speedValue = 1000;
int valToSpeed;
int mappedSpeed;

//Values for Lens Calibration
int lowEndMark = -50000;
int highEndMark = 50000;

//ON LED
#define onLed 12


int mode;
#define Rewind 1
#define Play 2
// standBy mode is meant to enable Xbee receive changes in speed
// in values 0-255, remap to 200-100 and reyrn to Play/Rewind.
// currently not working.
#define standBy 3
#define realTime 4
#define Stop 5
#define LENSCALIB 6


AccelStepper stepper(1, easyDriverStepPin, easyDriverDirPin);

//Sleep Function - to diable ED when not active
long previousMillis = 0;
int sleepTimer = 5000;

void setup(){
  Serial.begin(115200);
  stepper.setMinPulseWidth(MinPulseWidth); 
  stepper.setMaxSpeed(speedValue);             //variable to later determine speed play/rewind
  stepper.setAcceleration(100000); 
  stepper.setSpeed(50000); 


  pinMode(onLed, OUTPUT);
  pinMode(enablePin, OUTPUT);

  digitalWrite(onLed, HIGH);
}

void loop(){
  stepper.run();

  if (Serial.available()>0)
  {
    digitalWrite (enablePin, LOW);
    previousMillis = millis();
    dataReceive = Serial.read();
    //function to sort data reveived and action.
    dataSort ();
    //take care of variable speed
    stepperMove();
  }
  else
  {
    //Stepper sleep after 5sec of no data
    unsigned long currentMillis = millis ();
    if (currentMillis - previousMillis>sleepTimer)
      digitalWrite (enablePin, HIGH);
  }

  switch (mode)
  {

  case standBy:
    /*if (Serial.available()>0)
     {
     dataReceive = Serial.read();
     if (dataReceive == 6) 
     mode = Stop; 
     }*/

    break;

  case Stop:
    break;

  case realTime:
    dataSort();
    stepperMove();

    break;

  case Rewind:
    //take care of variable speed
    stepper.setMaxSpeed(speedValue);
    stepper.moveTo(inPoint);
    stepper.run();
    if(stepper.currentPosition()==inPoint)
    {
      Serial.write (1);  
      mode=standBy; 
    }

    break; 

  case LENSCALIB:
    highEndMark = 50000;
    lowEndMark = -50000;
    mode = Stop;
    break;

  case Play:
    //take care of variable speed
    stepper.setMaxSpeed(speedValue);
    stepper.moveTo(outPoint);
    stepper.run();
    if(stepper.currentPosition()==outPoint)
    {
      Serial.write (1); 
      mode=standBy;
    }
    break;
  }
}

//Sort the received data and decide action
void dataSort()
{
  if (dataReceive == 3)
  {
    inPoint = stepper.currentPosition();
    Serial.write(2);
  }
  else if (dataReceive == 4)
  {
    outPoint = stepper.currentPosition();
    Serial.write (3); 
  } 
  else if (dataReceive == 1)
  {
    encoderValue ++;
  }
  else if (dataReceive == 2)
  {
    encoderValue --;
  }
  else if (dataReceive == 5)
  {
    //function to make Play or Rewind
    defineDirection();
  }
  else if (dataReceive == 6)
  { 
    mode = Stop;   
  }

  else if (dataReceive == 7)
  {
    lowEndMark = stepper.currentPosition();
    Serial.write(2); 
  }

  else if (dataReceive == 8)
  {
    highEndMark = stepper.currentPosition(); 
    Serial.write(3);
  }
  else if (dataReceive == 9)
  {
    mode = LENSCALIB;
  }

  else if (dataReceive>9 && dataReceive<31)
  {
    valToSpeed = dataReceive;
    valToSpeed = map(valToSpeed, 10, 30, 200, 1000);
    speedValue = valToSpeed;
  }  
}

void defineDirection()
{
  if (stepper.currentPosition()!=inPoint)
  { 
    mode = Rewind; 
  }
  else if (stepper.currentPosition() == inPoint)
  {
    mode = Play;
  }
}

void stepperMove ()
{

  int stepsPerRotaryStep = (motorStepsPerRev * easyDriverMicroSteps) / rotaryEncoderSteps;
  int motorMove = (encoderValue * stepsPerRotaryStep);
  if (mode==standBy)return; 
  //Lens Calibration
  if (motorMove >lowEndMark)
    motorMove = lowEndMark; 
  if (motorMove<highEndMark)
    motorMove = highEndMark;



  stepper.run();
  stepper.setMaxSpeed(1000);
  stepper.moveTo(motorMove);
}


