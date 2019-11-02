/* This is the reciever code for Project Stella
 * Contributors: Damien Gilbert, Dan Even
 * Last Date Modified: 10/27/2019
 */
/*Libraries*/
#include <SPI.h>      // SPI
#include <nRF24L01.h> // RF Transciever
#include <RF24.h>     // RF Transciever
#include "RoboClaw.h" // Roboclaw 
#include <Wire.h>     // Wire

//============================================================================================================

/*Global Variable*/
#define CE_PIN  7     // Chip Enable to digital pin 7
#define CSN_PIN 8     // Chip Select Not to digital pin 8
#define address2 0x80 // Address to roboclaw (must match inside Motion Studio)

//============================================================================================================

const byte thisSlaveAddress[5] = {'R','x','A','A','A'};  //Address for master (transmitter)
const int MAX = 1024;       // Max value of joysticks, 10bit resolution
float k = 0.01;            // Determines "steepness" of logistic curve
float x_0 = 512;            // Initial value of joysticks (middle)
float upper_bound = 32767;  // Maximum upper bound for logistic curve
float lower_bound = -32767;      // Maximum lower bound for logistic curve
float e = 2.718281828459045;// Approximate value for the 'e'
int table[1024];            // Initialized table array
bool newData = false;
//bool valid;               // Boolean for reading voltage from roboclaw

//============================================================================================================

// Instances
RoboClaw roboclaw(&Serial3, 10000); //Connecting TX, RX to roboclaw object
RF24 radio(CE_PIN, CSN_PIN);

//============================================================================================================

/*Data Structure*/
struct Data_Package {
  int rightTrack, leftTrack;
};
Data_Package data; //Create a variable with the above structure

//============================================================================================================

void setup() {
    Serial.begin(57600);     // Set baud rate for serial communication
    Serial3.begin(38400);   // Set baud rate for serial communication to roboclaw (must match inside Motion Studio)

    Serial.println("Rx Starting");  // Beginning Prompt
    radio.begin();  // Begin radion communication
    radio.setDataRate( RF24_250KBPS );  // Set data rate to 250kilobits per second
    radio.openReadingPipe(1, thisSlaveAddress);  // Begin listening radio communication
    radio.startListening();   // Radio begins listening

    buildTable();  //Build logistic table for joystick mapping
}

//============================================================================================================

void resetData() {
  // Reset the values when there is no radio connection - Set initial default values
  data.rightTrack = 512; //Set to middle
  data.leftTrack = 512; //Set to middle
}

//============================================================================================================

int logistic(float input) {
    //returns nearest integer for logistic sigmoid function 
    //with ranges and constants declared as globals up top
    int result = (lower_bound + (upper_bound - lower_bound)/(1+pow(e, (-k*(input-x_0)))));
    return result;
}

//============================================================================================================

void buildTable() {
  //Create lookup table for sigmoid map 
  for (int i = 0; i < MAX; ++i) {
    table[i] = logistic(i);
    //Printable Table (comment in for testing, comment out for running)
//    Serial.print("f(");
//    Serial.print(i);
//    Serial.print(")= ");
//    Serial.println(table[i]);
  }
}

//============================================================================================================

void loop() {
    //Call all functions to main loop
    getData();
    processData();
    showData();
    sendMotorControl();
}

//============================================================================================================

void getData() {
    if ( radio.available() ) {
        radio.read( &data, sizeof(data) );
        newData = true;
    }
}

//============================================================================================================

void processData() {
  //Pull sigmoid values from lookup table where table[x] = f(x)
  data.rightTrack = table[data.rightTrack];
  data.leftTrack = table[data.leftTrack];

  // Deadband for the Middle
  /*(if(data.rightTrack < 10 && data.rightTrack > -10){
    data.rightTrack = 0;
  }
  if(data.leftTrack < 10 && data.leftTrack > -10){
    data.leftTrack = 0;
  }*/
  
//  // Fix backward values
//  if(data.rightTrack < 0){
//    data.rightTrack = 32767 + abs(data.rightTrack);
//  }
//  if(data.leftTrack < 0){
//    data.leftTrack = 32767 + abs(data.leftTrack);
//  }
}

//============================================================================================================

void showData() {
    if (newData == true) {
        Serial.print("Data received ");
        Serial.print(data.rightTrack);
        Serial.print("\t");
        Serial.println(data.leftTrack);

        // Commented out since we do not need this right now
//        Serial.print("\t");
//        Serial.print("; Main Battery Voltage: ");
//        Serial.println(roboclaw.ReadMainBatteryVoltage(address2 ,&valid));
        newData = false;
    }
}

//============================================================================================================

void sendMotorControl() {
  delay(10);
  roboclaw.DutyM1(address2, data.rightTrack);
  delay(10);
  roboclaw.DutyM2(address2, data.leftTrack);
  delay(20);
}
