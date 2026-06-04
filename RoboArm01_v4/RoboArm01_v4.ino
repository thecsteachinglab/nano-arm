#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>

#define VERSION 0.5 //laser version
 
// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


// Depending on your servo make, the pulse width min and max may vary, you 
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to twea k them as necessary to match the servos you
// have!
#define SERVOMIN  140 // micro servo this is the 'minimum' pulse length count (out of 4096)
//#define SERVOMAX  600 // micro servo this is the 'maximum' pulse length count (out of 4096)
#define SERVOMAX  500


/* Robo Arm 01 definiions */
#define NUM_SERVOS 4

//#define TWO_BASE_MOTORS 2 /* 3D printed arm has 2 base motors, metal arm has 1 base motor */
#define ONE_BASE_MOTORS 1

#define ASCII_TO_INT 48

#define DEBUG 1  //log messages 

#define SERVO_START_ANGLE 90
uint8_t servo_positions[NUM_SERVOS];
uint8_t curr_servo = 0;
uint8_t gServoSpeed = 20; //the lower the faster
char servo_status[NUM_SERVOS]; //'p'-moving positive 'n' -moving negative 's'-stopped



// Calculate based on max input size expected for one command
#define INPUT_SIZE 128


/* Commands */
#define MOVE_NEGATIVE 'n'
#define MOVE_POSITIVE 'p'
#define MOVE_STOP 's'


/*numbers sent from app. anything from 0 to NUM_SERVOS is to move the servo */
#define APP_CMD_STOP 10
#define APP_CMD_SETSPEED 11
#define APP_CMD_RESET 12
#define APP_CMD_GET_VERSION 14
#define APP_CMD_SAVE_SERVOS 15 //saves the current servos into eeprom so when it loas up it uses these values

/*EEPROM values*/
#define STORED_VALUES 128

//joystick
const int SW_pin = A2; // digital pin connected to switch output
const int X_pin = A0; // analog pin connected to X output
const int Y_pin = A1; // analog pin connected to Y output


byte previous_press = 1; //input held high
byte current_press = 1;

#define JOY_CONTROL_LOW 0 //joystick controls lower half of arm base and low motor
#define JOY_CONTROL_HIGH 2 

byte joy_control = JOY_CONTROL_LOW;

#define CONTROL_MODE_JOY 1
#define CONTROL_MODE_SERIAL 2 //controlled by serial command

byte control_mode = CONTROL_MODE_JOY;//default control mode 

void SetServosSync(short servo_num1,short servo_num2,short degrees, short servo_speed = 0)
{
    short reverse = 0;
    short reverse_degrees = 0;
    short reverse_pos = 0;

    reverse_degrees  = 180 - degrees;
   
    short reverse_pulse = map(reverse_degrees, 0, 180, SERVOMIN, SERVOMAX);
    short pulselength = map(degrees, 0, 180, SERVOMIN, SERVOMAX);
    
    
    short curr_pos = servo_positions[servo_num1];
    reverse_pos = servo_positions[servo_num2];

    short target_pos = degrees;
    short target_delta = degrees -  curr_pos;
  
    target_delta = abs(target_delta);
    
    if(servo_speed == 0)
    {
      pwm.setPWM(servo_num1, 0, pulselength);
      pwm.setPWM(servo_num2, 0, reverse_pulse);

      servo_positions[servo_num1] = degrees;
      servo_positions[servo_num2] = reverse_degrees;
    }
    else
    {
      if(target_pos > curr_pos)
      {
        for(int i=0;i<target_delta;i++)
        {
          pulselength = map(curr_pos+i, 0, 180, SERVOMIN, SERVOMAX);
          reverse_pulse = map(reverse_pos-i, 0, 180, SERVOMIN, SERVOMAX);
          pwm.setPWM(servo_num1, 0, pulselength);
          pwm.setPWM(servo_num2, 0, reverse_pulse);
          delay(servo_speed);
        }
        servo_positions[servo_num1] = degrees;
        servo_positions[servo_num2] = reverse_degrees;
      }
      else if(target_pos < curr_pos)
      {
        for(int i=0;i<target_delta;i++)
        {
          pulselength = map(curr_pos-i, 0, 180, SERVOMIN, SERVOMAX);
          reverse_pulse = map(reverse_pos + i, 0, 180, SERVOMIN, SERVOMAX);
          pwm.setPWM(servo_num1, 0, pulselength);
          pwm.setPWM(servo_num2, 0, reverse_pulse);
          delay(servo_speed);
        }
        servo_positions[servo_num1] = degrees;
        servo_positions[servo_num2] = reverse_degrees;
      }
      
    }
    
}

void SetServoAngle(short servo_num, short degrees, short servo_speed)
{
   
    
  if(servo_num < 1 || servo_num > NUM_SERVOS) //validte servo number
  {
    Serial.print("invalid servo number ");
    Serial.println(servo_num);
    return;
  }

  short reverse = 0;
  short reverse_degrees = 0;
  short servo = 0;

  servo = servo_num - 1; // servos start from index 0
  short curr_pos = servo_positions[servo];
  short pulselength = 0;

  /* move servo 1 & 2 in sync*/
  #ifdef TWO_BASE_MOTORS
  
  if( servo == 1 || servo == 2 )
  {

    SetServosSync(1,2,degrees,gServoSpeed);
    return;
  }
  #endif


  /*
  if(servo_speed == 0)
  {
     servo_positions[servo] = degrees;
     pulselength = map(degrees, 0, 180, SERVOMIN, SERVOMAX);
     pwm.setPWM(servo, 0, pulselength);
     return;
  }
  */
  
  short target_pos = degrees;
  short target_delta = degrees -  curr_pos;
  
  target_delta = abs(target_delta);

  
  
  if(target_pos > curr_pos)
  {
    for(int i=0;i<target_delta;i++)
    {
        pulselength = map(curr_pos+i, 0, 180, SERVOMIN, SERVOMAX);
        pwm.setPWM(servo, 0, pulselength);
        delay(servo_speed);
    }
    servo_positions[servo] = degrees;
  }
  else if(target_pos < curr_pos)
  {
    for(int i=0;i<target_delta;i++)
    {
        pulselength = map(curr_pos-i, 0, 180, SERVOMIN, SERVOMAX);
        pwm.setPWM(servo, 0, pulselength);
        delay(servo_speed);
    }
    servo_positions[servo] = degrees;
  }
  return;
}



void ReadCommand()
{
  // Get next command from Serial (add 1 for final 0)
  
  byte data = Serial.read();
  byte cmd = data;
  byte dir = 0;

  //Serial.print("Data recieved ");
  //Serial.println(data,DEC);

  data = data - ASCII_TO_INT;

  #ifdef DEBUG
  if( !(data >= 1) and (data<=NUM_SERVOS) )
  {
   // Serial.println(data,DEC);
  }
  #endif
  if(cmd == 'm') //switch mode from joystick to serial
  {
    control_mode = CONTROL_MODE_SERIAL;
    return;
  }
  if( (data >= 1) and (data<=NUM_SERVOS) )
  {
    //Serial.print("Servo selected ");
    //Serial.println(data);
    while(!Serial.available()) //wait until another number is sent, this is the direction of the servo
    {
    }
    dir = Serial.read();
    dir = dir - ASCII_TO_INT;
    
    
    
    if( (dir!=1) && (dir!=2)) //error check the dir
    {
      //Serial.print("invalid dir ");
      //Serial.print(dir);
      return;
    }
    else
    {
      //Serial.print("dir is");
      //Serial.println(dir);
      if(dir == 1)
        servo_status[data-1] = MOVE_POSITIVE; //servos index start from 0. App should send starting from index 1
      else
        servo_status[data-1] = MOVE_NEGATIVE;

        curr_servo = data-1;
    }
       
  }
  else if((cmd == APP_CMD_STOP) || (cmd == MOVE_STOP))
  {
    //Serial.println("stopping");
    servo_status[curr_servo] = MOVE_STOP;

  }
    
  else if(cmd == APP_CMD_SETSPEED)
  {
    //Serial.println("Setting speed, waiting for speed");
    while(!Serial.available()) //wait until another number is sent, this is the speed of the servos
    {
    }
    //set the speed;
    gServoSpeed = Serial.read();
    //Serial.print("Received speed ");
    //Serial.println(gServoSpeed);
  }
  else if(cmd == APP_CMD_RESET)
  {
    //Serial.println("Resetting Servos");
    ResetServos();
  }
  else if ( cmd == APP_CMD_GET_VERSION )
  {
    Serial.print("Robo Arm Version: ");
    Serial.print(VERSION);
    #ifdef ONE_BASE_MOTORS
    Serial.println(" Base motors: 1");
    #endif
    #ifdef TWO_BASE_MOTORS
    Serial.println(" Base motors: 2");
    #endif
  }
  else if (cmd == APP_CMD_SAVE_SERVOS)
  {
      Serial.println("Saving Servos"); 
      SaveServos();
  }
  else
  {
    //Serial.println("Invalid command");
  }

}

void SaveServos()
{
 
   short int curr_angle = 0;
    for(int i=0; i < NUM_SERVOS; i++)
    {
      curr_angle = EEPROM.read(i);
      if (curr_angle != servo_positions[i]) //only write if there is a change in the position
      {
     
        EEPROM.write(i,servo_positions[i]);
      }

       
    }

    if(IsServoInfoInMemory() == 0) //if this is the first time we are saving, set the saved values flag
      EEPROM.write(STORED_VALUES,STORED_VALUES);
}

byte IsServoInfoInMemory()
{
  return false;
  //TMP !return ( EEPROM.read(STORED_VALUES) == STORED_VALUES );
}

void SetUpServos(short init_angle)
{
  short int AreServoValuesInMemory = 0;
  short int ServoAngle = 0;

  AreServoValuesInMemory = IsServoInfoInMemory();

 
  if(AreServoValuesInMemory == 1)
  {
 
    for(int i=0; i < NUM_SERVOS; i++)
    {

      #ifdef TWO_BASE_MOTORS
        if ((i==1) || (i==2) ) //dont move the base assume its already at 90 deg
        continue;
      #endif
      ServoAngle = EEPROM.read(i);
      servo_status[i] = MOVE_STOP;

      SetServoAngle(i+1, ServoAngle,gServoSpeed);
    }    
  }

  else
  {
  
    for(int i=0; i < NUM_SERVOS; i++)
    {
      
      servo_positions[i] = init_angle;
      
      servo_status[i] = MOVE_STOP;

      #ifdef TWO_BASE_MOTORS
        if ((i==1) || (i==2) ) //dont move the base assume its already at 90 deg
        continue;
      #endif
    
      SetServoAngle(i+1, init_angle,gServoSpeed);
    }
  }
   
}
void ResetServos()
{

  SetUpServos(SERVO_START_ANGLE);

}
 
            
        

void MoveServos()
{
  short angle = 0;
  char curr_servo_status =' ';
  short servo_step = 1;
  for(int i=0; i < NUM_SERVOS; i++)
  {
   
    curr_servo_status = servo_status[i];
    if(curr_servo_status == MOVE_POSITIVE)
    {
       
        angle = servo_positions[i];
        angle += servo_step;

        if(angle > 180)
          angle = 180;
        SetServoAngle(i+1, angle,gServoSpeed);
        servo_positions[i] = angle;
        delay(gServoSpeed);
    }
    else if (curr_servo_status == MOVE_NEGATIVE )
    {
 
        angle = servo_positions[i];
        angle -= servo_step;
        if(angle <= 0)
          angle = 0;
        SetServoAngle(i+1, angle,gServoSpeed);
        servo_positions[i] = angle;
        delay(gServoSpeed);
    }
    
  }
   
}

void setupJoystick()
{
  pinMode(X_pin, INPUT);
  pinMode(Y_pin, INPUT);
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
}

void switchJoystickControl()
{
  if(joy_control == JOY_CONTROL_LOW)
  {
    servo_status[0] = MOVE_STOP;
    servo_status[1] = MOVE_STOP;
    joy_control = JOY_CONTROL_HIGH;
  }
  else if(joy_control == JOY_CONTROL_HIGH)
  {
    servo_status[2] = MOVE_STOP;
    servo_status[3] = MOVE_STOP;
    joy_control = JOY_CONTROL_LOW;
  }
}

void readJoystick()
{

  const int pos_thresh = 1000; //positive threshold 
  const int neg_thresh = 25;// negative threshold
  
  int xval = analogRead(X_pin);
  int yval = analogRead(Y_pin);

  if(xval > pos_thresh)
  {
    servo_status[joy_control] = MOVE_POSITIVE;
  }
  else if(xval < neg_thresh)
  {
    servo_status[joy_control] = MOVE_NEGATIVE;
  }
  else 
  {
    servo_status[joy_control] = MOVE_STOP;
  }


  if(yval > pos_thresh)
  {
    servo_status[joy_control+1] = MOVE_POSITIVE;
    
  }
  else if(yval < neg_thresh)
  {
    servo_status[joy_control+1] = MOVE_NEGATIVE;
    
  }
  else 
  {
    servo_status[joy_control+1] = MOVE_STOP;
  }
}

bool buttonPress() //jpystick button press
{
  current_press = digitalRead(SW_pin);

  if(current_press == 0 && previous_press == 1)
  {
      previous_press = current_press;
      return true;
  }

  previous_press = current_press;
  
  
  //delay(200);
  return false;
}
 
void setup() {
  Serial.begin(115200); 

  setupJoystick();
   
  pwm.begin();
   
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  
  yield();

  

  //Serial.print("Robo Arm Version ");
  //Serial.println(VERSION);


  int servo = 0;
  int angle_mem = 0;
  int pulselength = 0;
  
  
  for(int i =0; i < NUM_SERVOS; i++)
  {
    if(IsServoInfoInMemory())
    {
       angle_mem = EEPROM.read(i);
       Serial.print(i);
       Serial.print(" ");
       Serial.println(angle_mem);
       pulselength = map(angle_mem, 0, 180, SERVOMIN, SERVOMAX);
       pwm.setPWM(i, 0, pulselength);
       servo_positions[i] = angle_mem;
    }
    else
    {
      pulselength = map(90, 0, 180, SERVOMIN, SERVOMAX);
      pwm.setPWM(i, 0, pulselength);
      servo_positions[i] = 90;
    }
    delay(1000);
  }
  
  
  
  SetUpServos(SERVO_START_ANGLE);
}



void loop() {

  
        if (Serial.available() > 0) {
                
                ReadCommand();        
        }

        if(control_mode == CONTROL_MODE_JOY)
        {
          readJoystick();
        }
        
        if(buttonPress())
        {
          if(control_mode == CONTROL_MODE_JOY)
          {
            switchJoystickControl(); //switches between lower and upper arm control
          }
          else if(control_mode == CONTROL_MODE_SERIAL)//switch from serial to joystick control
          {
             control_mode = CONTROL_MODE_JOY;
          }
          
        }
        
        MoveServos();

}

