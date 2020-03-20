#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <STM32RTC.h>
#include <Tone.h>
#include <STM32FreeRTOS.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP085.h>



//OLED
#define OLED_RESET 4
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
bool display_on = true;
bool display_dim_flag=true;
int displayOff_period = 30;

//Buttons and zummer
#define btn1Pin PA0
#define btn2Pin PA1
#define btn3Pin PA2
#define btn4Pin PA3
#define zoomerPin PA8

byte BtnState = 0;
bool BtnUp = 1;
bool BtnDown = 1;
bool BtnMenu = 1;
bool BtnExit = 1;

/* Get the RTC object */
STM32RTC& rtc = STM32RTC::getInstance();
bool alarmSet = false;
bool alarmGo = false;

// define tasks for FreeRTOS
void scan_btn( void *pvParameters );
void mainScreen( void *pvParameters );
void alarm( void *pvParameters );
void ADC_read( void *pvParameters );
xTaskHandle xHandle;

//Accelerometr
Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;
bool gyro_enable = false;
bool move_detected = false;

//BMP180
Adafruit_BMP085 bmp;
bool pressure_flag = false;

//Bat
float V_bat;
int bat_level;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(btn1Pin, OUTPUT);
  pinMode(btn2Pin, OUTPUT);
  pinMode(btn3Pin, OUTPUT);
  pinMode(btn4Pin, OUTPUT);

  digitalWrite(btn1Pin, HIGH);
  digitalWrite(btn2Pin, HIGH);
  digitalWrite(btn3Pin, HIGH);
  digitalWrite(btn4Pin, HIGH);


  //Adafruit_SSD1306(128, 64, TwoWire);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Display init....done");
  display.display();
  delay(200);

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  // By default the LSI is selected as source.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);
  rtc.begin(); // initialize RTC 24H format
  display.setCursor(0, 16);
  display.println("RTC init....done");
  display.display();
  delay(200);

  display.setCursor(0, 28);
  if (!mpu.begin()) display.println("MPU6050 init..fail");
  else {
    display.println("MPU6050 init..done");
    gyro_enable = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  display.display();
  delay(200);

  display.setCursor(0, 40);
  if (!bmp.begin()) display.println("BMP180 Init...fail");
  else {
    display.println("BMP180 Init...done");
    pressure_flag = true;
  }
  display.display();
  delay(200);

  // Now set up two tasks to run independently.
  xTaskCreate(
    scan_btn
    ,  (const portCHAR *)"Scan button"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    mainScreen
    ,  (const portCHAR *) "Main screen"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );
  xTaskCreate(
    alarm
    ,  (const portCHAR *) "Alarm"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &xHandle );

  xTaskCreate(
    ADC_read
    ,  (const portCHAR *) "ADC read data"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );


  display.setCursor(0, 52);
  display.println("Starting FreeRTOS");
  display.display();
  delay(2000);

  tone(zoomerPin, 2300, 100);

  vTaskSuspend( xHandle );
  // start scheduler
  vTaskStartScheduler();


}


void alarm(void *pvParameters) {
  (void) pvParameters;

  for (;;) {

    for (uint8_t i = 0; i < 10; i++) {
      tone(zoomerPin, 2300, 200);

      vTaskDelay(200);
      noTone(zoomerPin);
      vTaskDelay(200);
      if (!BtnExit | !BtnMenu | !BtnUp | !BtnDown) {
        vTaskSuspend( NULL );
      }
    }
    vTaskSuspend( NULL );
  }
}


void ADC_read(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    int adc_data = analogRead(A7);    // read the ADC value from pin B0
    V_bat = (float(adc_data) / 4096) * 3.3 * 24; //formulae to convert the ADC value to voltage
    bat_level = (V_bat - 3.5) / 0.007;
    vTaskDelay(100);
  }
}


void setupTime(bool alarm = false) {
  char time[8];
  uint8_t  pos = 0, space = 0, tmpTime[3] = {rtc.getHours(), rtc.getMinutes(), rtc.getSeconds()};
  display.clearDisplay();
  display.display();
  while (1) {
    vTaskDelay(100);
    if (!BtnExit) {
      vTaskDelay(200);
      return;
    }

    if (!BtnUp) {
      tmpTime[pos]++;
      if (tmpTime[0] > 23) tmpTime[0] = 0;
      if (tmpTime[1] > 60) tmpTime[1] = 0;
      if (tmpTime[2] > 60) tmpTime[2] = 0;
      else space = 0;
      display.clearDisplay();
      display.display();
      vTaskDelay(20);
    }

    if (!BtnDown) {
      tmpTime[pos]--;
      if (tmpTime[0] > 23) tmpTime[0] = 0;
      if (tmpTime[1] > 60) tmpTime[1] = 0;
      if (tmpTime[2] > 60) tmpTime[2] = 0;
      display.clearDisplay();
      display.display();
      vTaskDelay(20);
    }

    if (!BtnMenu) {
      pos++;
      if (pos > 3) pos = 0;
      display.clearDisplay();
      display.display();
    }

    sprintf(time, "%02d:%02d:%02d", tmpTime[0], tmpTime[1], tmpTime[2]);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(time);
    display.setCursor(0, 20);
    display.println("Set?");
    if (pos == 3) {
      display.drawRect(0 , 20 , 48, 16, SSD1306_WHITE);
      if (!BtnUp) {
        if (alarm) {
          rtc.setAlarmTime(tmpTime[0], tmpTime[1], tmpTime[2]);
          rtc.enableAlarm(rtc.MATCH_HHMMSS);
          rtc.attachInterrupt(alarmMatch);
          alarmSet = true;
        }
        else rtc.setTime(tmpTime[0], tmpTime[1], tmpTime[2]);
        return;
      }
    }
    else display.drawRect(0 + pos * 36, 0 , 24, 16, SSD1306_WHITE);
    display.display();
  }
}

void alarmMatch(void *data)
{
  UNUSED(data);
  alarmGo = true;
  alarmSet = false;
  rtc.disableAlarm();
}

void scan_btn(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    BtnUp = digitalRead(btn4Pin);
    BtnDown = digitalRead(btn3Pin);
    BtnMenu = digitalRead(btn2Pin);
    BtnExit = digitalRead(btn1Pin);
    BtnState = BtnUp << 0 | BtnDown << 1 | BtnMenu << 2 | BtnExit << 3 ;

    if (alarmGo) {
      vTaskResume( xHandle );
      alarmGo = false;
    }

    vTaskDelay(10);
  }
}

void menu(void) {
  uint8_t pos = 0;
  bool item[5] = {0, 0, 0, 0, 0};
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  if (!BtnMenu) vTaskDelay(300);

  while (1) {

    if (!BtnExit) return;

    display.setCursor(display.width() / 2 - 5, 0);
    display.println("Menu");
    if (!BtnUp) {
      pos--;
      display.clearDisplay();
      display.display();
      vTaskDelay(50);
    }
    if (!BtnDown) {
      pos++;
      display.clearDisplay();
      display.display();
      vTaskDelay(50);
    }
    if (pos > 4) pos = 0;
    if (!BtnMenu) {
      item[pos] = !item[pos];
      display.clearDisplay();
      display.display();
    }

    display.setCursor(1, 17);
    display.println("Setup time");
    display.setCursor(100, 17);
    if (item[0]) {
      setupTime();
      display.setTextSize(1);
      display.clearDisplay();
      item[0] = 0;
    }


    display.setCursor(1, 26);
    display.println("Setup Alarm");
    display.setCursor(100, 26);
    if (item[1]) {
      setupTime(true);
      display.setTextSize(1);
      display.clearDisplay();
      item[1] = 0;
    }


    display.setCursor(1, 35);
    display.println("Dim display");
    display.setCursor(100, 35);
    if (item[2]) {
      display.println("Yes");
      display.dim(true);
      display_dim_flag=false;
    }
    else {
      display.println("No");
      display.dim(false);
      display_dim_flag=true;
    }

    display.setCursor(1, 44);
    display.println("Display off time");
    display.setCursor(100, 44);
    display.println(displayOff_period);
    if (item[3]) {
      displayOff_period += 10;
      if (displayOff_period > 60) displayOff_period = 0;
      display.clearDisplay();
      display.display();
      item[3] = !item[3];
    }
    

    display.setCursor(1, 53);
    display.println("Item5");
    display.setCursor(100, 53);
    if (item[4]) display.println("Yes");
    else display.println("No");

    display.drawRect(0, 16 + (pos * 9), display.width(), 10, SSD1306_WHITE);
    display.display();
    vTaskDelay(50);

  }
}


void showTime(uint8_t x, uint8_t y, uint8_t txtSize = 1) {
  char time[8];

  sprintf(time, "%02d:%02d:%02d", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

  if (txtSize == 1) {
    display.setTextSize(1);
  }
  if (txtSize == 2) {
    display.setTextSize(2);
  }
  if (txtSize == 3) {
    display.setTextSize(3);
  }

  display.setCursor(x, y);
  display.println(time);
  if (alarmSet) {
    display.setCursor(110, 0);
    display.println("*");
  }
}

void show_gyro(void) {
  mpu.getEvent(&a, &g, &temp);

  if (g.gyro.y > 0.5 or g.gyro.x > 0.5 or g.gyro.z > 0.5) move_detected = true;
  else move_detected = false;

  if (display_on) {
    display.setCursor(0, 25);
    display.println("Gyro:");
    display.setCursor(0, 35);
    display.println(g.gyro.x);
    display.setCursor(0, 45);
    display.println(g.gyro.y);
    display.setCursor(0, 55);
    display.println(g.gyro.z);

    display.setCursor(40, 25);
    display.println("Accel:");
    display.setCursor(40, 35);
    display.println(a.acceleration.x);
    display.setCursor(40, 45);
    display.println(a.acceleration.y);
    display.setCursor(40, 55);
    display.println(a.acceleration.z);

    display.setCursor(80, 25);
    display.println("Temp:");
    display.setCursor(80, 35);
    display.println(temp.temperature);

    display.setCursor(80, 45);
    display.println(V_bat);
    display.setCursor(80, 55);
    display.println(int(bat_level));
  }
}


void mainScreen(void *pvParameters) {
  (void) pvParameters;
  int displayOff_timer = 0;


  for (;;) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    if (display_on) showTime(0, 0, 2);

    if (!move_detected) {
      displayOff_timer++;
      vTaskDelay(100);
      if (displayOff_timer >= (displayOff_period * 5)/2) display.dim(true);
      if (displayOff_timer >= displayOff_period * 5) {
        display_on = false;
      }
    }
    else {
      display_on = true;
      displayOff_timer = 0;
      if (display_dim_flag) display.dim(false);
    }

    if (!BtnMenu) menu();
    display.setTextSize(1);
    display.setCursor(0, 16);
    if (display_on) display.println(BtnState);
    
    if (pressure_flag and display_on) {
      display.setCursor(20, 16);
      display.print("P=");
      display.print(bmp.readPressure() * 0.0075);
      display.print(" T=");
      display.print(bmp.readTemperature());
    }
    if (gyro_enable) show_gyro();

    if (bat_level <= 20) {
      bmp.begin(BMP085_ULTRALOWPOWER);
      mpu.enableSleep(true);
      pressure_flag = false;
      gyro_enable = false;
      display.setCursor(0, 26);
      display.println("Low battery!");
      display.setCursor(0, 36);
      display.println("Power saveing on!");
      //LowPower.deepSleep(1000);
    }
    else {
      bmp.begin();
      mpu.enableSleep(false);
      pressure_flag = true;
      gyro_enable = true;
    }

    display.display();
    vTaskDelay(10);
  }
}

void loop() {

}
