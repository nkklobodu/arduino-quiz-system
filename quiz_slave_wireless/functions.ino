// checks whether a button has been clicked within the past 200 milliseconds
// returns true if the button has been clicked within that time, and false otherwise.

bool buttonPressed(uint8_t button) {
  // static variables to retain their values between function calls
  static bool prevButState = 1;
  static unsigned long lastTime = 0;
  bool butState = digitalRead(button); //read the current state of the button

  // check if button is clicked with a debounce of 200 milliseconds
  if (!butState && prevButState && (millis() - lastTime >= 200UL)) {
    prevButState = butState;  //update previous button state
    lastTime = millis();      //update last time counter
    return true;  //return true
  }

  // execute if the button is not clicked
  prevButState = butState;  //update previous button state
  return false;   //return false
}
