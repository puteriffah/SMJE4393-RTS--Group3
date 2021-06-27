////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    Description:                                                                                                                            //
//    1) Initially, the password needed to be set in 4 words/chars in the system.                                                             //
//    2) After password is set, the default state of the system is Non-Armed state, where all timer and sensors tasks are suspended.          //
//    3) User have to pressed the correct password to change the state into Armed state, where all timer and sensors tasks are resumed.       //
//    4) In Armed state,                                                                                                                      //
//       For front-door sensor task: If it detect object, it will jump to Pre-Alarm state.                                                    //
//       For window sensor task: If it detect object, it will jump to Alarming state.                                                         //
//    5) In Alarming state, the alarm will be activated immediately according to the alarm triggered.                                         //
//    6) In Pre-Alarm state, the timer task will countdown 30 seconds before jumps into alarming state, and user can press the password to    //
//       stop the coutdown which it will jump into Non-Armed state.                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino_FreeRTOS.h>       //Real-time system library
#include <Keypad.h>                 //Keypad library
#pragma GCC optimize("03")         

#define buzzer1 A2                  //Front-door alarm pin                                                                       
#define pir1 A0                     //Sensor for front-door alarm pin                                                            
#define buzzer2 A3                  //Window alarm pin                                                                       
#define pir2 A1                     //Sensor for window alarm pin                                                                         

TaskHandle_t TimerTask;             //Declare countdown task                 
TaskHandle_t DisplayTask;           //Declare display task            
TaskHandle_t Sensor1Task;           //Declare Front-door sensing task             
TaskHandle_t Sensor2Task;           //Declare window sensing task             

//Variables declaration/////////////////////////////////////////////////////////////////////////////////////////////
const int ROW_NUM = 4;                                                                                            //
const int COLUMN_NUM = 4;                                                                                         //
String password = "";                                                                                             //
boolean Set = false;                                                                                              //
String input = "";                                                                                                //
int states = 0;                                                                                                   //
int countdown = 30;                                                                                               //
int Sensor1_Value = 0;                                                                                            //
int Sensor2_Value = 0;                                                                                            //
int Range_Sensor1 = 0;                                                                                            //
int Range_Sensor2 = 0;                                                                                            //
                                                                                                                  //
char keys[ROW_NUM][COLUMN_NUM] = {                                                    //4x4 keypad arrangement    //
  {'1','2','3', 'A'},                                                                                             //
  {'4','5','6', 'B'},                                                                                             //
  {'7','8','9', 'C'},                                                                                             //
  {'*','0','#', 'D'}                                                                                              //
};                                                                                                                //
                                                                                                                  //
byte rowPins[ROW_NUM] = {7,6,5,4};                                                    //Keypad row pins           //
byte colPins[COLUMN_NUM] = {3,2,1,0};                                                 //Keypad column pins        //                              
                                                                                                                  //
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROW_NUM, COLUMN_NUM);     //Declare keypad function   //                                         
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);                                                       //Start 9600 bit baud rate
  pinMode(8, OUTPUT);                                                       //Yellow LED pin
  pinMode(9, OUTPUT);                                                       //Green LED pin
  pinMode(10, OUTPUT);                                                      //Orange LED pin
  pinMode(11, OUTPUT);                                                      //Red LED pin
  xTaskCreate(Task_Sensor2, "TaskSensor2", 100, NULL, 4, &Sensor2Task);     //Create front-door sensor task with priority level 4
  xTaskCreate(Task_Sensor1, "TaskSensor1", 100, NULL, 3, &Sensor1Task);     //Create window sensor task with priority level 3
  xTaskCreate(Task_Display, "TaskDisplay", 100, NULL, 2, &DisplayTask);     //Create display task with priority level 2
  xTaskCreate(Task_Countdown, "TaskCountDown", 100, NULL, 1, &TimerTask);   //Create countdown task with priority level 1
}

void loop() {
  
}

static void Task_Countdown(void* pvParameters){      //Countdown task
  while(1){
    if(states == 2){                                 //When it is in Pre-Alarm state
      Serial.println(countdown);                     //Shows the countdown numbers in Serial Monitor
      vTaskDelay(1000/portTICK_PERIOD_MS);           //Delay 1 second
      countdown--;                                   //Countdown by 1
      if(countdown == 0){                            //If countdown number reach 0
        digitalWrite(10, LOW);                       //Orange light dims out
        states = 3;                                  //Jumps into Alarming state
      }
    }else{
      countdown = 30;                                //Assigns countdown with value of 30 seconds
      Serial.println(F("task countdown"));           //Shows task countdown in Serial Monitor
      vTaskDelay(100/portTICK_PERIOD_MS);            //Countdown task get into blocked state for 100ms in Armed state and Alarming state
    }
  }
}

static void Task_Display(void* pvParameters){             //Display task
  while(1){
    if(password == ""){                                   //Setting or Initialize password
      Serial.println(F("task set password"));    
      digitalWrite(8, HIGH);                              ///////////////////////////////////////////
      digitalWrite(9, HIGH);                              //   4 colors LEDs ligth up to indicate  //  
      digitalWrite(10, HIGH);                             //   password setting is in progress     // 
      digitalWrite(11, HIGH);                             ///////////////////////////////////////////
      while(Set == false){
        char key = keypad.getKey();                       ////////////////////////
        if(key){                                          //   Get the inputs   //
          password += key;                                //   from the keypad  //
          Serial.println(password);                       ////////////////////////
        }
        if(password.length() == 4){                       //Get 4 characters to form a password
          Set = true;
          Serial.println(F("task set password finish"));
          digitalWrite(8, LOW);                           ////////////////////////////////////////////
          digitalWrite(9, LOW);                           //   4 colors LEDs dim out to indicate    //
          digitalWrite(10, LOW);                          //     password setting is finished       //
          digitalWrite(11, LOW);                          ////////////////////////////////////////////
        }
      }
    }else{                                                //Using Switch...case to run Finite State Machine of the system with 4 states
      char key;
      Serial.print(F("state "));
      Serial.println(states);                             //Indicate the state in Serial Monitor
      switch(states){
        case 1:                                           //System is in Armed state
        vTaskResume(TimerTask);                           //Resume countdown task
        vTaskResume(Sensor1Task);                         //Resume front-door sensing task
        vTaskResume(Sensor2Task);                         //Resume window sensing task            
        digitalWrite(8, HIGH);                            //Yellow LED lights up to indicate it is Armed state
        key = keypad.getKey();
        if(key){                                          ///////////////////////////
          input += key;                                   //  Key in the password  //
          Serial.println(input);                          //    to jump state      //
          Serial.println(password);                       ///////////////////////////
        }

        if(input.length() == 4){                          ///////////////////////
          if(input != password){                          //                   //
            input = "";                                   //  Password key in  //
            Serial.print(F("Incorrect password!!"));      //  error detection  //
          }                                               //                   //
        }                                                 ///////////////////////

        if(input == password){                            /////////////////////////////////////////////////////////////
          digitalWrite(8, LOW);                           //  If input password is correct, it jumps into Non-Armed  //
          states = 4;                                     //     state, Yellow light dims out, and then display      //
          input = "";                                     //        task get into blocked state for 100ms            //
          vTaskDelay(100/portTICK_PERIOD_MS);             /////////////////////////////////////////////////////////////
        }else{
          vTaskDelay(100/portTICK_PERIOD_MS);             //Display task get into blocked state 100ms if no action happened in Armed state
        }
        break;                                            
      
        case 2:                                           //System is in Pre-Alarm state                          
        digitalWrite(10, HIGH);                           //Orange LED lights up to indicate it is Pre-Alarm state
        key = keypad.getKey();
        if(key){                                          ///////////////////////////
          input += key;                                   //  Key in the password  //
          Serial.println(input);                          //     to jump state     //
          Serial.println(password);                       ///////////////////////////
        }

        if(input.length() == 4){                          ///////////////////////
          if(input != password){                          //                   //
            input = "";                                   //  Password key in  //
            Serial.print(F("Incorrect password!!"));      //  error detection  //
          }                                               //                   //
        }                                                 ///////////////////////

        if(input == password){                            /////////////////////////////////////////////////////////////
          digitalWrite(10, LOW);                          //  If input password is correct, it jumps into Non-Armed  //
          states = 4;                                     //      state, Orange light dims out, and then display     //
          input = "";                                     //          task get into blocked state for 100ms          //
          vTaskDelay(100/portTICK_PERIOD_MS);             /////////////////////////////////////////////////////////////
        }else{
          vTaskDelay(100/portTICK_PERIOD_MS);             //Display task get into blocked state 100ms if no action happened in Pre-Alarm state
        }
        break;
      
        case 3:                                           //System is in Alarming state
        digitalWrite(11, HIGH);                           //Red LED lights up to indicate it is Alarming state
        key = keypad.getKey();
        if(key){                                          ///////////////////////////    
          input += key;                                   //  Key in the password  //
          Serial.println(input);                          //     to jump state     //
          Serial.println(password);                       ///////////////////////////
        }

        if(input.length() == 4){                          ///////////////////////
          if(input != password){                          //                   //
            input = "";                                   //  Password key in  //
            Serial.print(F("Incorrect password!!"));      //  error detection  //
          }                                               //                   //
        }                                                 ///////////////////////

        if(input == password){                            /////////////////////////////////////////////////////////////
          digitalWrite(11, LOW);                          //  If input password is correct, it jumps into Non-Armed  //
          states = 4;                                     //      state, Red light dims out, and then display        //
          input = "";                                     //         task get into blocked state for 100ms           //
          vTaskDelay(100/portTICK_PERIOD_MS);             /////////////////////////////////////////////////////////////
        }else{
          vTaskDelay(100/portTICK_PERIOD_MS);             //Display task get into blocked state 100ms if no action happened in Alarming state
        }
        break;
      
        default:                                          //System is in Non-Armed state
        vTaskSuspend(TimerTask);                          //Suspend countdown task
        vTaskSuspend(Sensor1Task);                        //Suspend front-door sensing task
        vTaskSuspend(Sensor2Task);                        //Suspend window sensing task
        analogWrite(buzzer1, 0);                          //Switch off the front-door alarm
        analogWrite(buzzer2, 0);                          //Switch off the window alarm
        digitalWrite(9, HIGH);                            //Green LED indicate it is Non-Armed state
        key = keypad.getKey();
        if(key){                                          ///////////////////////////
          input += key;                                   //  Key in the password  //
          Serial.println(input);                          //     to jump state     //
          Serial.println(password);                       ///////////////////////////
        }

        if(input.length() == 4){                          ///////////////////////
          if(input != password){                          //                   //
            input = "";                                   //  Password key in  //
            Serial.print(F("Incorrect password!!"));      //  error detection  //
          }                                               //                   //
        }                                                 ///////////////////////
      
        if(input == password){                            /////////////////////////////////////////////////////////////
          digitalWrite(9, LOW);                           //     If input password is correct, it jumps into Armed   //
          states = 1;                                     //       state, Green light dims out and then display      //
          input = "";                                     //          task get into blocked state for 100ms          //
          vTaskDelay(100/portTICK_PERIOD_MS);             /////////////////////////////////////////////////////////////
        }else{
          vTaskDelay(100/portTICK_PERIOD_MS);             //Display task get into blocked state 100ms if no action happened in Non-Armed state
        }
        break;
      }
    }
  }
}

static void Task_Sensor1(void* pvParameters){                 //Front-door sensing task
  while(1){
    if(states == 1){                                          //When it is in Armed state
      Sensor1_Value = analogRead(pir1);                       //Read the front-door sensor reading  
      Range_Sensor1 = map(Sensor1_Value, 0, 1023, 0, 255);    //Convert front-door sensor reading into the range of 0 to 255
      if(Range_Sensor1 > 100){                                //If front-door sensor reading exceeds 100
        digitalWrite(8, LOW);                                 //Yellow light dims out
        states = 2;                                           //Jumps into Pre-Alarm state
      }
      Serial.print(F("task front-door sensing: "));
      Serial.println(Range_Sensor1);                          //Shows the front-door sensor reading in Serial Monitor
      vTaskDelay(100/portTICK_PERIOD_MS);                     //Front-door sensor task get into blocked state for 100ms in Armed state
    }else if(states == 3){                                    //When it is in Alarming state
      if(Range_Sensor1 > 100){                                //If front-door sensor reading exceeds 100
        analogWrite(buzzer1, Range_Sensor1);                  //Front-door alarm is switched on
        Serial.println(F("task front-door alarm on"));        //Shows front-door alarm is on in Serial Monitor
      }        
      vTaskDelay(100/portTICK_PERIOD_MS);                     //Front-door sensor task get into blocked state for 100ms in Alarming state
    }else{
      Serial.println(F("task front-door sensor"));            //Shows front-door sensing task in Serial Monitor      
      vTaskDelay(100/portTICK_PERIOD_MS);                     //Front-door sensor task get into blocked state for 100ms in Pre-alarm state
    }
  }
}

static void Task_Sensor2(void* pvParameters){                 //Window sensing task
  while(1){
    if(states == 1){                                          //When it is in Armed state
      Sensor2_Value = analogRead(pir2);                       //Read the window sensor reading
      Range_Sensor2 = map(Sensor2_Value, 0, 1023, 0, 255);    //Convert window sensor reading into the range of 0 to 255
      if(Range_Sensor2 > 100){                                //If window sensor reading exceeds 100
        digitalWrite(8, LOW);                                 //Yellow light dims out
        states = 3;                                           //Jumps into Alarming state
      }
      Serial.print(F("task window sensing: "));             
      Serial.println(Range_Sensor2);                          //Shows the window sensor reading in Serial Monitor
      vTaskDelay(100/portTICK_PERIOD_MS);                     //Front-door sensor task get into blocked state for 100ms in Armed state
    }else if(states == 3){                                    //When it is in Alarming state
      if(Range_Sensor2 > 100){                                //If window sensor reading exceeds 100
        analogWrite(buzzer2, Range_Sensor2);                  //Window alarm is switched on
        Serial.println(F("task window alarm on"));            //Shows window alarm is on in Serial Monitor
      }
      vTaskDelay(100/portTICK_PERIOD_MS);                     //Window sensor task get into blocked state for 100ms in Alarming state
    }else{
      Serial.println(F("task window sensor"));                //Shows window sensing task in Serial Monitor
      vTaskDelay(100/portTICK_PERIOD_MS);                     //Window sensor task get into blocked state for 100ms in Pre-alarm state
    }
  }  
}
