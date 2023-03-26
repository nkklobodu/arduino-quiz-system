// include libraries
#include <SPI.h>
#include <Wire.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>
#include "slave.h" // include the "slave.h" header file


// define peripherals and create objects
uint8_t butEnter = 4; //"Enter" button
uint8_t butEsc = 5;   //"Escape" button
uint8_t CLK = 2;      //rotary encoder CLK pin
uint8_t DT = 3;       //rotary encoder DT pin

LiquidCrystal_I2C lcd(0x27, 16, 2); //LCD display object (address, length, height)
RF24 radio(9, 8); //nRF24L01 radio module object (CE, CSN)


// define necessary variables
const uint64_t pipes[4] = { //all slave addresses
  0,  //unused
  0xF0F0F0F0E1LL,
  0xF0F0F0F0E2LL,
  0xF0F0F0F0E3LL
};
slave slaveObjects[] = {  //array of slave objects with ID and name
  slave(0, ""), //unused
  slave(1, "LEONADO"),
  slave(2, "MEGA"),
  slave(3, "NANO")
};

int mode = 0;                 //set the initial mode to 0 (default)
uint8_t request[2];           //create array to store request received from a slave
uint8_t code;                 //code part of the request
uint8_t slaveId;              //slave ID of the slave
uint8_t queue[3] = {0};       //queue of slave IDs in the order of received request
uint8_t counter = 0;          //counter to keep track of the current slave receiving score from master
bool previousMode = 1;        //flag to update the LCD screen on mode change
bool lcdPrintOnce = 1;        //flag to ensure static info is printed to LCD once
volatile int rotaryScore = 0; //score entered with the rotary encoder (volatile as it is updated by an ISR)
bool currentStateCLK;         //current state of the rotary encoder's CLK pin
bool lastStateCLK;            //last state of the rotary encoder's CLK pin


// declare functions
void sendAllScores();             //send each slave's scores
void setReadingPipes();           //open reading pipes for all three slaves
void displayScore();              //display scores on the LCD
void enterScore(uint8_t slave);   //enter score for a particular slave with rotary encoder
void updateEncoder();             //ISR to increment or decrement score based on rotary encoder movement
bool buttonPressed(uint8_t but);  //detect push button click with debouncing



void setup() {
  Serial.begin(500000); //initialize serial communication at 500000 baud rate  
  radio.begin();  //initialize NRF24L01 module
  
  lcd.init();       //initialize the LCD display
  lcd.clear();      //clear LCD screen
  lcd.backlight();  //turn on LCD backlight

  pinMode(butEnter,INPUT_PULLUP); //configure "Enter" button pin as input with pull-up resistor
  pinMode(butEsc,INPUT_PULLUP);   //configure "Escape" button pin as input with pull-up resistor
  
  pinMode(CLK,INPUT); //configure CLK pin as input
  pinMode(DT,INPUT);  //configure DT pin as input

  lastStateCLK = digitalRead(CLK);  //read the current state of the CLK pin and assign it to lastStateCLK

  // attach interrupts to the CLK and DT pins and call the updateEncoder function on change
  attachInterrupt(digitalPinToInterrupt(2), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), updateEncoder, CHANGE);

  sendAllScores();  //send score to each slave

  setReadingPipes();  //open reading pipes for all slaves
  // radio.setDataRate(RF24_1MBPS); //set the data rate to 1Mbps (commented out)
  radio.startListening(); //set the module as a receiver

}



void loop() {
  // debugging
  static unsigned long lastTime = 0;
  if (millis() - lastTime >= 10UL) {
    Serial.print("mode: ");
    Serial.print(mode);
    Serial.print("    ");

    Serial.print("counter: ");
    Serial.print(counter);
    Serial.print("    ");

    Serial.print("queue: ");
    Serial.print(queue[0]);
    Serial.print(".");
    Serial.print(queue[1]);
    Serial.print(".");
    Serial.print(queue[2]);
    Serial.print("    ");

    Serial.print("request: ");
    Serial.print(request[0]);
    Serial.print(".");
    Serial.print(request[1]);
    Serial.print("    ");

    Serial.print("previousMode: ");
    Serial.print(previousMode);
    Serial.print("   ");
    Serial.print("lcdPrintOnce: ");
    Serial.println(lcdPrintOnce);

    lastTime = millis();
  }



  // if queue is empty, set mode to 0 (idle), otherwise set mode to 1 (handle queued slaves)
  queue[0]==0 ? mode=0 : mode=1;

  // mode 0: idle
  if (mode == 0) displayScore();  //display the scores on the LCD

  // mode 1: handle queued slaves
  else {
    // if the counter 0, enter score for first queued slave
    if (counter == 0) enterScore(queue[0]);

    // if counter is 1 and the second queue is not empty, enter score for the second queued slave
    else if (counter == 1 && queue[1] != 0) enterScore(queue[1]);

    // if counter is 2 and the third queue is not empty, enter score for the third queued slave
    else if (counter == 2 && queue[2] != 0) enterScore(queue[2]);

    // else, exit score mode, resets queue, counter, flags and rotary score
    else {
      sendAllScores();  //send score to each slave
      queue[0] = queue[1] = queue[2] = 0; // reset the queue
      counter = 0; // reset queuing counter
      lcdPrintOnce = 1; // allow clearing of display
      previousMode = 1; // allow update lcd screen on mode change
      rotaryScore = 0; // reset rotary encoder score
    }
  }

  // handle incoming requests
  if (radio.available()) {
    radio.read(&request, sizeof(request));  //read request
    code = request[0];    //extract the code from  request
    slaveId = request[1]; //extract the slave ID from request

    // if code is 0 (slave requested his score)
    if (code == 0) {
      int scorePayload = slaveObjects[slaveId].score; //get score of the requested slave

      radio.openWritingPipe(pipes[slaveId]);  //set writing address to the requested slave's address
      radio.stopListening();  //set module as transmitter
      radio.write(&scorePayload, sizeof(scorePayload)); //write score to the requested slave

      setReadingPipes();  //set the reading pipes
      radio.startListening(); //set the module as receiver
    }

    // if code is 1 (slave pressed buzzer)
    // will not execute if all slaves are queued
    else if (queue[2] == 0) {
      // if first slot is empty, queue slave there
      if (queue[0] == 0) queue[0] = slaveId;

      // check if slave is already queued before queuing to avoiding duplication
      else if (queue[0] != slaveId && queue[1] == 0) queue[1] = slaveId;

      //avoid duplication of previous 2 slots
      else if (queue[0] != slaveId && queue[1] != slaveId) queue[2] = slaveId;
    }
  }
}


