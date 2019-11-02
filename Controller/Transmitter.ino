/* This is the transmitter coder for Project Stella
 * Contributors: Damien Gilbert, Dan Even, Aden Prince, Niraj Salunkhe
 * Last Date Modified: 10/30/2019
 */
/*Libraries*/
#include <SPI.h>      // SPI 
#include <nRF24L01.h> // RF Transciever 
#include <RF24.h>     // RF Transceiver
#include <LiquidCrystal_I2C.h> // LCD Library
//============================================================================================================

/*Global Variables*/
#define CE_PIN  7     // Chip Enable to digital pin 7
#define CSN_PIN 8     // Chip Select Not to digital pin 8
#define LEFT_Y A0     // Left Y-axis to analog pin A0
#define RIGHT_Y A1    // Right Y-axis to analog pin A1

//============================================================================================================

const byte slaveAddress[5] = {'R','x','A','A','A'};   // Address for slave (reciever)
const int MAX = 1023;         // Maximum value for joystick, 10 bit resolution
unsigned long currentMillis;  // current time in millisecond
unsigned long prevMillis;     // previous time in millisecond
unsigned long txIntervalMillis = 10;    // send once per second
unsigned long centerR = 0, centerL = 0; // initial centers for joysticks

const short buttonPin = 2;  // The pin the button is plugged into
short buttonState = 0; // Used to detect when the button state changes
short lastButtonState = 0; // Used to detect when the button state changes
short menuNumber = 0; // The value each menua is associated with
int bruhNumber = 0; // Bruh number
/* Menus:
 * 0: Joystick 1 and 2 values
 * 1: Bruh number display
 * 2: Ampersand
 */

//============================================================================================================

/*Instances*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//============================================================================================================

/*Data Structure*/
struct Data_Package {
  int rightTrack, leftTrack;
};
Data_Package data; // Create a variable with the above structure

//============================================================================================================

void setup() {
    Serial.begin(9600);   // Set baud rate for serial monitor
    Serial.println("Tx Starting");  // Beginning Prompt
    radio.begin();  //Communication with radio
    radio.setDataRate( RF24_250KBPS );    // Set data rate to 250kilobits per second
    radio.setRetries(3,5); // delay, count
    radio.openWritingPipe(slaveAddress);  // Begin writing radio 
    calibrate();  // Do calibration to find nominal centers of the joysticks

    lcd.init(); // Initialize the lcd 
    lcd.backlight(); // Start lcd backlight
    printJoystickLabels(); // Prints the joystick labels

    pinMode(buttonPin, INPUT); // Set pin mode for the button
}

//============================================================================================================

void calibrate() {
    centerR = 0;  // initialize center right to zero (again?)
    centerL = 0;  // initialize center left to zero (again?)
    
    //accumulate data points
    for (int i = 0; i < 512; ++i) {
      centerR += analogRead(RIGHT_Y); // add up all middle values 512 times for right joystick
      centerL += analogRead(LEFT_Y);  // add up all middle values 512 times for left joystick
    }
    
    //bitshift 9 to the right to divide by 512
    centerR = centerR >> 9;  // take the average of right joystick (nominal)
    centerL = centerL >> 9; // take the average of left joystick (nominal)
}

//============================================================================================================

void loop() {
    currentMillis = millis();   // current time is set to milliseconds
    //Check clock for sending message
    if (currentMillis - prevMillis >= txIntervalMillis) {
        send();
        prevMillis = millis();
    }
}

//============================================================================================================

// NOTE: is send() a built-in function?
void send() {
    bool rslt;  // Boolean variable for verification of receiving message
    //data.rightTrack = 1;    // Tester
    //data.leftTrack = 1;     // Tester
    rslt = radio.write( &data, sizeof(data) );      // will return true if message is verified as received

    // Printable Table for Data Being Sent
    Serial.print("Data Sent ");
    Serial.print(data.rightTrack);
    Serial.print("\t");
    Serial.print(data.leftTrack);
    Serial.print("\t");

    // Get the button state
    buttonState = digitalRead(buttonPin);
  
    // This if statement is entered only once, the first instant the button is pressed
    if (buttonState == HIGH && lastButtonState == LOW) {
      // Toggles between each menu number
      menuNumber++;
      if(menuNumber == 3) {
        menuNumber = 0;
    }
    
      // Clears the lcd and prints labels
      // This is done once so the labels won't flicker
      lcd.clear();
      printLabels();
    }
    // Prints the data every loop iteration
    printData();
  
    // Reads the button state again so we can detect a button press
    lastButtonState = digitalRead(buttonPin);

    // Acknowledgement Checks
    if (rslt) {
        Serial.println("  Acknowledge received");
        getDataPackage();
    }
    else {
        Serial.println("  Tx failed");
    }
}

//============================================================================================================

void getDataPackage() {   
    int rRead = analogRead(RIGHT_Y);  // read right joystick value from analog pin
    int lRead = analogRead(LEFT_Y);   // read left joystick value from analog pin

    // Check Left Joystick Value and Map Accordingly
    if(lRead < centerL) {
      data.leftTrack = map(lRead, 0, centerL, 0, 511);
    } else {
      data.leftTrack = map(lRead, centerL, 1023, 512, 1023);
    }

    // Check Right Joystick Valule and Map Accordingly
    if(rRead < centerR) {
      data.rightTrack = map(rRead, 0, centerR, 0, 511);
    } else {
      data.rightTrack = map(rRead, centerR, 1023, 512, 1023);
    }
}

//============================================================================================================

/*Prints labels based on the menu number*/
void printLabels() {
  if(menuNumber == 0) {
    printJoystickLabels();
  }
  else if(menuNumber == 1) {
    printBruhLabels();
  }
  else if(menuNumber == 2) {
    printAmpersandLabels();
  }
}

//============================================================================================================

/*Prints data based on the menu number*/
void printData() {
  if(menuNumber == 0) {
    printJoystickData();
  }
  else if(menuNumber == 1) {
    printBruhData();
  }
  else if(menuNumber == 2) {
    printAmpersandData();
  }
}

//============================================================================================================

/*Prints joystick menu labels*/
void printJoystickLabels() {
  lcd.setCursor(0,0);
  lcd.print("j1PotY: ");
  lcd.setCursor(0,1);
  lcd.print("j2PotY: ");
}

//============================================================================================================

/*Prints joystick data*/
void printJoystickData() {
  // Prints j1PotY
  lcd.setCursor(8,0);
  lcd.print("   ");
  lcd.setCursor(8,0);
  lcd.print(data.rightTrack);

  // Prints j2PotY
  lcd.setCursor(8,1);
  lcd.print("   ");
  lcd.setCursor(8,1);
  lcd.print(data.leftTrack);
}

//============================================================================================================

/*Prints bruh menu labels*/
void printBruhLabels() {
  lcd.setCursor(0,0);
  lcd.print("Bruh Number: ");
}

//============================================================================================================

/*Prints bruh data*/
void printBruhData() {
  lcd.setCursor(0,1);
  lcd.print(bruhNumber);

  // Updates the bruh number
  bruhNumber++;
}

//============================================================================================================

/*Prints ampersand menu labels*/
void printAmpersandLabels() {
  lcd.setCursor(0,0);
  lcd.print("Ampersand:");
}

//============================================================================================================

/*Prints ampersand data*/
void printAmpersandData() {
  lcd.setCursor(0,1);
  lcd.print("&");
}
