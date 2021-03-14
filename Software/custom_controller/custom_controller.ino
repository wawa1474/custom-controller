#include <Joystick.h>

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  11, 1,                 // Button Count, Hat Switch Count
  true, true, true,      // X, Y, and Z
  true, true, false,     // Rx, Ry, no Rz
  false, false,          // no rudder or throttle
  false, false, false);  // no accelerator, brake, or steering

void setup() {
  Joystick.begin(false);//only update controller state when sendState() is called

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
}

void loop() {
  Joystick.setXAxis(1023 - analogRead(A2));
  Joystick.setYAxis(1023 - analogRead(A1));

  Joystick.setRxAxis(1023 - analogRead(A5));
  Joystick.setRyAxis(analogRead(A4));

  int z = 512 + (analogRead(A3) / 2) - (analogRead(A0) / 2);
  Joystick.setZAxis(z);

  Joystick.sendState();//send the updated joystick states all at once
}
