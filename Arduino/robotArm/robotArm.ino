#include "pinout.h"
#include "robotGeometry.h"
#include "interpolation.h"
#include "fanControl.h"
#include "RampsStepper.h"
#include "queue.h"
#include "command.h"

#include <Stepper.h>


Stepper stepper(2400, STEPPER_GRIPPER_PIN_0, STEPPER_GRIPPER_PIN_1, STEPPER_GRIPPER_PIN_2, STEPPER_GRIPPER_PIN_3);
RampsStepper stepperRotate(Z_STEP_PIN, Z_DIR_PIN, Z_ENABLE_PIN);
RampsStepper stepperLower(Y_STEP_PIN, Y_DIR_PIN, Y_ENABLE_PIN);
RampsStepper stepperHigher(X_STEP_PIN, X_DIR_PIN, X_ENABLE_PIN);
RampsStepper stepperExtruder(E_STEP_PIN, E_DIR_PIN, E_ENABLE_PIN);
FanControl fan(FAN_PIN);
RobotGeometry geometry;
Interpolation interpolator;
Queue<Cmd> queue(15);
Command command;
bool absolute=true;
int button=0;
int button_pressed=0;
float pos = -1 ; //-1=not armed 0=start, 100=scan end, 0.001-5.6=pos

void setup() {
  Serial.begin(115200);
  
  //various pins..
  pinMode(HEATER_0_PIN  , OUTPUT);
  pinMode(HEATER_1_PIN  , OUTPUT);
  pinMode(LED_PIN       , OUTPUT);
  
  //unused Stepper..
  pinMode(E_STEP_PIN   , OUTPUT);
  pinMode(E_DIR_PIN    , OUTPUT);
  pinMode(E_ENABLE_PIN , OUTPUT);
  
  //unused Stepper..
  pinMode(Q_STEP_PIN   , OUTPUT);
  pinMode(Q_DIR_PIN    , OUTPUT);
  pinMode(Q_ENABLE_PIN , OUTPUT);

  pinMode(X_MIN_PIN,INPUT);
  
  //GripperPins
  pinMode(STEPPER_GRIPPER_PIN_0, OUTPUT);
  pinMode(STEPPER_GRIPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_GRIPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_GRIPPER_PIN_3, OUTPUT);
  digitalWrite(STEPPER_GRIPPER_PIN_0, LOW);
  digitalWrite(STEPPER_GRIPPER_PIN_1, LOW);
  digitalWrite(STEPPER_GRIPPER_PIN_2, LOW);
  digitalWrite(STEPPER_GRIPPER_PIN_3, LOW);

  
  //reduction of steppers..
  stepperHigher.setReductionRatio(32.0 / 9.0, 200 * 16);  //big gear: 32, small gear: 9, steps per rev: 200, microsteps: 16
  stepperLower.setReductionRatio( 32.0 / 9.0, 200 * 16);
  stepperRotate.setReductionRatio(32 / 9.0, 400 * 16);
  stepperExtruder.setReductionRatio(32.0 / 9.0, 200 * 16);
  
  //enable and init..
  setStepperEnable(false);
  Serial.println("start");
}

void setStepperEnable(bool enable) {
  if(enable){
    Serial.println("echo Enable Stepper");
  }else{
    Serial.println("echo Disable Stepper");
  }
  stepperRotate.enable(enable);
  stepperLower.enable(enable);
  stepperHigher.enable(enable); 
  stepperExtruder.enable(enable); 
  fan.enable(enable);
}


void doscan(){
          Serial.println("Start scanning");
          stepperRotate.setSpeed(10);
          for (float i = 0; i < 5.50; i=i+0.0005){
            stepperRotate.stepRelativeRad(0.0005);
            stepperRotate.update();
            pos=i;
            int m = (i*1000) ;
            
            //Check cancel button
            if( m % 50 == 0){
              Serial.println(i);  
               button = digitalRead(X_MIN_PIN);
               if(button == LOW){
                //Interrupt
                Serial.println("echo Action Button pressed- CANCEL");
                stepperRotate.enable(false);               
                fan.enable(false);
                while(button == LOW){
                  delay(100);
                  button = digitalRead(X_MIN_PIN);
                  //wait for button to be released
                }
                return;
            }
          }
      }
}


void returntohome(){
            Serial.println("Back to home");
            setStepperEnable(true);
            stepperRotate.setSpeed(130);
            for (float i = pos; i > 0; i=i-0.0005){
   
              stepperRotate.stepRelativeRad(-0.0005);
              stepperRotate.update();
              pos=i;
              int m = (i*1000) ;
            
              //Check cancel button
              if( m % 50 == 0){
               Serial.println(i); 
               button = digitalRead(X_MIN_PIN);
               if(button == LOW){
                //Interrupt
                Serial.println("echo Action Button pressed- CANCEL");
                stepperRotate.enable(false);               
                fan.enable(false);             
                while(button == LOW){
                  delay(100);
                  button = digitalRead(X_MIN_PIN);
                  //wait for button to be released
                }
                return;
            }
          }
      }
      pos=0;
}

void actionButton(){
  button = digitalRead(X_MIN_PIN);
  while(button == LOW){
    Serial.println("echo Action Button low");
      button_pressed++;
      delay(50);
      button = digitalRead(X_MIN_PIN);
      //wait till  button is released
        //reset
      if(button_pressed > 50 ){
          Serial.println("echo Action long Button pressed");
          setStepperEnable(false);   
          pos=-1;
          delay(500);
      }
  }


  
  if(button_pressed > 1 && button_pressed < 50){
    Serial.println("echo Action Button pressed");
    Serial.println(pos);
    if(pos < 0){
      setStepperEnable(true); 
      pos=0;
      Serial.println("ARMED!!");
    }else if(pos == 0){
      doscan();
    }else if (pos > 0){
      //return to home
      returntohome();
    }
  }
  
  button_pressed=0;
}


void loop () {
  actionButton();
 
  if (!queue.isFull()) {
    if (command.handleGcode()) {
      queue.push(command.getCmd());
      
    }
  }
  if ((!queue.isEmpty()) ) {
    executeCommand(queue.pop());
    printOk();
  }
    
  if (millis() %500 <250) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}



void cmdDwell(Cmd (&cmd)) {
  delay(int(cmd.valueT * 1000));
}

void cmdStepperOn() {
  setStepperEnable(true);
}
void cmdStepperOff() {
  setStepperEnable(false);
}
void cmdFanOn() {
  fan.enable(true);
}
void cmdFanOff() {
  fan.enable(false);
}




void handleAsErr(Cmd (&cmd)) {
  printComment("Unknown Cmd " + String(cmd.id) + String(cmd.num) + " (queued)"); 
  printFault();
}

void executeCommand(Cmd cmd) {
  if (cmd.id == -1) {
    String msg = "parsing Error";
    printComment(msg);
    handleAsErr(cmd);
    return;
  }
  
  if (isnan(cmd.valueX)) {
    cmd.valueX=0;
    if(absolute){
      cmd.valueX = interpolator.getXPosmm();
    }
  }
  if (isnan(cmd.valueY)) {
    cmd.valueY=0;
    if(absolute){
      cmd.valueY = interpolator.getYPosmm();
    }
  }
  if (isnan(cmd.valueZ)) {
    cmd.valueZ=0;
    if(absolute){
      cmd.valueZ = interpolator.getZPosmm();
    }
  }
  if (isnan(cmd.valueE)) {
    cmd.valueE=0;
    if(absolute){
      cmd.valueE = interpolator.getEPosmm();
    }
  }
  
   //decide what to do
  if (cmd.id == 'M') {
    switch (cmd.num) {
      //case 0: cmdEmergencyStop(); break;
      case 17: cmdStepperOn(); break;
      case 18: cmdStepperOff(); break;
      case 105: Serial.print("T:0 B:0 "); break;
      case 106: cmdFanOn(); break;
      case 107: cmdFanOff(); break;
      default: handleAsErr(cmd); 
    }
  } else if (cmd.id == 'R') {
     switch (cmd.num) {
      case 1:
           doscan();
           break;
     case 2:
           returntohome();
           break;
     case 3:
         Serial.println("H-");
        stepperHigher.stepRelativeRad(-0.001);
        stepperHigher.update();
        break;
     case 4:
        Serial.println("H+");
        stepperHigher.stepRelativeRad(0.001);
        stepperHigher.update();
        break;
      case 5:
         Serial.println("L-");
        stepperLower.stepRelativeRad(-0.001);
        stepperLower.update();
        break;
     case 6:
        Serial.println("L+");
        stepperLower.stepRelativeRad(0.001);
        stepperLower.update();
        break;
        
  }
  } else {
    handleAsErr(cmd); 
  }
}
