// include necessary libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SevSeg.h>


// define slave ID as a constant
// 3 transmitters for this project ie. (1 <= slaveID <= 3)
const byte slaveID = 2;


// define peripherals & create objects
uint8_t butPin = 2;   //push button
uint8_t buzzPin = 3;  //buzzer
RF24 radio(9, 8);   //nRF24L01 radio module object (CE, CSN)
SevSeg sevseg;      //seven segment display


// define variables
const uint64_t pipes[] = { //all slave addresses
  0,  //unused
  0xF0F0F0F0E1LL,
  0xF0F0F0F0E2LL,
  0xF0F0F0F0E3LL
};
uint8_t data[2];        //data to be transmitted
int score;              //recieved score
bool transmitMode = 0;  //flag to enable data transmission


// declare functions
bool buttonPressed(uint8_t but);  //detect push button click with debouncing



void setup() {
  // Serial.begin(115200); //initialize serial communication at 115200 baud rate
  radio.begin(); //initialize NRF24L01 module

  //seven segment display
  byte numDigits = 2;     //2 digit display
  byte digitPins[] = {5, A5};   //D2 D1  //pin numbers for each digit on the display
  byte segmentPins[] = {7, 4, A3, A1, A0, 6, A4, A2};   // A B C D E F G DP  //pin numbers of segments on the display
  bool resistorsOnSegments = true;    //resistors are connected to segment pins
  byte hardwareConfig = COMMON_CATHODE;   //display is common cathode
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments); //initialize seven-segment display
  sevseg.setBrightness(100); //set brightness of the display

  pinMode(butPin, INPUT_PULLUP); //configure button pin as input with pull-up resistor
  pinMode(buzzPin, OUTPUT); //configure buzzer pin as output

  data[1] = slaveID; //set transmission slave id
  radio.openWritingPipe(pipes[slaveID]); //set address for the NRF24L01 module
  // radio.setDataRate(RF24_1MBPS); //set data rate to 1Mbps
  radio.stopListening(); //set NRF24L01 module as transmitter

  // Request score from the master and wait for response
  data[0] = 0; //code for request score is 0
  bool success = 0; // variable to determine if request transmission was successful
  do {
    success = radio.write(&data, sizeof(data)); //send request to master
    delay(5);
  } while(!success); //keep resending the request until successful

  // Receive the score from the master
  radio.openReadingPipe(0, pipes[slaveID]); //set address for NRF24L01 module
  radio.startListening(); //set NRF24L01 module as receiver
  while (!radio.available()) {}; //wait for data to be available
  radio.read(&score, sizeof(score)); //read score from master

  radio.openWritingPipe(pipes[slaveID]); //set address for the NRF24L01 module
  radio.startListening(); //set module as receiver

}



void loop() {
  // constantly display score on the seven segment display
  sevseg.setNumber(score);
  sevseg.refreshDisplay();

  // activate buzzer when button is pressed
  digitalWrite(buzzPin, !digitalRead(butPin));

  // if push button is pressed, set transmitMode to 1
  if (buttonPressed(butPin)) transmitMode = 1;

  // if transmitMode is 1, transmit to master
  if (transmitMode) {
    radio.stopListening();  //set module as transmitter
    data[0] = 1;  //code for buzzer pressed
    if (radio.write(&data, sizeof(data))) { //transmit data to master, and if successful 
      transmitMode = 0; //set transmitMode to 0
      radio.startListening(); //set module as receiver
    }
  }

  // if transmitMode is 0, receive score from master
  else {
    if (radio.available()) radio.read(&score, sizeof(score)); //if data is available, read into score variable
  }
  
}

