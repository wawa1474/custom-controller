#include <Joystick.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 vanillaMCP;
Adafruit_MCP23017 chocolateMCP;

// Interrupts from the MCP will be handled by this PIN
#define vanillaIntPin 0
#define chocolatePin 1

volatile boolean awakenByVanillaInterrupt = false;
volatile boolean awakenByChocolateInterrupt = false;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  12, 1,                 // Button Count, Hat Switch Count
  true, true, true,      // X, Y, and Z
  true, true, false,     // Rx, Ry, no Rz
  false, false,          // no rudder or throttle
  false, false, false);  // no accelerator, brake, or steering

//#define leftX 2
//#define leftY 1
//#define leftTrig 3
//#define rightX 5
//#define rightY 4
//#define rightTrig 0
//const int vanillaStickPins[] = {A0, A1, A2, A3, A4, A5};
#define leftX 0
#define leftY 1
#define leftTrig 2
#define rightX 3
#define rightY 4
#define rightTrig 5
const int vanillaStickPins[] = {A5, A1, A0, A2, A4, A3};
int vanillaStickValues[6];
boolean vanillaStickChanged = false;

//#define buttonA 1
//#define buttonB 2
//#define buttonX 4
//#define buttonY 8
//#define buttonL2 16
//#define buttonR2 32
//#define buttonSelect 64
//#define buttonStart 128
//#define buttonL3 256
//#define buttonR3 512
//#define buttonHome 1024
//#define buttonSync 2048
//#define buttonDUp 4096
//#define buttonDRight 8192
//#define buttonDDown 16384
//#define buttonDLeft 32768
//                       NA,u,r ,ur,d  ,du,dr ,dur,l  ,lu ,lr,lru,ld ,ldu,ldr,ldru
const int hatValues[] = {-1,0,90,45,180,-1,135,-1 ,270,315,-1,-1 ,225,-1 ,-1 ,-1};
boolean vanillaButtonValues[16];

void setup() {
  Joystick.begin(false);//only update controller state when sendState() is called

  for(int i = 0; i < 6; i++){
    pinMode(vanillaStickPins[i], INPUT);
  }

  pinMode(vanillaIntPin,INPUT);

  vanillaMCP.begin(0);      // use address 0
  chocolateMCP.begin(1);      // use address 1

  // We mirror INTA and INTB, so that only one line is required between MCP and Arduino for int reporting
  // The INTA/B will not be Floating 
  // INTs will be signaled with a LOW
  vanillaMCP.setupInterrupts(true,false,LOW);
  chocolateMCP.setupInterrupts(true,false,LOW);

  // configuration for buttons
  // interrupt will triger when the pin is taken to ground by a pushbutton
  for(int i = 0; i < 15; i++){
    vanillaMCP.pinMode(i, INPUT);
    vanillaMCP.pullUp(i, HIGH);  // turn on a 100K pullup internally
    vanillaMCP.setupInterruptPin(i,CHANGE);
  }

  // configuration for buttons
  // interrupt will triger when the pin is taken to ground by a pushbutton
  for(int i = 0; i < 15; i++){
    chocolateMCP.pinMode(i, INPUT);
    chocolateMCP.pullUp(i, HIGH);  // turn on a 100K pullup internally
    chocolateMCP.setupInterruptPin(i,CHANGE);
  }
}

void loop() {
  // enable interrupts before going to sleep/wait
  // And we setup a callback for the arduino INT handler.
  attachInterrupt(digitalPinToInterrupt(vanillaIntPin),vanillaIntCallBack,FALLING);
  attachInterrupt(digitalPinToInterrupt(chocolateIntPin),chocolateIntCallBack,FALLING);
  
  while(!awakenByVanillaInterrupt && !awakenByChocolateInterrupt){
    for(int i = 0; i < 6; i++){
      int tmp = analogRead(vanillaStickPins[i]);
      if(vanillaStickValues[i] != tmp){
        vanillaStickValues[i] = tmp;
        vanillaStickChanged = true;
      }
    }

    if(vanillaStickChanged == true){
      Joystick.setXAxis(1023 - vanillaStickValues[leftX]);
      Joystick.setYAxis(1023 - vanillaStickValues[leftY]);
    
      Joystick.setRxAxis(1023 - vanillaStickValues[rightX]);
      Joystick.setRyAxis(vanillaStickValues[rightY]);
    
      int z = 512 - (vanillaStickValues[rightTrig] / 2) + (vanillaStickValues[leftTrig] / 2);
      Joystick.setZAxis(z);

      Joystick.sendState();//send the updated joystick states all at once

      vanillaStickChanged = false;
    }
  }
  
  // disable interrupts while handling them.
  detachInterrupt(digitalPinToInterrupt(vanillaIntPin));
  detachInterrupt(digitalPinToInterrupt(chocolateIntPin));
  
  if(awakenByVanillaInterrupt) handleVanillaInterrupt();
  if(awakenByChocolateInterrupt) handleChocolateInterrupt();
}

// The int handler will just signal that the int has happen
// we will do the work from the main loop.
void vanillaIntCallBack(){
  awakenByVanillaInterrupt=true;
}

// The int handler will just signal that the int has happen
// we will do the work from the main loop.
void chocolateIntCallBack(){
  awakenByChocolateInterrupt=true;
}

void handleVanillaInterrupt(){
  
  // Get more information from the MCP from the INT
  //uint8_t pin=vanillaMCP.getLastInterruptPin();
  //uint8_t val=vanillaMCP.getLastInterruptPinValue();

  uint16_t buttons = vanillaMCP.readGPIOAB();
  
  for(int i = 0; i < 12; i++){
    boolean tmp = (buttons & (1 << i)) ? 1 : 0;
    if(vanillaButtonValues[i] != tmp){
      Joystick.setButton(i, tmp);
      vanillaButtonValues[i] = tmp;
    }
  }

  Joystick.setHatSwitch(0, hatValues[(buttons >> 12) & 0x0F]);

  Joystick.sendState();//send the updated joystick states all at once

  // we have to wait for the interrupt condition to finish
  // otherwise we might go to sleep with an ongoing condition and never wake up again.
  // as, an action is required to clear the INT flag, and allow it to trigger again.
  // see datasheet for datails.
  //while( ! (vanillaMCP.digitalRead(vanillaMCPPinB) && vanillaMCP.digitalRead(vanillaMCPPinA) ));
  
  awakenByVanillaInterrupt=false;
}
