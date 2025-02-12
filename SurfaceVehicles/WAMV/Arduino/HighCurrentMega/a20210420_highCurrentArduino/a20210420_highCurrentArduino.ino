/* highCurrentArduino.ino
 *   Created by: Team Kanaloa (http://rip.eng.hawaii.edu/research/unmanned-x-systems/)
 *   LICENSE: Internal Kanaloa use only
 * 
 *   Master code for the WAMV low current arduino the code includes:
 *     - I2C communications between High current and Low current boxes
 *       - Send motor commands from low current to high current
 *       - Send high current battery voltage from high current to low current
 *     - Feedback control of motor voltages
 *     
 *   # I2C master. 
 *     This program communicates with the low current Arduino over I2C. 
 *     Motor commands are requests from the slave low current Arduino
 *     and High current battery voltages are sent from the high current
 *     Arduino to the low curent Arduino.
 * 
 *     Msg Template: 
 *       Q    : start flag
 *       1-4  : thruster number
 *       XXXX : thruster speed magnitude (0-1000)
 *       F/R  : direction & end flag
 * 
 *     Example Output per line: Q1540FQ240R Q31000FQ45R 
 * 
 *   # Motor Controllers.
 *     Convert motor controller inputs (i.e. the Qi commands provided by the I2C
 *     functions) desired motor voltage and control the servo shield PWM outputs 
 *     to set and maintain the desired motor voltage using a PID controller.
 *  
 *     Motor Control Command --> Desired Motor Voltage
 *       Motor voltage proportionally scaled between input maximums and motor
 *       voltage maximums
 *         proportional mapping: V_i = (V_max/Qmax) * Qi
 *     Generate motor control commands
 *       Adafruit PWM servo shield is used to generate the PWM signals used to 
 *       control the motor controllers
 *     Measure motor voltage
 *       Adafruit ADC breakout board ADS1115
 *     PID controller
 *       PID_V1 library
 * 
 *   # Version History
 *     2019.09.12 - Kai Jones (kaijones@hawaii.edu)
 *       - Initial Creatation of i2c communication
 *     2019.09.14 - A Trimble (atrimble@hawaii.edu)
 *       - Added motor controller
 *       - Added license statement
 *     2019.09.17 - A Trimble (atrimble@hawaii.edu)
 *       - Converted motor controller to object oriented
 *       - Updated some commenting throughout
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_ADS1015.h>
#include <kanaloaMotor.h>

// -------- CONSTANTS ----------------------
// I2C communications
#define SLAVE_ADDR 9        // arbitrary address number to identify slave Arduino
#define ANSWERSIZE 14       // number of bits expected to come from slave Arduino
// PWM Shield setup
#define ROS_BAUDRATE 57600
#define FREQ 50
// Voltage feedback addresses
#define ORANGE 0x48  // GND (DEFAULT)
#define GREEN 0x49   // VDD
#define BLUE 0x4A    // SDA 
#define BROWN 0x4B   // SCL
// Motor controller addresses
#define MC1 4,5
#define MC2 0,1
#define MC3 6,7
#define MC4 2,3

// --------- MESSAGE VARIABLES -------------
volatile char message[28];    // array used to store motor command message from slave Arduino as characters
String q1Msg;                 // string for parsing Q1 motor command
String q2Msg;                 // string for parsing Q2 motor command
String q3Msg;                 // string for parsing Q3 motor command
String q4Msg;                 // string for parsing Q4 motor command
String voltageMsg = "V";      // string for sending Voltage message to slave Arduino
int q1Motor = 0;              // Q1 motor command to send to motor controller
int q2Motor = 0;              // Q2 motor command to send to motor controller
int q3Motor = 0;              // Q3 motor command to send to motor controller
int q4Motor = 0;              // Q4 motor command to send to motor controller
char q1Dir;                   // variable to store character of Q1's motor direction
char q2Dir;                   // variable to store character of Q2's motor direction
char q3Dir;                   // variable to store character of Q3's motor direction
char q4Dir;                   // variable to store character of Q4's motor direction
bool debug = false;            // set to true to see printouts for messages

// Create the PWM Shield Object
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Create motor objects
kanaloaMotor q1 = kanaloaMotor(pwm, MC1, GREEN);
kanaloaMotor q2 = kanaloaMotor(pwm, MC2, ORANGE);
kanaloaMotor q3 = kanaloaMotor(pwm, MC3, BROWN);
kanaloaMotor q4 = kanaloaMotor(pwm, MC4, BLUE);

// Timer for sending main battery voltage
int motorCmdTimer = millis();    // timer variable to store time for requesting motor commands 
float motorCmdFreq = 10;         // number of times to request motorCmds from slave Arduino [Hz]
int voltageTimer = millis();     // timer variable to store time for sending voltage measurement
float voltageSendFreq = 4;       // number of times to send voltage measurement to slave Arduino [Hz]


// Variables for high battery voltage monitor
const int voltagePin = A1;    // pin for voltage measurement
int sensorValue;              // voltage measurement in bits (0-255)
float voltage;                // voltage measurement


void setup() {
  // Set up serial connection(s)
  Serial.begin(ROS_BAUDRATE);
    
  // join the I2C bus as a master (master device has no paramter)
  Wire.begin();

  // Initialize servo shield
  pwm.begin();
  pwm.setPWMFreq(FREQ);
  
  // Initialize motor objects
  q1.init();
  q2.init();
  q3.init();
  q4.init();
}

void loop() {
  
  // Update PID motor commands
  q1.pidUpdate();
  q2.pidUpdate();
  q3.pidUpdate();
  q4.pidUpdate(); 
  
  // Print debugging information
//    Serial.print("Q1 Debug -> ");
//     q1.debugOutput();
//     Serial.print("Q2 Debug -> ");
//     q2.debugOutput();
//     Serial.print("Q3 Debug -> ");
//     q3.debugOutput();
//    Serial.print("Q4 Debug -> ");
//    q4.debugOutput();

  // Request motor commands from slave Arduino at a set frequency
  if((millis()-motorCmdTimer) > (1/motorCmdFreq)){
    // Receive motor commands from slave Arduino
    requestFromSlave();
    parseMotorCmds();
  
    //Update the motor voltage commands
    q1.setVoltageSetpointFromQcommand(q1Motor);
    q2.setVoltageSetpointFromQcommand(q1Motor);
    
    q3.setVoltageSetpointFromQcommand(q2Motor);
    q4.setVoltageSetpointFromQcommand(q2Motor);
    
    // Reset timer
    motorCmdTimer = millis();
  }

  // Send main battery voltage to slave Arduino at a set frequency
  if((millis()-voltageTimer) > (1/voltageSendFreq)){
    // Measure mainBattVoltage
    getVoltage();
    
    // Send main battery voltage to slave Arduino every 
    sendVoltageMsg();
    
    // Reset timer
    voltageTimer = millis();
  }
}
