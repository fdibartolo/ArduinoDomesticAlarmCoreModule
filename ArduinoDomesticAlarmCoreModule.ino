#include <LiquidCrystal.h>

/* keyboard */
const int pinCol1 = 11;
const int pinCol2 = 12;
const int pinCol3 = 13;
const int pinRow1 = A0;
const int pinRow2 = A1;
const int pinRow3 = A2;
const int pinRow4 = A3;

/* lcd */
const int pinLCD_D4 = 5;
const int pinLCD_D5 = 4;
const int pinLCD_D6 = 3;
const int pinLCD_D7 = 2;
const int pinLCD_E = 6;
const int pinLCD_RS = 7;

const int LCD_NumberOfColumns = 16;
const int LCD_NumberOfRows = 2;
const char LCD_EmptyString[] = "                ";
LiquidCrystal LCD(pinLCD_RS, pinLCD_E, pinLCD_D4, pinLCD_D5, pinLCD_D6, pinLCD_D7);

const int pinOutputLed = 8;
const int pinOutputBuzz = 9;
const int pinSensor = A5;

const int keysCol1[] = {1,4,7,10};
const int keysCol2[] = {2,5,8,0};
const int keysCol3[] = {3,6,9,11};

const int code[5] = {1,2,3,4};
int password[5]; 

boolean alarmActivatedFlag = false;
unsigned long activatedCountDown = 0;
int activationThreshold = 25; //seconds (max 99 or change print countdown char declaration!!)

boolean alarmTriggeredFlag = false;
unsigned long triggeredCountDown = 0;
int triggeredThreshold = 25; //seconds (max 99 or change print countdown char declaration!!)

boolean isLcdUpdateNeeded = false;

void setup() {
  pinMode(pinCol1, OUTPUT);
  pinMode(pinCol2, OUTPUT);
  pinMode(pinCol3, OUTPUT);
  pinMode(pinRow1, INPUT);
  pinMode(pinRow2, INPUT);
  pinMode(pinRow3, INPUT);
  pinMode(pinRow4, INPUT);

  pinMode(pinOutputLed, OUTPUT);
  pinMode(pinOutputBuzz, OUTPUT);
  
  LCD.begin(LCD_NumberOfColumns, LCD_NumberOfRows);
  LCD.print("Alarma Desactivada");

  Serial.begin(9600);
}

void loop() {
  int scannedCol = 1;
  int key;
  int index = 0;
  int previousCount = -1;

  //initial state
  digitalWrite(pinOutputBuzz, LOW);
  digitalWrite(pinOutputLed, LOW);
  
  while(1){
    // read keyboard
    ScanColumn(scannedCol);    
    key = ReadEntryKey();
    
    if (key > 0){
      int decodedKey = DecodeKey(scannedCol, key);

      if ((index >= 0) && (decodedKey >= 0) && (decodedKey <= 9))
      {
        password[index] = decodedKey;
        
        Serial.println(decodedKey);
        PrintKeyToLCD(index, decodedKey);

        if (index < 3)
          index++;
        else
          index = -1;
      }
      else 
      {
        if (decodedKey == 11){
          alarmActivatedFlag = IsPasswordMatched();
          ClearEntry();
          index = 0;
          PrintTemporalMessageToLCD(index, "Enter!");
        }
        
        if (decodedKey == 10){
          ClearEntry();
          index = 0;
          PrintTemporalMessageToLCD(index, "Clear!");
        }
      }
      
      WaitForKeyIsReleased(key);
    }
    
    scannedCol++;
    if (scannedCol > 3)
      scannedCol = 1;

    if (alarmActivatedFlag)
    {
      digitalWrite(pinOutputLed, HIGH);
      
      unsigned long now = millis();
      if (((now - activatedCountDown) / 1000) < activationThreshold)
      {
        int count = activationThreshold - ((now - activatedCountDown) / 1000);
        if (count != previousCount)
          PrintCountdownToLCD(count);
          
        if ((count == 1) && (count != previousCount)) {
          ClearRowOfLCD(0, 1);
          PlayActivationSound();
        }

        previousCount = count;
      }
      else
      {
        if (alarmTriggeredFlag)
        {
          unsigned long now = millis();
          if (((now - triggeredCountDown) / 1000) < triggeredThreshold)
          {
            int count = triggeredThreshold - ((now - triggeredCountDown) / 1000);
            if (count != previousCount)
              PrintCountdownToLCD(count);
          
            previousCount = count;

            if (count == 1)
              ClearRowOfLCD(0, 1);
          }
          else 
            PlayTriggeredSound();
        }
        else
          alarmTriggeredFlag = AreSensorsTriggered();
      }
    }
    else
      ClearAllFlagAndOutput();
      
    if (isLcdUpdateNeeded) {
      isLcdUpdateNeeded = false;
      PrintAlarmStatusToLCD();
    }
  }
}

void ScanColumn(int colNumber){  
  switch (colNumber){
    case 1:
      digitalWrite(pinCol1, HIGH);
      digitalWrite(pinCol2, LOW);
      digitalWrite(pinCol3, LOW);
      break;
    case 2:
      digitalWrite(pinCol1, LOW);
      digitalWrite(pinCol2, HIGH);
      digitalWrite(pinCol3, LOW);
      break;
    case 3:
      digitalWrite(pinCol1, LOW);
      digitalWrite(pinCol2, LOW);
      digitalWrite(pinCol3, HIGH);
      break;
  }
}

int ReadEntryKey(){
  if (digitalRead(pinRow1) == HIGH)
    return 1;

  if (digitalRead(pinRow2) == HIGH)
    return 2;
  
  if (digitalRead(pinRow3) == HIGH)
    return 3;
   
  if (digitalRead(pinRow4) == HIGH)
    return 4;
  
  return 0;
}

void WaitForKeyIsReleased(int pressedKey){
  int value;
  int pressedRow;
  
  switch (pressedKey){
    case 1:
      pressedRow = pinRow1;
      break;
    case 2:
      pressedRow = pinRow2;
      break;
    case 3:
      pressedRow = pinRow3;
      break;
    case 4:
      pressedRow = pinRow4;
      break;
  }
  
  do{
    value = digitalRead(pressedRow);
  }while(value == HIGH);  
}

int DecodeKey(int col, int row){
  int number = 0;
  
  if (col == 1)
    number = keysCol1[row-1];
  else if (col == 2)
    number = keysCol2[row-1];
  else if (col == 3)
    number = keysCol3[row-1];
  
  return number;
}

void ClearEntry(){
  for (int i=0; i<4; i++){
    password[i]=-1;
  }
  
  Serial.println("pwd cleared!");  
}

boolean IsPasswordMatched(){
  if ((password[0] == code[0]) && (password[1] == code[1]) && (password[2] == code[2]) && (password[3] == code[3]))
  {
    activatedCountDown = millis(); //initialize activation window
    isLcdUpdateNeeded = true;
    return !alarmActivatedFlag;
  }
  else
    return alarmActivatedFlag;
}

boolean AreSensorsTriggered(){
  if (digitalRead(pinSensor) == HIGH){
    Serial.println("Sensor triggered!");
    triggeredCountDown = millis(); //initialize triggered window
    isLcdUpdateNeeded = true;
    return true;
  }
  else
    return false;
}

void ClearAllFlagAndOutput(){
  alarmTriggeredFlag = false;
  digitalWrite(pinOutputLed, LOW);
  digitalWrite(pinOutputBuzz, LOW);
}

void PrintKeyToLCD(int index, int pressedKey){
  LCD.setCursor(index, 1);
  LCD.print(pressedKey);  
}

void PrintTemporalMessageToLCD(int index, char message[]){
  LCD.setCursor(index, 1);
  LCD.print(message);  
  delay(1000);
  ClearRowOfLCD(index, 1);
}

void PrintAlarmStatusToLCD(){
  ClearRowOfLCD(0, 0);
  LCD.setCursor(0, 0);
  if (!alarmActivatedFlag)
    LCD.print("Alarma Desactivada");  
  else if (alarmTriggeredFlag)
    LCD.print("Alarma Disparada");  
  else
    LCD.print("Alarma Activada");  
}

void PrintCountdownToLCD(int count){
  char formattedCount[3];
  formattedCount[0] = '0' + (count / 10);
  formattedCount[1] = '0' + (count % 10);
  formattedCount[2] = '\0';

  LCD.setCursor(14, 1);
  LCD.print(formattedCount);  
}

void ClearRowOfLCD(int col, int row){
  LCD.setCursor(col, row);
  LCD.print(LCD_EmptyString);  
}

void PlayTriggeredSound(){
  unsigned long now = millis();
  if (((now - triggeredCountDown) / 1000) < 3600) //will play for 3600 sec (1hr) unless it is deactivated
    digitalWrite(pinOutputBuzz, HIGH);
  else {
    isLcdUpdateNeeded = true;
    ClearAllFlagAndOutput(); 
  }
}

void PlayActivationSound(){
  digitalWrite(pinOutputBuzz, HIGH);
  delay(200);  
  digitalWrite(pinOutputBuzz, LOW);
}

