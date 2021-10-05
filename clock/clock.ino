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
const int MSW_INSIDE = 12; // 微動開關內部
const int MSW_OUTSIDE = 8; // 微動開關外部
const int MOTOR_OUT = 9; // 退出馬達
const int MOTOR_IN = 10; // 進入馬達
const int SW_DRAWER = 7; // 抽屜退出開關
const int CHARGE_PIN = 5; //充電指示
/*Pin definitions*/

/*Settings*/
#define HONK_PERIOD 400 //喇叭每段持續時間
#define HONK_FRE 2500 //喇叭音色
#define HONK_WAKE_FRE 2800 //喇叭音色

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
long long lastHonk2 = 0;
long long lastWakeHonk = 0;
bool Century = false;
int alarmH = 0, alarmM = 0, alarmHH = 0, alarmHM = 0;
volatile long count = 0; //旋轉編碼器
unsigned long t = 0;
bool isSetting = 0; //設定鬧鐘
long long lastSet = 0;
int alarmStage = 0; // 鬧鐘設定狀態
#define ALARM_CLOCK 0
#define ALARM_HOUR 1
#define ALARM_MIN 2
#define ALARM_W_HOUR 3
#define ALARM_W_MIN 4
bool isDrawerIn = 1;
bool isLock = 0;
bool isTaken = 0;
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
  pinMode(MSW_OUTSIDE, INPUT);
  pinMode(MSW_INSIDE, INPUT);
  pinMode(MOTOR_IN, OUTPUT);
  pinMode(MOTOR_OUT, OUTPUT);

  //initial alarm clock time
  alarmH = Clock.getHour(h12, PM);
  alarmM = Clock.getMinute() + (Clock.getMinute() > 57 ? 0 : 2);
  alarmHH = alarmH;
  alarmHM = alarmM + 2;

  //initial Motor
  digitalWrite(MOTOR_OUT, 0);
  digitalWrite(MOTOR_IN, 0);
}

void loop() {
//   while(1) {
//     Serial.print(digitalRead(MSW_OUTSIDE));
//     Serial.println(digitalRead(MSW_INSIDE));
//   }
  // 抽屜電機控制
  if (digitalRead(SW_DRAWER) == 0 && isLock == 0) { //按了開關
    Serial.println(isDrawerIn);
    if (isDrawerIn == 1) { // 要退出
      digitalWrite(MOTOR_OUT, 1);
      digitalWrite(MOTOR_IN, 0);
    Serial.println(digitalRead(MSW_OUTSIDE));
        while (digitalRead(MSW_OUTSIDE) != 1) {
          Serial.println("OUT");
        }
      digitalWrite(MOTOR_OUT, 0);
      digitalWrite(MOTOR_IN, 0);
      isDrawerIn = 0;
      Serial.println(isDrawerIn);
      delay(1000);
    } else { // 要吸入
      digitalWrite(MOTOR_OUT, 0);
      digitalWrite(MOTOR_IN, 1);
        while (digitalRead(MSW_INSIDE) != 0) {
          Serial.println("IN");
        }
      digitalWrite(MOTOR_OUT, 0);
      digitalWrite(MOTOR_IN, 0);
      isDrawerIn = 1;
      if (isLocked() == 1)
        isLock = 1;
    }
  }

  // 鬧鐘設定
  if(digitalRead(SW_PIN) == LOW){ // 按下開關，歸零
    Serial.println("Setting");
    if (alarmStage == ALARM_CLOCK) { //進入小時設定
      alarmStage = ALARM_HOUR;
    } else if (alarmStage == ALARM_HOUR) { //進入分鐘設定
      alarmStage = ALARM_MIN;
      //alarmM = timeSet(count, alarmStage, alarmM);
      count = 0;
    } else if (alarmStage == ALARM_MIN) { //進入喚醒小時設定
      alarmStage = ALARM_W_HOUR;
      count = 0;
    } else if (alarmStage == ALARM_W_HOUR) { //進入喚醒分鐘設定
      alarmStage = ALARM_W_MIN;
      count = 0;
    } else { //進入時鐘模式
      alarmStage = ALARM_CLOCK;
      //alarmH = timeSet(count, alarmStage, alarmH);
      count = 0;
    }
    lastSet = millis();
    delay(300);
  }

  if (count != 0) { // catch 旋轉編碼器
    Serial.print(count);
    Serial.println("is rotating!");
    if (alarmStage == ALARM_MIN) { //進入分鐘設定
      alarmM = timeSet(count == 1, alarmStage, alarmM);
      count = 0;
    } else if (alarmStage == ALARM_HOUR) { //進入小時設定
      alarmH = timeSet(count == 1, alarmStage, alarmH);
      count = 0;
    } else if (alarmStage == ALARM_W_HOUR) { //進入喚醒小時設定
      alarmHH = timeSet(count == 1, alarmStage, alarmHH);
      count = 0;
    } else if (alarmStage == ALARM_W_MIN) { //進入喚醒分鐘設定
      alarmHM = timeSet(count == 1, alarmStage, alarmHM);
      count = 0;
    }
  }
  
  ClockDisplayManager(alarmStage);
  //hornManager(1, HONK_PERIOD);
  alarmManager();
  //delay(1000);
}

void hornManager(boolean honkControl, int onPeriod, int offPeriod) {
  if (honkControl == 1) {
    if (millis() >= lastHonk + onPeriod + offPeriod) {
      tone(BUZ_PIN, HONK_FRE, onPeriod);
      lastHonk = millis();
    }
  }
}

void wakeHornManager(boolean honkControl, int onPeriod, int offPeriod) {
  if (honkControl == 1) {
    if (millis() >= lastWakeHonk + onPeriod * 1.5 + offPeriod) {
      tone(BUZ_PIN, HONK_WAKE_FRE, onPeriod * 0.7);
      lastWakeHonk = millis();
    } else if (millis() >= lastWakeHonk + onPeriod * 0.7 && millis() <= lastWakeHonk + onPeriod * 1.5) {
      tone(BUZ_PIN, HONK_WAKE_FRE * 0.8, onPeriod * 0.8);
    }
  }
}

void rotaryEncoderChanged(){ // when CLK_PIN is FALLING (旋轉編碼器)
  unsigned long temp = millis();
  if(temp - t < 200) // 去彈跳
    return;
  t = temp;
 
  // DT_PIN的狀態代表正轉或逆轉
  if(alarmStage != ALARM_CLOCK) {
    count = digitalRead(DT_PIN) == HIGH ? 1 : -1;
  }
}

int timeSet(bool isAdd, int alarmStage, int valueNow) {
  valueNow += (isAdd ? 1 : -1);
  if (valueNow < 0)
    valueNow = ((alarmStage == ALARM_HOUR || alarmStage == ALARM_W_HOUR) ? 23 : 59);
  else if (valueNow >= (alarmStage == ALARM_HOUR || alarmStage == ALARM_W_HOUR ? 24 : 60)) {
    valueNow = 0;
  }
  return valueNow;
}

void alarmManager() {
  if (alarmStage != ALARM_CLOCK)
    return;

  int timeNow = 60 * Clock.getHour(h12, PM) + Clock.getMinute();
  int timeSetted = 60 * alarmH + alarmM;
  int timeAwake = 60 * alarmHH + alarmHM;
  
    Serial.print(alarmH);
    Serial.print(":");
    Serial.println(alarmM);
    Serial.print(Clock.getHour(h12, PM));
    Serial.print(":");
    Serial.println(Clock.getMinute());
    Serial.print(":");
    Serial.println(Clock.getSecond());
  if (timeSetted <= timeNow && timeNow < timeSetted + 1) {
    if (isLocked() == 1)
      return;
    Serial.println("L3");
    hornManager(1, 400, 200);
  } else if (timeSetted + 1 <= timeNow && timeNow < timeSetted + 2) {
    if (isLocked() == 1)
      return;
    Serial.println("L4");
    hornManager(1, 200, 100);
  } else if ((timeSetted == timeNow + 1 && Clock.getSecond() <= 30) || timeSetted == timeNow + 2) {
    if (isLocked() == 1)
      return;
    Serial.println("L1");
    hornManager(1, 500, 9500);
  } else if (timeSetted == timeNow + 1 && Clock.getSecond() > 30) {
    if (isLocked() == 1)
      return;
    Serial.println("L2");
    hornManager(1, 1000, 1000);
  } else if ((timeAwake == timeNow && digitalRead(CHARGE_PIN))) {
    Serial.println("WAKE");
    isLock = 0;
    wakeHornManager(1, 600, 1200);
  }
}

void ClockDisplayManager(int alarmStage) {
  lcd.setCursor ( 0, 0 );            // go to the top left corner

  if (alarmStage == ALARM_CLOCK) {
    lcd.noBlink();
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
    lcd.print("C     ");
  } else if (alarmStage == ALARM_HOUR || alarmStage == ALARM_MIN) {
    lcd.blink();
    lcd.setCursor(0,0);
    lcd.print("====SETTING====");
    lcd.setCursor( 5, 1 );
    lcd.print(" <Bed Time> ");
    lcd.setCursor( 0, 1 );
    // Finally the hour, minute, and second
    if (alarmH < 10)
      lcd.print('0');
    lcd.print(alarmH, DEC);
    lcd.print(':');
    if (alarmM < 10)
      lcd.print('0');
    lcd.print(alarmM, DEC);
    if (alarmStage == ALARM_HOUR)
      lcd.setCursor(0,1);
    else
      lcd.setCursor(3,1);
  } else {
    lcd.blink();
    lcd.setCursor(0,0);
    lcd.print("====SETTING====");
    lcd.setCursor( 5, 1 );
    lcd.print("<Wake Time>");
    lcd.setCursor( 0, 1 );
    // Finally the hour, minute, and second
    if (alarmHH < 10)
      lcd.print('0');
    lcd.print(alarmHH, DEC);
    lcd.print(':');
    if (alarmHM < 10)
      lcd.print('0');
    lcd.print(alarmHM, DEC);
    if (alarmStage == ALARM_W_HOUR)
      lcd.setCursor(0,1);
    else
      lcd.setCursor(3,1);
  }
}

bool isLocked() {
  bool isCharge = digitalRead(CHARGE_PIN); //
  Serial.print("Charge :");
  Serial.println(digitalRead(CHARGE_PIN));
  Serial.print("Locked :");
  Serial.println(isLock);
  bool isDoorCosed = isDrawerIn; //TODO (digitalRead(LOCK_PIN))

  return (isCharge && isDoorCosed);
}
