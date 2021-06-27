#include <Arduino_FreeRTOS.h>     //Real time system library
#include <LiquidCrystal.h>        //LCD library 
#include <Keypad.h>               //Keypad library
#pragma GCC optimize("03")         

//Define pins for buzzers and PIR sensors (sensor 1 = front door sensor, sensor 2 = window sensor)///
#define buzzer1 A2                  //Front-door alarm pin                                                                       
#define pir1 A0                     //Sensor for front-door alarm pin                                                            
#define buzzer2 A3                  //Window alarm pin                                                                       
#define pir2 A1                     //Sensor for window alarm pin 
/////////////////////////////////////////////////////////////////////////////////////////////////////

//4 tasks declaration////////////////////////
                                           //
TaskHandle_t TimerTask;                    //
TaskHandle_t LCDTask;                      //
TaskHandle_t Sensor1Task;                  //
TaskHandle_t Sensor2Task;                  //
/////////////////////////////////////////////

//Variables declaration/////////////////////////////////////////////////////////////////
const int ROW_NUM = 4;                                                                //
const int COLUMN_NUM = 4;                                                             //
String password = "";                                                                 //
boolean Set = false;                                                                  //
boolean key_in = false;                                                               //
String displays = "";                                                                 //
String input = "";                                                                    //
int states = 4;                                                                       //
int countdown = 30;                                                                   //
int Sensor1_Value = 0;                                                                //
int Sensor2_Value = 0;                                                                //
int Range_Sensor1 = 0;                                                                //
int Range_Sensor2 = 0;                                                                //
                                                                             //
char keys[ROW_NUM][COLUMN_NUM] = {                                                    //
  {'1','2','3', 'A'},                                                                 //
  {'4','5','6', 'B'},                                                                 //
  {'7','8','9', 'C'},                                                                 //
  {'*','0','#', 'D'}                                                                  //
};                                                                                    //
                                                                                      //
byte rowPins[ROW_NUM] = {7,6,5,4};                                                    //
byte colPins[COLUMN_NUM] = {3,2,1,0};                                                 //
                                                                                      //
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROW_NUM, COLUMN_NUM);     //
LiquidCrystal lcd(8,9,10,11,12,13);                                                   //
////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
  xTaskCreate(Task_Sensor2, "TaskSensor2", 100, NULL, 4, &Sensor2Task);     //Create front-door sensor task with priority level 4
  xTaskCreate(Task_Sensor1, "TaskSensor1", 100, NULL, 3, &Sensor1Task);     //Create window sensor task with priority level 3
  xTaskCreate(Task_Display, "TaskDisplay", 100, NULL, 2, &LCDTask);     //Create display task with priority level 2
  xTaskCreate(Task_Countdown, "TaskCountDown", 100, NULL, 1, &TimerTask);   //Create countdown task with priority level 1
}

void loop() {
  
}

static void Task_Countdown(void* pvParameters){
  while(1){
  if(states == 2){
    //Countdown 30 seconds
    lcd.setCursor(10, 0);
    lcd.print(countdown);
    Serial.println(countdown);  
    vTaskDelay(1000/portTICK_PERIOD_MS);
    countdown--;
    if(countdown == 0){
      states = 3;
    }
  }else{
    //Blocking 150ms
    countdown = 30;
    Serial.println(F("task countdown"));
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
  }
}

static void Task_Display(void* pvParameters){
  while(1){
  //Setting or Initialize password
  if(password == ""){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Password");
    lcd.setCursor(0,1);
    lcd.print("4 words:");
    Serial.println("Set Password 4 words:");
    while(Set == false){
      lcd.setCursor(8, 1);
      lcd.print(password);
      char key = keypad.getKey(); 
      if(key){                                         
        password += key;                             
        Serial.println(password);                      
      }

      if(password.length() == 4){
        Set = true;
        Serial.println(F("task set password finish"));
      }
    }
  }else{
    //Finite State Machine of system
    char key;
    switch(states){
      case 1: //First Stage Armed
      vTaskResume(TimerTask);
      vTaskResume(Sensor1Task);
      vTaskResume(Sensor2Task);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Armed");
      key = keypad.getKey();
      lcd.setCursor(0, 1);
      lcd.print(input);
      Serial.println("Armed state");
      if(key){                                         
        input += key;                                   
        Serial.println(input);                          
        Serial.println(password);                       
      }

      if(input.length() == 4){                          
        if(input != password){                          
          input = "";  
          lcd.setCursor(0, 1);
          lcd.print("Wrong password");                                 
          Serial.print(F("Incorrect password!!"));      
        }                                               
      }       
      if(input == password){                                                      
          states = 4;                                     
          input = "";                                     
          vTaskDelay(100/portTICK_PERIOD_MS);             
        }else{
          vTaskDelay(100/portTICK_PERIOD_MS);             
        }
      break;
      
      case 2: //Second Stage Pre-Arlam
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pre-alarm");
      key = keypad.getKey();
      lcd.setCursor(0, 1);
      lcd.print(input);
      Serial.println("Pre-alarm state");
      if(key){                                         
        input += key;                                   
        Serial.println(input);                          
        Serial.println(password);                       
      }

      if(input.length() == 4){                          
        if(input != password){                          
          input = "";  
          lcd.setCursor(0, 1);
          lcd.print("Wrong password");                                 
          Serial.print(F("Incorrect password!!"));      
        }                                               
      }   
      if(input == password){                                                      
        states = 4;                                     
        input = "";                                     
        vTaskDelay(100/portTICK_PERIOD_MS);             
      }else{
        vTaskDelay(100/portTICK_PERIOD_MS);             
      }
      break;
      
      case 3: //Third Stage Arlamed
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Alarming");
      key = keypad.getKey();
      lcd.setCursor(0, 1);
      lcd.print(input);
      Serial.println("Alarming state");
      if(key){                                         
        input += key;                                   
        Serial.println(input);                          
        Serial.println(password);                       
      }

      if(input.length() == 4){                          
        if(input != password){                          
          input = "";  
          lcd.setCursor(0, 1);
          lcd.print("Wrong password");                                 
          Serial.print(F("Incorrect password!!"));      
        }                                               
      }   
      if(input == password){                                                      
        states = 4;                                     
        input = "";                                     
        vTaskDelay(100/portTICK_PERIOD_MS);             
      }else{
        vTaskDelay(100/portTICK_PERIOD_MS);             
      }
      break;
      
      default: //Fourth Stage Not Armed
      vTaskSuspend(TimerTask);
      vTaskSuspend(Sensor1Task);
      vTaskSuspend(Sensor2Task);
      analogWrite(buzzer1, 0);
      analogWrite(buzzer2, 0);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Not Armed");
      key = keypad.getKey();
      lcd.setCursor(0, 1);
      lcd.print(input);
      Serial.println("Non-armed state");
      if(key){                                         
        input += key;                                   
        Serial.println(input);                          
        Serial.println(password);                       
      }

      if(input.length() == 4){                          
        if(input != password){                          
          input = "";  
          lcd.setCursor(0, 1);
          lcd.print("Wrong password");                                 
          Serial.print(F("Incorrect password!!"));      
        }                                               
      }   
      if(input == password){                                                      
        states = 1;                                     
        input = "";                                     
        vTaskDelay(100/portTICK_PERIOD_MS);             
      }else{
        vTaskDelay(100/portTICK_PERIOD_MS);             
      }
      break;
    }
  }
  }
}

static void Task_Sensor1(void* pvParameters){ 
  while(1){
  if(states == 1){
    Sensor1_Value = analogRead(pir1);
    Range_Sensor1 = map(Sensor1_Value, 0, 1023, 0, 255);
    if(Range_Sensor1 > 100){
      states = 2;
    }
    Serial.print(F("task front-door sensing: "));
    Serial.println(Range_Sensor1);
    vTaskDelay(100/portTICK_PERIOD_MS);
  }else if(states == 3){
    if(Range_Sensor1 > 100){
      analogWrite(buzzer1, Range_Sensor1);
      Serial.println(F("task front-door alarm on"));
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }else{
    Serial.println(F("task front-door sensor")); 
    vTaskDelay(100/portTICK_PERIOD_MS); //Blocking 200ms
  }
  }
}

static void Task_Sensor2(void* pvParameters){
  while(1){
  if(states == 1){
    Sensor2_Value = analogRead(pir2);
    Range_Sensor2 = map(Sensor2_Value, 0, 1023, 0, 255);
    if(Range_Sensor2 > 100){
      states = 3;
    }
    Serial.print(F("task window sensing: "));             
    Serial.println(Range_Sensor2); 
    vTaskDelay(100/portTICK_PERIOD_MS);
  }else if(states == 3){
    if(Range_Sensor2 > 100){
      analogWrite(buzzer2, Range_Sensor2);
      Serial.println(F("task window alarm on")); 
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }else{
    Serial.println(F("task window sensor"));
    vTaskDelay(100/portTICK_PERIOD_MS); //Blocking 200ms
  }
  }    
}
