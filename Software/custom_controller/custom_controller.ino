#include <Joystick.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 mcp;

byte ledPin=13;

// Interrupts from the MCP will be handled by this PIN
byte arduinoIntPin=3;

// ... and this interrupt vector
byte arduinoInterrupt=1;

volatile boolean awakenByInterrupt = false;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  11, 1,                 // Button Count, Hat Switch Count
  true, true, true,      // X, Y, and Z
  true, true, false,     // Rx, Ry, no Rz
  false, false,          // no rudder or throttle
  false, false, false);  // no accelerator, brake, or steering

#define leftX 2
#define leftY 1
#define leftTrig 5
#define rightX 5
#define rightY 4
#define rightTrig 0
const int stickPins[] = {A0, A1, A2, A3, A4, A5};
int stickValues[6];
boolean stickChanged = false;

//#define buttonA 1
//#define buttonB 2
//#define buttonX 4
//#define buttonY 8
//#define buttonL2 16
//#define buttonR2 32
//#define buttonL3 64
//#define buttonR3 128
//#define buttonSelect 256
//#define buttonStart 512
//#define buttonHome 1024
//#define Unused 2048
//#define buttonDUp 4096
//#define buttonDRight 8192
//#define buttonDDown 16384
//#define buttonDLeft 32768
//                       NA,u,r ,ur,d  ,du,dr ,dur,l  ,lu ,lr,lru,ld ,ldu,ldr,ldru
const int hatValues[] = {-1,0,90,45,180,-1,135,-1 ,270,315,-1,-1 ,225,-1 ,-1 ,-1};
boolean buttonValues[15];

void setup() {
  Joystick.begin(false);//only update controller state when sendState() is called

  for(int i = 0; i < 6; i++){
    pinMode(stickPins[i], INPUT);
  }

  pinMode(arduinoIntPin,INPUT);

  mcp.begin();      // use default address 0

  // We mirror INTA and INTB, so that only one line is required between MCP and Arduino for int reporting
  // The INTA/B will not be Floating 
  // INTs will be signaled with a LOW
  mcp.setupInterrupts(true,false,LOW);

  // configuration for buttons
  // interrupt will triger when the pin is taken to ground by a pushbutton
  for(int i = 0; i < 15; i++){
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);  // turn on a 100K pullup internally
    mcp.setupInterruptPin(i,CHANGE);
  }
}

void loop() {
  // enable interrupts before going to sleep/wait
  // And we setup a callback for the arduino INT handler.
  attachInterrupt(arduinoInterrupt,intCallBack,FALLING);
  
  while(!awakenByInterrupt){
    for(int i = 0; i < 6; i++){
      int tmp = analogRead(stickPins[i]);
      if(stickValues[i] != tmp){
        stickValues[i] = tmp;
        stickChanged = true;
      }
    }

    if(stickChanged == true){
      Joystick.setXAxis(1023 - stickValues[leftX]);
      Joystick.setYAxis(1023 - stickValues[leftY]);
    
      Joystick.setRxAxis(1023 - stickValues[rightX]);
      Joystick.setRyAxis(stickValues[rightY]);
    
      int z = 512 + (stickValues[rightTrig] / 2) - (stickValues[leftTrig] / 2);
      Joystick.setZAxis(z);

      Joystick.sendState();//send the updated joystick states all at once

      stickChanged = false;
    }
  }
  
  // disable interrupts while handling them.
  detachInterrupt(arduinoInterrupt);
  
  if(awakenByInterrupt) handleInterrupt();
}

// The int handler will just signal that the int has happen
// we will do the work from the main loop.
void intCallBack(){
  awakenByInterrupt=true;
}

void handleInterrupt(){
  
  // Get more information from the MCP from the INT
  uint8_t pin=mcp.getLastInterruptPin();
  uint8_t val=mcp.getLastInterruptPinValue();

  uint16_t buttons = mcp.readGPIOAB();
  
  for(int i = 0; i < 11; i++){
    Joystick.setButton(i, (buttons & (1 << i)) ? 1 : 0);
  }

  Joystick.setHatSwitch(0, hatValues[(buttons >> 12) & 0x0F]);

  Joystick.sendState();//send the updated joystick states all at once

  // we have to wait for the interrupt condition to finish
  // otherwise we might go to sleep with an ongoing condition and never wake up again.
  // as, an action is required to clear the INT flag, and allow it to trigger again.
  // see datasheet for datails.
  //while( ! (mcp.digitalRead(mcpPinB) && mcp.digitalRead(mcpPinA) ));
  
  // and clean queued INT signal
  cleanInterrupts();
}

// handy for interrupts triggered by buttons
// normally signal a few due to bouncing issues
void cleanInterrupts(){
  EIFR=0x01;
  awakenByInterrupt=false;
}
