//PRODUCT_ID(1783)
//PRODUCT_VERSION(18)

#include <math.h>
#include "LIDARLite.h"
#include "SharpIR.h"
#include "PID.h"
#include <Servo.h>

#define model 20150
// ir: the pin where your sensor is attached
// model: an int that determines your sensor:  1080 for GP2Y0A21Y
//                                            20150 for GP2Y0A02Y
//                                            (working distance range according to the datasheets)
Servo wheels; // servo for turning the wheels
Servo esc; // not actually a servo, but controlled like one!
bool startup = true; // used to ensure startup only happens once
int startupDelay = 1000; // time to pause at each calibration step
double maxSpeedOffset = 45; // maximum speed magnitude, in servo 'degrees'
double maxWheelOffset = 85; // maximum wheel turn magnitude, in servo 'degrees'
double distance5, distance6;
double prevDistanceLidar;
double prevDistanceIR;
int dis2 = 0;
int dis;

int wheel_write;
bool initial;
int go = 1;

bool PIDAppliedOnLIDAR = false;

int countGaps = 0;


int start(String command) {
  if(command == "go") {
    go = 1;
    return 1;
  }
  else if(command == "no") {
    go = 0;
    return 0;
  }
}

SharpIR ir(A0, 20150);
SharpIR ir2(A1, 20150);
LIDARLite lidar;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID = PID(&Input, &Output, &Setpoint,2,1,1, DIRECT);

bool remoteControlling = false;

void leftTurn1(){
//  wheels.write(90);
//  esc.write(125);
//  delay(
  wheels.write(180);
  delay(10);
  esc.write(60);
  delay(1000);
  esc.write(90);
  delay(25);
  wheels.write(0);
  delay(10);
  esc.write(120);
  delay(1000);
  esc.write(90);
  delay(25);
  wheels.write(140);
  delay(10);
  esc.write(60);
  delay(1500);

  //FIX POSITION
  wheels.write(70);
  delay(10);
  esc.write(60);
  delay(1000);
  
  wheels.write(90);
  delay(10);
  esc.write(90);
  delay(1000);
}

void leftTurn(){
//  wheels.write(90);
//  esc.write(125);
//  delay(
  esc.write(90);
  delay(100);
  esc.write(40);
  delay(10);
  wheels.write(180);
  delay(10);

  delay(2000);
  esc.write(90);
  delay(25);

//  wheels.write(0);
//  delay(10);
//  esc.write(120);
//  delay(1000);
//  esc.write(90);
//  delay(25);
//  wheels.write(180);
//  delay(10);
//  esc.write(60);
//  delay(1500);
//  wheels.write(90);
//  delay(10);
//  esc.write(90);
//  delay(1000);
}


void setup()
{
  // For serial communication with raspberry pi
  Serial.begin(9600);    // USB port
  
  wheels.attach(2); // initialize wheel servo to Digital IO Pin #2
  esc.attach(3); // initialize ESC to Digital IO Pin #3

  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  wheels.write(180);
  delay(1000);
  wheels.write(0);
  delay(1000);
  wheels.write(90);
  delay(1000);

  //calibrateESC();

  digitalWrite(5, HIGH);
  digitalWrite(6, LOW);
  delay(20);
  lidar.begin(0, true);
  lidar.configure(0);
  lidar.changeAddress(LIDARLITE_ADDR_SECOND, true, LIDARLITE_ADDR_DEFAULT);

  digitalWrite(6, HIGH);
  delay(20);
  lidar.configure(0);

  initial = true;
  wheels.write(90);
  //esc.write(90);

  dis2 = ir2.distance();
  prevDistanceIR = dis2;
  //////

  
  //PID initalization
  distance5 = lidar.distance(true, LIDARLITE_ADDR_SECOND)*1.00; //Left
  delay(10);
  distance6 = lidar.distance(true, LIDARLITE_ADDR_DEFAULT)*1.00; //Right
  prevDistanceLidar = distance5;
  myPID.SetOutputLimits(-90, 90); // Servo angles limits for output of PID to scale properly
  myPID.SetMode(AUTOMATIC);
  
  delay(10);
}

/* Convert degree value to radians */
double degToRad(double degrees){
  return (degrees * 71) / 4068;
}

/* Convert radian value to degrees */
double radToDeg(double radians){
  return (radians * 4068) / 71;
}

/* Calibrate the ESC by sending a high signal, then a low, then middle.*/
void calibrateESC(){
    esc.write(180); // full backwards
    delay(startupDelay);
    esc.write(0); // full forwards
    delay(startupDelay);
    esc.write(90); // neutral
    delay(startupDelay);
    esc.write(90); // reset the ESC to neutral (non-moving) value
}

String rx_data;
void loop()
{
  // get line
  while(Serial.available() > 0)
  {
    char rx_byte = Serial.read();
    rx_data += rx_byte;
    
    // check for newline
    if(rx_byte == '\n')
    {
      ////// string finished /////
      if(rx_data[0] == 'r') { // remote control
        remoteControlling = true;
        bool readingWheelAngle = true; // we start by reading the wheel angle
        
        /* variables for wheels */
        String wheel_angle_str;
        /* variables for esc */
        String esc_angle_str;
        int size_esc_angle = 0;
        
        /*** PARSING COMMAND SET FROM NODE.JS ***/
        for(int i=1; i<rx_data.length(); i++) {
          if(rx_data.charAt(i) != ',') {
            if(readingWheelAngle) {
              wheel_angle_str.concat(rx_data.charAt(i));
            }
            else {
               esc_angle_str.concat(rx_data.charAt(i));
            }
          }
          else {
            readingWheelAngle = false;
          }
        }
        /*** DONE PARSING ***/
        /*** GETTING THE ANGLE VALUES ***/
        int esc_val = esc_angle_str.toInt();
        int wheel_val = wheel_angle_str.toInt();
        //Serial.println("esc: " + esc_val);
        //Serial.println("wheel: "+wheel_val);
        esc.write(esc_val);
        wheels.write(wheel_val);
      }
      else if(rx_data[0] == 'a')
      {
        remoteControlling = false;
      }
      
      //Serial.print(rx_data);
      rx_data = "";
    }
  }
  
  if(!remoteControlling)
  {
    if(go) {
      
      esc.write(60);
      // IR collision detection
      dis = ir.distance();  // this returns the distance to the object you're measuring 
      delay(10);


      // Calculate input to PID control for straight navigation
      distance5 = lidar.distance(true, LIDARLITE_ADDR_SECOND)*1.00; //Left
      delay(10);
      distance6 = lidar.distance(true, LIDARLITE_ADDR_DEFAULT)*1.00; //Right
      delay(10);

      dis2 = ir2.distance();
      delay(10);

      if(countGaps == 1){
        PIDAppliedOnLIDAR = true;
      }
      else if (countGaps == 2){
        PIDAppliedOnLIDAR = false;
      }
      else if (countGaps == 3){
        PIDAppliedOnLIDAR = false;
      }
      else{
        PIDAppliedOnLIDAR = false;
      }
       
     
      //Print for debugging
      //Serial.print("LIDAR 1: ");
    
      //Serial.print("LIDAR 2: ");
      //Serial.println(distance6);
      //Serial.println("LEFT IR: ");
      //Serial.println(dis2);
      //Serial.println("FRONT IR: ");
      //Serial.println(dis);

      //if(PIDAppliedOnLIDAR)
        //Serial.println("USING LIDAR");
      // else
        //Serial.println("USING IR");

        //Serial.println(distance5);
      //Serial.println(dis2);
      
      if(((dis2-prevDistanceIR) > 35) && !PIDAppliedOnLIDAR){ // gab detected on the left
          countGaps++;
          if(countGaps == 1) {
            //PIDAppliedOnLIDAR = true;
            leftTurn();
          }
          else if(countGaps == 2) {
            //PIDAppliedOnLIDAR = false;
            leftTurn(); 
          }
          else if(countGaps == 3) {
            //PIDAppliedOnLIDAR = true;
            //leftTurn();
          
            
            delay(100);
               wheels.write(180);
               delay(10);
            esc.write(40);
            delay(10);
          
            delay(1250);
            esc.write(90);
            delay(25);

             wheels.write(0);
            esc.write(120);
            delay(1500);
            esc.write(90);

              delay(100);
               wheels.write(180);
               delay(10);
            esc.write(40);  
            delay(1250);
            wheels.write(0);
            delay(10);
            esc.write(50);
            delay(1500);
            esc.write(90);
            delay(10);
            wheels.write(90);
            delay(10);
            
            
          }
          else if(countGaps == 4) {
            //PIDAppliedOnLIDAR = false;
            leftTurn();
            countGaps = 0;
          }
      }
      else if(((distance5-prevDistanceLidar) > 40) && PIDAppliedOnLIDAR) { // gab detected on right
          countGaps++;
          if(countGaps == 1) {
            //PIDAppliedOnLIDAR = false;
          }
          else if(countGaps == 2) {
            //PIDAppliedOnLIDAR = true;
            leftTurn();
          }
          else if(countGaps == 3) {
            //PIDAppliedOnLIDAR = false;
            leftTurn();
          }
          else if(countGaps == 4) {
            //PIDAppliedOnLIDAR = false;
            leftTurn();
            countGaps = 0;
          }
          //PIDAppliedOnLIDAR = false;
      }

      //Serial.println(countGaps);
      // if bin 9 && lidar=true 
              //switch to ir

      //if bin 7 &&  ir=true 
              //switch to lidar
              

      
      if(PIDAppliedOnLIDAR) { // LIDAR PID
        Input = (distance5 + distance6) / 2.00; 
        Setpoint = 90;
        delay(10);
        
      }
      else { // IR PID
        Input = dis2;
        Setpoint = 60;
        delay(10);
        
      }
      
      //COMPUTE PID AND WRITE TO SERVOS
      myPID.Compute();

      //Serial.print("Input: ");
      //Serial.println(Input);
      //Serial.println("Output: ");

      if(PIDAppliedOnLIDAR){
        wheels.write(90 + Output);  
        //Serial.println(90 + Output);
        }
      else if(!PIDAppliedOnLIDAR){
        wheels.write(90 - Output);
        //Serial.println(90 - Output);
      }
      
      if(dis < 35)
      {
        wheels.write(90);
        esc.write(110); //slower backwards
        delay(1000);
        esc.write(90);
        //start("no");
      }

      
      prevDistanceLidar = distance5;
      prevDistanceIR = dis2;  
      delay(10);
    }
    if(!go) {
      esc.write(90); //Stop wheels
    }
  }
  
//  if(Serial.available() > 0) {
//    String command_from_js;
//    command_from_js = Serial.readString();
//    Serial.println("received command: " + command_from_js);
    
    
//    // command is automatic (only sending the bin number)
//    else {
//      String bin_num_str;
//      for(int i=1; i<15; i++) {
//        if(isDigit(command_from_js.charAt(i)))
//          bin_num_str.concat(command_from_js.charAt(i));
//      }
//      int bin_num = bin_num_str.toInt();
      
    //}
  //}
//  }
}
