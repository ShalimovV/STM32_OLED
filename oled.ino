#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <STM32RTC.h>
#include <Tone.h>

//git
/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 25;
const byte hours = 14;

/* Change these values to set the current initial date */
/* Monday 15th June 2015 */
const byte weekDay = 5;
const byte day = 13;
const byte month = 3;
const byte year = 20;

#define OLED_RESET 4
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);



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
  delay(2000);
  display.clearDisplay();

  // Select RTC clock source: LSI_CLOCK, LSE_CLOCK or HSE_CLOCK.
  // By default the LSI is selected as source.
  rtc.setClockSource(STM32RTC::LSE_CLOCK);

  rtc.begin(); // initialize RTC 24H format
  //rtc.setTime(hours, minutes, seconds);
  //rtc.setDate(weekDay, day, month, year);

  tone(zoomerPin, 2300, 100);
}

void animateCircle(uint8_t x, uint8_t y, uint8_t diam) {
  uint8_t i;

  for (i = diam; i > 0; i--) {
    display.drawCircle(x, y, i, SSD1306_WHITE);
    display.display();
  }

}

void alarm(uint8_t repeat) {
  for (uint8_t i = 0; i < repeat; i++) {
    tone(zoomerPin, 2300, 200);
    
    delay(200);
    noTone(zoomerPin);
    delay(200);
    scan_btn();
    if (!BtnExit | !BtnMenu | !BtnUp | !BtnDown) {
      return;
    }

  }
}

void scan_btn(void) {

  BtnUp = digitalRead(btn4Pin);
  BtnDown = digitalRead(btn3Pin);
  BtnMenu = digitalRead(btn2Pin);
  BtnExit = digitalRead(btn1Pin);
  BtnState = BtnUp << 0 | BtnDown << 1 | BtnMenu << 2 | BtnExit << 3 ;
}

void menu(void) {
  uint8_t pos = 0;
  bool item[5] = {0, 0, 0, 0, 0};
  display.clearDisplay();
  display.display();
  display.setTextSize(1);

  while (1) {

    scan_btn();
    if (!BtnExit) return;
    display.setCursor(display.width() / 2 - 5, 0);
    display.println("Menu");
    if (!BtnUp) {
      pos--;
      display.clearDisplay();
      display.display();
      delay(50);
    }
    if (!BtnDown) {
      pos++;
      display.clearDisplay();
      display.display();
      delay(50);
    }
    if (pos > 4) pos = 0;
    if (!BtnMenu) {
      item[pos] = !item[pos];
      display.clearDisplay();
      display.display();
    }

    display.setCursor(1, 17);
    display.println("Set time");
    display.setCursor(100, 17);
    if (item[0]) display.println("Yes");
    else display.println("No");

    display.setCursor(1, 26);
    display.println("Alarm");
    display.setCursor(100, 26);
    if (item[1]) {
      display.println("Yes");
      alarm(5);
      item[1] = false;
    }
    else display.println("No");

    display.setCursor(1, 35);
    display.println("Item3");
    display.setCursor(100, 35);
    if (item[2]) display.println("Yes");
    else display.println("No");

    display.setCursor(1, 44);
    display.println("Item4");
    display.setCursor(100, 44);
    if (item[3]) display.println("Yes");
    else display.println("No");

    display.setCursor(1, 53);
    display.println("Item5");
    display.setCursor(100, 53);
    if (item[4]) display.println("Yes");
    else display.println("No");

    display.drawRect(0, 16 + (pos * 9), display.width(), 10, SSD1306_WHITE);
    display.display();
    delay(50);
    switch (pos) {
      case 0:

        break;
      case 1:

        break;
      default:
        break;
    }
  }


}

void showTime(uint8_t x, uint8_t y, uint8_t txtSize = 1) {
  String time = String(8);

  time = rtc.getHours();
  time += ':';
  time += rtc.getMinutes();
  time += ':';
  time += rtc.getSeconds();

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
}

void showDate(uint8_t x, uint8_t y, uint8_t txtSize = 1) {
  String time = String(8);

  time = rtc.getDay();
  time += '.';
  time += rtc.getMonth();
  time += '.';
  time += rtc.getYear();

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
}


void loop() {
  // put your main code here, to run repeatedly:
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  showTime(0, 0, 2);
  //showDate(60,9);
  scan_btn();
  if (!BtnMenu) menu();
  display.setCursor(0, 18);
  display.println(BtnState);

  //animateCircle(display.width() / 2, display.height() / 2, 16);

  display.display();
  /*
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);
  */
}
