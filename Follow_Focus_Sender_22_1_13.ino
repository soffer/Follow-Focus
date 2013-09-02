// Movement of Optical encoder is translated to moving stepper
// THIS IS A WORKING VERSION 22.1.13

//      Adi Soffer  2013       //
//    for more info visit      //
// http://adisoffer.tumblr.com //

#include <OneButton.h>

//these pins can not be changed 2/3 are special pins
int encoderPin1 = 2;
int encoderPin2 = 3;

int lastEncoded = 0;
int encoderValue = 0;

int lastencoderValue = 0;
int lastMSB = LOW;
int lastLSB = LOW;

//LEDs
#define realTimeLED 12      //Real Time LED
#define playLED 9          //PLay LED
#define inLED 11            //In LED
#define outLED 10           //Out LED

// Setup OneButton 
OneButton realTimebutton(A3, true);
OneButton playButton (A1, true);
OneButton inButton (A5, true);
OneButton outButton (A4, true);

//Values for focus points
volatile int inPoint = 0;
volatile int outPoint = 3000;

//Blink without delay 
boolean ledState = LOW;
long previousMillis = 0;
int ledInterval = 75;

//Defining booleans
boolean rClickedOnce = false;
boolean rLongPress = false;
boolean pClickedOnce = false;
boolean highEndMark = false;
boolean lowEndMark = false;
boolean startUp = false;

//Value for sendFunction - to send over Xbee
byte value;

//value for variable speed - to send over Xbee
int valToRemap;
int encoderValToRemap;

//Value to recieve from 2nd Xbee: when "play" or "rewind" has finished"
int dataReceive;

//Modes
int mode;
#define Stop 1
#define LENSCALIB 2

void setup(){
  Serial.begin(115200);

  //Pinmodes
  pinMode (realTimeLED, OUTPUT);
  pinMode (playLED, OUTPUT);
  pinMode (inLED, OUTPUT);
  pinMode (outLED, OUTPUT);
  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //Attach Click to Buttons
  realTimebutton.attachClick(Click);
  playButton.attachClick(ClickPlay);
  inButton.attachClick(ClickIn);
  outButton.attachClick(ClickOut);

  //Attach Press to Real Time for calibrating lens and motor
  realTimebutton.attachPress(rPress);
  //StartUp light




}

void loop(){

  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  //only if changed value
  if(lastMSB != MSB || lastLSB != LSB){
    updateEncoder(MSB, LSB);
  }
  if (startUp == false)
  {
    for (int x=0;x<3;x++)
    {
      for (int l=9;l<13;l++)
      {
        digitalWrite(l,HIGH);
        delay (100);
        digitalWrite(l,LOW); 
        delay (100);
      }
    } 
    startUp = true;
  }


  // keep watching the push buttons:
  realTimebutton.tick();
  playButton.tick();
  inButton.tick();
  outButton.tick();

  // filter signal from stepper motor  
  //arduino when(play/rewind) is done.
  if (Serial.available())
  {
    dataReceive = Serial.read();
    if (dataReceive == 1)
    {
      pClickedOnce = false;
      rClickedOnce = false;
      digitalWrite(playLED, LOW);
    }
    else if (dataReceive == 2)
      blinkMark (inLED);

    else if (dataReceive == 3)
      blinkMark (outLED);
  }

  switch (mode)
  {

  case LENSCALIB:
    {
      blinkFunction (realTimeLED);

      if (lowEndMark == true && highEndMark == true ||rClickedOnce == true || pClickedOnce == true)
      {
        rLongPress = false;
        digitalWrite (realTimeLED, LOW);
        if (rClickedOnce == true)
          digitalWrite(realTimeLED, HIGH);
        if (pClickedOnce == true)
          digitalWrite (playLED, HIGH);
        mode = Stop;
        break;
      }
    }
  }
}


void updateEncoder(int MSB, int LSB){
  lastMSB = MSB;
  lastLSB = LSB;


  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
  {
    encoderValue ++;

    // This is a function making use of the data
    // in 2 different ways: 1/as encoder value 
    // 2/as speed value for play/rewind
    // depending on state of switches.
    sendFunction (1); 
  }
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) 
  {
    encoderValue --;

    // This is a function makinf use of the data
    // in 2 different ways: 1/as encoder value 
    // 2/as speed value for play/rewind
    // depending on state of switches.
    sendFunction(2);
  }
  lastEncoded = encoded; //store this value for next time
}

//4 Buttons Click Functions

//Real Time Switch
void Click() {
  digitalWrite(playLED, LOW);
  rLongPress = false;
  if (rClickedOnce == false )
  {
    rClickedOnce = true;
    digitalWrite(realTimeLED, HIGH); 
    Serial.write (6);
  }
  else
  {
    digitalWrite(realTimeLED, LOW);
    rClickedOnce = false;
  }
}
// Press Function - calibrating lens and stepper

void rPress() {
  if (rLongPress == false) {
    rLongPress = true;
    rClickedOnce = false;
    highEndMark = false;
    lowEndMark = false;
    Serial.write (9);
    mode = LENSCALIB;
  }
  else
  {
    rLongPress = false;
    digitalWrite(realTimeLED, LOW);
    mode = Stop;
  }
}

//Play Switch
void ClickPlay () {
  //terminate realtime in case it wasn't stopped
  if (pClickedOnce == false)
  {
    pClickedOnce = true;
    rClickedOnce = false; 
    rLongPress = false; 
    digitalWrite(realTimeLED, LOW);
    digitalWrite(playLED, HIGH);
    Serial.write(5);
    //Serial.print("pressed");
  }
  else
  {
    pClickedOnce = false;
    digitalWrite(playLED, LOW); 
  }
}

//In Switch
void ClickIn () {
  // Saving In
  if (rClickedOnce == true)                       
  {
    inPoint = encoderValue;
    Serial.write (3);
  }
  else if (rLongPress == true)
  {
    lowEndMark = true;
    Serial.write (7);
  }  
}

//Out Switch
void ClickOut() {
  //Saving Out
  if (rClickedOnce == true) 
  {
    Serial.write (4);
    outPoint = encoderValue;
  }
  else if (rLongPress == true)
  {
    highEndMark = true;
    Serial.write (8);
  }
}


int blinkFunction (int y)   // Blink without delay
{
  unsigned long currentMillis = millis ();
  if (currentMillis - previousMillis>ledInterval)
  {
    previousMillis = currentMillis;
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
    digitalWrite (y, ledState);

  }
}

int blinkMark (int y)
{
  boolean b = HIGH;
  for (int i=0;i<6;i++)
  {
    digitalWrite(y, b);
    delay(75);
    b=!b;
  }
}

int sendFunction(byte value)
{
  // 1/Send encoder value to move stepper 
  if (rClickedOnce == true||rLongPress == true)
    Serial.write (value); 

  else if (pClickedOnce == false)
  {
    // 2/Send remapped value to change speed of stepper     
    encoderValToRemap = encoderValue;

    if (inPoint<outPoint)
    {
      if (encoderValToRemap< inPoint)
        encoderValToRemap = inPoint;

      if (encoderValToRemap>outPoint)
        encoderValToRemap = outPoint;    
    }
    else
    {
      if (encoderValToRemap>inPoint)
        encoderValToRemap = inPoint;

      if (encoderValToRemap<outPoint)
        encoderValToRemap = outPoint;
    } 

    //remap value to ba able to send bytes
    valToRemap = focusSpeed(encoderValToRemap);
  }
}


int focusSpeed (int valToRemap)
{
  //remapped value sent to "receiving" arduino to change stepper speed
  //during play/rewind.
  valToRemap = map(valToRemap, inPoint, outPoint, 10, 30);

  if (valToRemap>30)
    valToRemap = 30;

  if (valToRemap<10)
    valToRemap = 10;

  Serial.write (valToRemap);
}




