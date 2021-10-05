/*Header files*/
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
/*Header files*/ 

/*Pin definitions*/
const int BUZ_PIN = A0;
const int CLK_PIN = 2;  // 旋轉編碼器定義連接腳位
const int DT_PIN = 3;   // 旋轉編碼器
const int SW_PIN = 4;   // 旋轉編碼器
const int interruptA = 0; // UNO腳位2是interrupt 0
/*Pin definitions*/

/*Settings*/
#define HONK_PERIOD 400 //喇叭每段持續時間
#define HONK_FRE 2000 //喇叭音色

/*Settings*/

/*Global Variables*/
LiquidCrystal_I2C lcd(0x27, 16, 2); //LCD
DS3231 Clock;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;
boolean isHonk = 0;
long long lastHonk = 0;
bool Century = false;
int alarmH = 0, alarmM = 0;
volatile long count = 0; //旋轉編碼器
unsigned long t = 0;
/*Global Variables*/


void setup() {
  // Start the I2C interface
  Wire.begin();
  // Start the serial interface
  Serial.begin(9600);
  // LCD initialize
  lcd.begin();
  // Turn on the blacklight and print a message.
  lcd.backlight();
  // 旋轉編碼器
  attachInterrupt(interruptA, rotaryEncoderChanged, FALLING);
  pinMode(CLK_PIN, INPUT_PULLUP); // 輸入模式並啟用內建上拉電阻
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP); 
}

void loop() {
  lcd.setCursor ( 0, 0 );            // go to the top left corner
  // Start with the year
  lcd.print("2");
  if (Century) {      // Won't need this for 89 years.
    lcd.print("1");
  } else {
    lcd.print("0");
  }
  lcd.print(Clock.getYear(), DEC);
  lcd.print(' ');
  // then the month
  lcd.print(Clock.getMonth(Century), DEC);
  lcd.print(' ');
  // then the date
  lcd.print(Clock.getDate(), DEC);
  lcd.print(' ');
  // Finally the hour, minute, and second
  lcd.setCursor ( 0, 1 );  // go ti second row
  lcd.print(Clock.getHour(h12, PM), DEC);
  lcd.print(':');
  lcd.print(Clock.getMinute(), DEC);
  lcd.print(':');
  lcd.print(Clock.getSecond(), DEC);
  // Display the temperature
  lcd.print(" T=");
  lcd.print(Clock.getTemperature(), 0);
  lcd.print("C");

  // 鬧鐘設定
  if(digitalRead(SW_PIN) == LOW){ // 按下開關，歸零
    count = 0; 
    Serial.println("count reset to 0");
    delay(300);
  }


  // Tell whether the time is (likely to be) valid
  if (Clock.oscillatorCheck()) {
    Serial.print(" O+");
  } else {
    Serial.print(" O-");
  }
  // Indicate whether an alarm went off
  if (Clock.checkIfAlarm(1)) {
    Serial.print(" A1!");
  }
  if (Clock.checkIfAlarm(2)) {
    Serial.print(" A2!");
  }
  // New line on display
  Serial.print('\n');
  // Display Alarm 1 information
  Serial.print("Alarm 1: ");
  Clock.getA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
  Serial.print(ADay, DEC);
  if (ADy) {
    Serial.print(" DoW");
  } else {
    Serial.print(" Date");
  }
  Serial.print(' ');
  Serial.print(AHour, DEC);
  Serial.print(' ');
  Serial.print(AMinute, DEC);
  Serial.print(' ');
  Serial.print(ASecond, DEC);
  Serial.print(' ');
  if (A12h) {
    if (Apm) {
      Serial.print('pm ');
    } else {
      Serial.print('am ');
    }
  }
  if (Clock.checkAlarmEnabled(1)) {
    Serial.print("enabled");
  }
  Serial.print('\n');
  // Display Alarm 2 information
  Serial.print("Alarm 2: ");
  Clock.getA2Time(ADay, AHour, AMinute, ABits, ADy, A12h, Apm);
  Serial.print(ADay, DEC);
  if (ADy) {
    Serial.print(" DoW");
  } else {
    Serial.print(" Date");
  }
  Serial.print(' ');
  Serial.print(AHour, DEC);
  Serial.print(' ');
  Serial.print(AMinute, DEC);
  Serial.print(' ');
  if (A12h) {
    if (Apm) {
      Serial.print('pm');
    } else {
      Serial.print('am');
    }
  }
  if (Clock.checkAlarmEnabled(2)) {
    Serial.print("enabled");
  }
  // display alarm bits
  Serial.print('\nAlarm bits: ');
  Serial.print(ABits, BIN);

  Serial.print('\n');
  Serial.print('\n');
  hornManager(1);
  delay(1000);
}

void hornManager(boolean honkControl) {
  if (honkControl == 1) {
    if (millis() >= lastHonk + HONK_PERIOD * 2) {
      tone(BUZ_PIN, HONK_FRE, HONK_PERIOD);
      lastHonk = millis();
    }
  }
}

void rotaryEncoderChanged(){ // when CLK_PIN is FALLING (旋轉編碼器)
  unsigned long temp = millis();
  if(temp - t < 200) // 去彈跳
    return;
  t = temp;
 
  // DT_PIN的狀態代表正轉或逆轉
  count += digitalRead(DT_PIN) == HIGH ? 1 : -1;
  Serial.println(count);
}