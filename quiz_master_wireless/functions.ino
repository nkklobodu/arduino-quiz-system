// send score to each slave
void sendAllScores() {
  for (int i = 1; i < 4; i++) { //loop through all three slaves
    radio.openWritingPipe(pipes[i]); //set radio to writing mode for current slave's address
    radio.stopListening(); //set module as transmitter
    int scorePayload = slaveObjects[i].score; //retrieve current slave's score
    radio.write(&scorePayload, sizeof(scorePayload)); //transmit score 
  }
  radio.startListening(); //set module as receiver after completion
}



// open reading pipes for all three slaves
void setReadingPipes() {
  radio.openReadingPipe(1, pipes[1]);
  radio.openReadingPipe(2, pipes[2]);
  radio.openReadingPipe(3, pipes[3]);
}



//display the scores on the LCD
void displayScore() {
  if (previousMode != mode) { //check if mode has changed since last call
    lcd.clear();  //clear the LCD display

    // Print the score for each slave on the first row of the display
    lcd.setCursor(0,0);
    lcd.print("1:");
    lcd.setCursor(2,0);
    lcd.print(slaveObjects[1].score);

    lcd.setCursor(6,0);
    lcd.print("2:");
    lcd.setCursor(8,0);
    lcd.print(slaveObjects[2].score);

    lcd.setCursor(12,0);
    lcd.print("3:");
    lcd.setCursor(14,0);
    lcd.print(slaveObjects[3].score);
    
    previousMode = mode;  //set previousMode to current mode
  }
}



// enter score for a particular slave with rotary encoder
void enterScore(uint8_t slave) {
  // clear the LCD display if mode has changed
  if (previousMode != mode) {
    lcd.clear();
    previousMode = mode;
  }

  if (lcdPrintOnce) { //print static info once
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print(slaveObjects[slave].id);//name of team + code name

    lcd.setCursor(2,0);
    lcd.print(slaveObjects[slave].name);//name of team + code name

    lcd.setCursor(0,1);
    lcd.print("SCORE: ");

    lcdPrintOnce = 0;
  }

  // properly display numbers entered using the rotary encoder
  if (rotaryScore < 0) {    // negative numbers
    lcd.setCursor(7,1);
    lcd.print(rotaryScore);
    lcd.setCursor(9,1);
    lcd.print(" ");
  }

  else {    // positive numbers
    // clear negative sign
    lcd.setCursor(7,1);
    lcd.print(" ");

    if (rotaryScore < 10) { //to solve some kind of error on the lcd
      lcd.setCursor(9,1);
      lcd.print(" ");
    }

    lcd.setCursor(8,1);
    lcd.print(rotaryScore);
  }
  
  // enter button adds slave's score, resets flags and rotary score
  if (buttonPressed(butEnter)) {
    slaveObjects[slave].add(rotaryScore);
    counter++;
    rotaryScore = 0;
    lcdPrintOnce = 1;
  }

  // escape button exits score mode (in case of a wrong buzz from slave; or if first slave received all the score)
  if (buttonPressed(butEsc)) counter = 3;
}



// ISR to increment or decrement score based on rotary encoder movement
void updateEncoder() {
  if (mode == 1) {  //executes only in score mode
    currentStateCLK = digitalRead(DT);  //read the current state of the CLK pin
    if (currentStateCLK != lastStateCLK){ //if the CLK state has changed
      if (digitalRead(CLK) != currentStateCLK) rotaryScore --;  //if rotated counter clockwise, decrement rotaryScore
      else rotaryScore ++;  //if rotated clockwise, increment rotaryScore
      rotaryScore = max(rotaryScore, -5); //constrain  minimum value of rotaryScore to -5
    }
    lastStateCLK = currentStateCLK; //store current state of the CLK pin as the last state for the next call
  }
}



// detect push button click with debouncing
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

