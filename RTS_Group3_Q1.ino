//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Description:                                                                                                                                          //
//    1) Initially, the password needed to be set in 4 words/chars in the system.                                                                           //
//    2) After password is set, the default state of the system is Non-Armed state and it shown in LCD, all timer and sensors tasks are suspended.          //
//    3) User have to pressed the correct password to change the state into Armed state and it shown in LCD, all timer and sensors tasks are resumed.       //
//    4) In Armed state,                                                                                                                                    //
//       For front-door sensor task: If it detect object, it will jump to Pre-Alarm state and it shown in LCD.                                              //
//       For window sensor task: If it detect object, it will jump to Alarming state and it shown in LCD.                                                   //
//    5) In Alarming state, the alarm will be activated immediately according to the alarm triggered.                                                       //
//    6) In Pre-Alarm state, the timer task will countdown 30 seconds before jumps into alarming state, and user can press the password to                  //
//       stop the coutdown which it will jump into Non-Armed state.                                                                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino_FreeRTOS.h>     //Real time system library
#include <LiquidCrystal.h>        //LCD library 
#include <Keypad.h>               //Keypad library
#pragma GCC optimize("03")         

#define buzzer1 A2                  //Front-door alarm pin                                                                       
#define pir1 A0                     //Sensor for front-door alarm pin                                                            
#define buzzer2 A3                  //Window alarm pin                                                                       
#define pir2 A1                     //Sensor for window alarm pin 
                              
TaskHandle_t TimerTask;                    //Declare countdown task
TaskHandle_t LCDTask;                      //Declare LCD task
TaskHandle_t Sensor1Task;                  //Declare front-door sensing task
TaskHandle_t Sensor2Task;                  //Declare window sensing task

//Variables declaration/////////////////////////////////////////////////////////////////////////////////////////////
const int ROW_NUM = 4;                                                                                            //
const int COLUMN_NUM = 4;                                                                                         //
String password = "";                                                                                             //
boolean Set = false;                                                                                              //
boolean key_in = false;                                                                                           //
String displays = "";                                                                                             //
String input = "";                                                                                                //
int states = 4;                                                                                                   //
int countdown = 30;                                                                                               //
int Sensor1_Value = 0;                                                                                            //
int Sensor2_Value = 0;                                                                                            //
int Range_Sensor1 = 0;                                                                                            //
int Range_Sensor2 = 0;                                                                                            //
                                                                                                                  //
char keys[ROW_NUM][COLUMN_NUM] = {                                                   //4x4 keypad arrangement     //
  {'1','2','3', 'A'},                                                                                             //
  {'4','5','6', 'B'},                                                                                             //
  {'7','8','9', 'C'},                                                                                             //
  {'*','0','#', 'D'}                                                                                              //
};                                                                                                                //
                                                                                                                  //
byte rowPins[ROW_NUM] = {7,6,5,4};                                                   //Keypad row pins            //
byte colPins[COLUMN_NUM] = {3,2,1,0};                                                //Keypad column pins         //
                                                                                                                  //
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROW_NUM, COLUMN_NUM);    //Declare keypad function    //
LiquidCrystal lcd(8,9,10,11,12,13);                                                  //LCD pins                   //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  lcd.begin(16, 2);                                                         //Begin LCD with 16 spaces in each row, and 2 spaces in each column
  Serial.begin(9600);                                                       //Start 9600 bit baud rate
  xTaskCreate(Task_Sensor2, "TaskSensor2", 100, NULL, 4, &Sensor2Task);     //Create front-door sensor task with priority level 4
  xTaskCreate(Task_Sensor1, "TaskSensor1", 100, NULL, 3, &Sensor1Task);     //Create window sensor task with priority level 3
  xTaskCreate(Task_LCD, "TaskDisplay", 100, NULL, 2, &LCDTask);             //Create LCD display task with priority level 2
  xTaskCreate(Task_Countdown, "TaskCountDown", 100, NULL, 1, &TimerTask);   //Create countdown task with priority level 1
}

void loop() {
  
}

static void Task_Countdown(void* pvParameters){          //Countdown task
  while(1){
  if(states == 2){                                       //When it is in Pre-Alarm state
    lcd.setCursor(10, 0);                                //Setting LCD cursor 10 at row and 0 at column
    lcd.print(countdown);                                //LCD shows the countdown numbers
    Serial.println(countdown);                           //Shows the countdown numbers in Serial Monitor
    vTaskDelay(1000/portTICK_PERIOD_MS);                 //Delay 1 second
    countdown--;                                         //Countdown by 1
    if(countdown == 0){                                  //If countdown number reach 0
      states = 3;                                        //Jumps into Alarming state
    }
  }else{
    countdown = 30;                                      //Assigns countdown with value of 30 seconds
    Serial.println(F("task countdown"));                 //Shows task countdown in Serial Monitor
    vTaskDelay(100/portTICK_PERIOD_MS);                  //Countdown task get into blocked state for 100ms in Armed state and Alarming state
  }
  }
}

static void Task_LCD(void* pvParameters){                //Display task
  while(1){
  if(password == ""){                                    //Setting or Initialize password
    lcd.clear();                                         //Clear the content in LCD
    lcd.setCursor(0, 0);                                 //Setting LCD cursor 0 at row, 0 at column
    lcd.print("Set Password");                           //Shows Set Password in LCD
    lcd.setCursor(0,1);                                  //Setting LCD cursor 0 at row, 1 at column
    lcd.print("4 words:");                               //Shows 4 words: in LCD
    Serial.println("Set Password 4 words:");             //Shows the message Set Password 4 words: in serial monitor
    while(Set == false){                                 
      lcd.setCursor(8, 1);                               //Setting LCD cursor 8 at row, 1 at column
      lcd.print(password);                               //Shows the keyed in numbers on the LCD
      char key = keypad.getKey();                        ////////////////////////
      if(key){                                           //   Get the inputs   //
        password += key;                                 //   from the keypad  //
        Serial.println(password);                        ////////////////////////
      }

      if(password.length() == 4){                        //Get 4 characters to form a password
        Set = true;
        Serial.println(F("task set password finish"));   //Shows task setting password is finished
      }
    }
  }else{                                                 //Using Switch...case to run Finite State Machine of the system with 4 states
    char key;
    switch(states){
      case 1:                                            //System is in Armed state
      vTaskResume(TimerTask);                            //Resume countdown task
      vTaskResume(Sensor1Task);                          //Resume front-door sensing task
      vTaskResume(Sensor2Task);                          //Resume window sensing task 
      lcd.clear();                                       //Clear the content in LCD
      lcd.setCursor(0, 0);                               //Setting LCD cursor 0 at row, 0 at column
      lcd.print("Armed");                                //Shows Armed in LCD
      key = keypad.getKey();                             //Assign keypad variable
      lcd.setCursor(0, 1);                               //Setting LCD cursor 0 at row, 1 at column
      lcd.print(input);                                  //Shows keyed in numbers on LCD
      Serial.println("Armed state");                     //Shows now is Armed state at serial monitor
      if(key){                                           ///////////////////////////
        input += key;                                    //  Key in the password  //
        Serial.println(input);                           //    to jump state      //
        Serial.println(password);                        ///////////////////////////
      }

      if(input.length() == 4){                           ///////////////////////
        if(input != password){                           //                   //
          input = "";                                    //  Password key in  //
          lcd.setCursor(0, 1);                           //  error detection  //
          lcd.print("Wrong password");                   //                   //              
          Serial.print(F("Incorrect password!!"));       ///////////////////////
        }                                               
      }       
      if(input == password){                             ///////////////////////////////////////////////////////////////////                         
          states = 4;                                    //  If input password is correct, it jumps into Non-Armed state, // 
          input = "";                                    //    and then display task get into blocked state for 100ms     // 
          vTaskDelay(100/portTICK_PERIOD_MS);            /////////////////////////////////////////////////////////////////// 
        }else{
          vTaskDelay(100/portTICK_PERIOD_MS);            //Display task get into blocked state 100ms if no action happened in Armed state 
        }
      break;
      
      case 2:                                            //System is in Pre-Alarm state
      lcd.clear();                                       //Clear the content in LCD
      lcd.setCursor(0, 0);                               //Setting LCD cursor 0 at row, 0 at column
      lcd.print("Pre-alarm");                            //Shows Pre-alarm in LCD
      key = keypad.getKey();                             //Assign keypad variable
      lcd.setCursor(0, 1);                               //Setting LCD cursor 0 at row, 1 at column
      lcd.print(input);                                  //Shows keyed in numbers on LCD
      Serial.println("Pre-alarm state");                 //Shows now is Pre-alarm state at serial monitor
      if(key){                                           ///////////////////////////
        input += key;                                    //  Key in the password  //
        Serial.println(input);                           //    to jump state      //
        Serial.println(password);                        ///////////////////////////
      }

      if(input.length() == 4){                           ///////////////////////
        if(input != password){                           //                   //
          input = "";                                    //  Password key in  //
          lcd.setCursor(0, 1);                           //  error detection  //
          lcd.print("Wrong password");                   //                   //              
          Serial.print(F("Incorrect password!!"));       ///////////////////////
        }                                               
      }   
      if(input == password){                             ///////////////////////////////////////////////////////////////////                         
        states = 4;                                      //  If input password is correct, it jumps into Non-Armed state, //
        input = "";                                      //    and then display task get into blocked state for 100ms     //
        vTaskDelay(100/portTICK_PERIOD_MS);              ///////////////////////////////////////////////////////////////////
      }else{
        vTaskDelay(100/portTICK_PERIOD_MS);              //Display task get into blocked state 100ms if no action happened in Pre-Alarm state
      }
      break;
      
      case 3:                                            //System is in Alarming state
      lcd.clear();                                       //Clear the content in LCD
      lcd.setCursor(0, 0);                               //Setting LCD cursor 0 at row, 0 at column
      lcd.print("Alarming");                             //Shows Alarming in LCD
      key = keypad.getKey();                             //Assign keypad variable
      lcd.setCursor(0, 1);                               //Setting LCD cursor 0 at row, 1 at column
      lcd.print(input);                                  //Shows keyed in numbers on LCD
      Serial.println("Alarming state");                  //Shows now is Alarming state at serial monitor
      if(key){                                           ///////////////////////////
        input += key;                                    //  Key in the password  //
        Serial.println(input);                           //    to jump state      //
        Serial.println(password);                        ///////////////////////////
      }

      if(input.length() == 4){                           ///////////////////////
        if(input != password){                           //                   //
          input = "";                                    //  Password key in  //
          lcd.setCursor(0, 1);                           //  error detection  //
          lcd.print("Wrong password");                   //                   //              
          Serial.print(F("Incorrect password!!"));       ///////////////////////
        }                                               
      }   
      if(input == password){                             ///////////////////////////////////////////////////////////////////                         
        states = 4;                                      //  If input password is correct, it jumps into Non-Armed state, //
        input = "";                                      //    and then display task get into blocked state for 100ms     //
        vTaskDelay(100/portTICK_PERIOD_MS);              ///////////////////////////////////////////////////////////////////
      }else{
        vTaskDelay(100/portTICK_PERIOD_MS);              //Display task get into blocked state 100ms if no action happened in Alarming state
      }
      break;
      
      default:                                           //System is in Non-Armed state
      vTaskSuspend(TimerTask);                           //Suspend countdown task
      vTaskSuspend(Sensor1Task);                         //Suspend front-door sensing task
      vTaskSuspend(Sensor2Task);                         //Suspend window sensing task
      analogWrite(buzzer1, 0);                           //Switch off the front-door alarm
      analogWrite(buzzer2, 0);                           //Switch off the window alarm
      lcd.clear();                                       //Clear the content in LCD
      lcd.setCursor(0, 0);                               //Setting LCD cursor 0 at row, 0 at column
      lcd.print("Not Armed");                            //Shows Not Armed in LCD
      key = keypad.getKey();                             //Assign keypad variable
      lcd.setCursor(0, 1);                               //Setting LCD cursor 0 at row, 1 at column
      lcd.print(input);                                  //Shows keyed in numbers on LCD
      Serial.println("Non-armed state");                 //Shows now is Non-Armed state at serial monitor
      if(key){                                           ///////////////////////////
        input += key;                                    //  Key in the password  //
        Serial.println(input);                           //    to jump state      //
        Serial.println(password);                        ///////////////////////////
      }

      if(input.length() == 4){                           ///////////////////////
        if(input != password){                           //                   //
          input = "";                                    //  Password key in  //
          lcd.setCursor(0, 1);                           //  error detection  //
          lcd.print("Wrong password");                   //                   //            
          Serial.print(F("Incorrect password!!"));       ///////////////////////
        }                                               
      }   
      if(input == password){                             ///////////////////////////////////////////////////////////////////                         
        states = 1;                                      //  If input password is correct, it jumps into Armed state,     //
        input = "";                                      //    and then display task get into blocked state for 100ms     //
        vTaskDelay(100/portTICK_PERIOD_MS);              ///////////////////////////////////////////////////////////////////
      }else{
        vTaskDelay(100/portTICK_PERIOD_MS);              //Display task get into blocked state 100ms if no action happened in Non-Armed state
      }
      break;
    }
  }
  }
}

static void Task_Sensor1(void* pvParameters){                  //Front-door sensing task
  while(1){
  if(states == 1){                                             //When it is in Armed state
    Sensor1_Value = analogRead(pir1);                          //Read the front-door sensor reading  
    Range_Sensor1 = map(Sensor1_Value, 0, 1023, 0, 255);       //Convert front-door sensor reading into the range of 0 to 255
    if(Range_Sensor1 > 100){                                   //If front-door sensor reading exceeds 100
      states = 2;                                              //Jumps into Pre-Alarm state
    }
    Serial.print(F("task front-door sensing: "));              
    Serial.println(Range_Sensor1);                             //Shows the front-door sensor reading in Serial Monitor
    vTaskDelay(100/portTICK_PERIOD_MS);                        //Front-door sensor task get into blocked state for 100ms in Armed state
  }else if(states == 3){                                       //When it is in Alarming state
    if(Range_Sensor1 > 100){                                   //If front-door sensor reading exceeds 100
      analogWrite(buzzer1, Range_Sensor1);                     //Front-door alarm is switched on
      Serial.println(F("task front-door alarm on"));           //Shows front-door alarm is on in Serial Monitor
    }
    vTaskDelay(100/portTICK_PERIOD_MS);                        //Front-door sensor task get into blocked state for 100ms in Alarming state
  }else{
    Serial.println(F("task front-door sensor"));               //Shows front-door sensing task in Serial Monitor
    vTaskDelay(100/portTICK_PERIOD_MS);                        //Front-door sensor task get into blocked state for 100ms in Pre-alarm state
  }
  }
}

static void Task_Sensor2(void* pvParameters){                  //Window sensing task
  while(1){
  if(states == 1){                                             //When it is in Armed state
    Sensor2_Value = analogRead(pir2);                          //Read the window sensor reading
    Range_Sensor2 = map(Sensor2_Value, 0, 1023, 0, 255);       //Convert window sensor reading into the range of 0 to 255
    if(Range_Sensor2 > 100){                                   //If window sensor reading exceeds 100
      states = 3;                                              //Jumps into Alarming state
    }
    Serial.print(F("task window sensing: "));                  
    Serial.println(Range_Sensor2);                             //Shows the window sensor reading in Serial Monitor
    vTaskDelay(100/portTICK_PERIOD_MS);                        //Front-door sensor task get into blocked state for 100ms in Armed state
  }else if(states == 3){                                       //When it is in Alarming state
    if(Range_Sensor2 > 100){                                   //If window sensor reading exceeds 100
      analogWrite(buzzer2, Range_Sensor2);                     //Window alarm is switched on
      Serial.println(F("task window alarm on"));               //Shows window alarm is on in Serial Monitor
    }
    vTaskDelay(100/portTICK_PERIOD_MS);                        //Window sensor task get into blocked state for 100ms in Alarming state
  }else{
    Serial.println(F("task window sensor"));                   //Shows window sensing task in Serial Monitor
    vTaskDelay(100/portTICK_PERIOD_MS);                        //Window sensor task get into blocked state for 100ms in Pre-alarm state
  }
  }    
}
