/*
 * matej_sat_v0.0.4 - verzija programa
 * 2023-03-14 - kopija verzije matej_sat_v0.0.3
 *   - todo: dodati komentare
 *     - default mode je prikaz sata
 *     - default svjetlina je 1 - spremit svjetlinu u memoriju
*/
// library for matrix led display
#include "LedControl.h"

#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Button.h>

#include "fonts.h"

// minimalni intenzitet ledica
# define INTENSITY_MIN 0
// maksimalni itenzitet ledica
# define INTENSITY_MAX 15

#define BUTTON_LONG_PRESS_MIN 2000

// broj modova prikaza: 100 sat, 200 datum, 300 godina, 400 test, 500 minimum
# define NO_OF_MODES 5

# define MODE_SHOW_TIME 100
  # define MODE_SET_HOUR 101
  # define MODE_SET_MINUTE 102
  # define MODE_SET_SECOND 103

# define MODE_SHOW_DATE 200
  # define MODE_SET_DAY 201
  # define MODE_SET_MONTH 202

# define MODE_SHOW_YEAR 300
  # define MODE_SET_YEAR 301

# define MODE_TEST 400
  # define MODE_TEST_ALL_LEDS_ON 401
  # define MODE_TEST_ALL_LEDS_OFF 402
  # define MODE_TEST_BLINK_ALL_LEDS 403
  # define MODE_TEST_MINIMUM 404
  # define MODE_TEST_FUNCTION 405

# define MODE_BRIGHTNESS 500

/*
# define MODE_FAST_INCREMENT 501
# define MODE_FAST_DECREMENT 502
# define MODE_NORMAL_INCREMENT 503
# define MODE_NORMAL_DECREMENT 504
*/

/*
  Now we need a LedControl to work with.
  pin 12 is connected to the DataIn
  pin 11 is connected to the CLK
  pin 10 is connected to LOAD
  We have only a single MAX72XX.
*/

tmElements_t tm_set;
tmElements_t tm;

// inicijalna svjetlina nakon reseta 1
int ledBrightness = 0;
// inicijalni prikaz nakon reseta 100 - prikaz sata
int programMode = 100;

// maksimalni pod mod moda prikaza
int modeMaxes[NO_OF_MODES] = {103,202,301,405,500};

LedControl lc = LedControl(12, 11, 10, 4);

Button button[3] = {
  Button(3, PULLUP),
  Button(4, PULLUP),
  Button(5, PULLUP)
};

/* we always wait a bit between updates of the display */
unsigned long delaytime = 1000;

byte font_5x7[13 * 5] = {
  0x7E, 0x81, 0x81, 0x81, 0x7E,// 0
  0x84, 0x82, 0xFF, 0x80, 0x80,// 1
  0xC2, 0xA1, 0x91, 0x89, 0x86,// 2
  0x42, 0x81, 0x89, 0x89, 0x76,// 3
  0x30, 0x28, 0x24, 0x22, 0xFF,// 4
  0x4F, 0x89, 0x89, 0x89, 0x71,// 5
  0x7E, 0x89, 0x89, 0x89, 0x72,// 6
  0x01, 0xE1, 0x11, 0x09, 0x07,// 7
  0x76, 0x89, 0x89, 0x89, 0x76,// 8
  0x46, 0x89, 0x89, 0x89, 0x7E,// 9
  0x00, 0x88, 0xFA, 0x80, 0x00,// i
  0x00, 0x88, 0xF8, 0x80, 0x00,// i bez tocke
  0x00, 0x00, 0x00, 0x00, 0x00,// space
};

byte sunce[8] = {0x42,0x24,0x18,0x3f,0xfc,0x18,0x24,0x42};

int hours = 0;
int minutes = 0;
int seconds = 0;

int days;
int months;
int years;

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  delay(200);
  Serial.println("Program version: matej_sat_v0.0.4");
  Serial.println("DS3231 RTC");
  Serial.println("-------------------");

  /*
    The MAX72XX is in power-saving mode on startup,
    we have to do a wakeup call
  */
  lc.shutdown(0, false);
  lc.shutdown(1, false);
  lc.shutdown(2, false);
  lc.shutdown(3, false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0, ledBrightness);
  lc.setIntensity(1, ledBrightness);
  lc.setIntensity(2, ledBrightness);
  lc.setIntensity(3, ledBrightness);
  /* and clear the display */

  clearDisplay();

  //printStatus();
  //shitOut();
  //lc.flush();
}

void initializeTime() {
  tm_set.Second = 0;
  tm_set.Minute = 0;
  tm_set.Hour = 0;
  tm_set.Wday = 2;
  tm_set.Day = 14;
  tm_set.Month = 3;
  tm_set.Year = CalendarYrToTm(2023);
}

// Testna funkcija, ne koristi se - test prikaza 
void printStatus() {
  //lc.setLed(0,0,0,1);
  //lc.setLed(0,0,1,1);

  writeHours(23);
  writeSeparator(1);
  writeMinutes(59);

  for (int i = 0; i < 8; i++) {
    for (int k = 0; k < 4; k++) {
      for (int j = 0; j < 8; j++) {
        if ((lc.status[8 * k + 7 - i] >> j) & 1)
          Serial.print('8');
        else
          Serial.print('.');
        Serial.print(' ');
      }
    }
    Serial.println();
  }
  Serial.println();
}

void writeHours(int inHours) {
  writeNumber(inHours / 10, 1);
  writeNumber(inHours % 10, 7);
}

void writeSeparator(int isWithDot) {
  writeNumber(isWithDot ? 10 : 11, 13);
}

void writeMinutes(int inMinutes) {
  writeNumber(inMinutes / 10, 19);
  writeNumber(inMinutes % 10, 25);
}

void writeTimeHHiMM() {
  /* here is the data for the characters */
  writeHours(hours);
  writeSeparator(seconds % 2);
  writeMinutes(minutes);
}

void writeDay() {
  writeNumber(days / 10, 2);
  writeNumber(days % 10, 8);
  lc.setLed(1, 7, 6, 1);
}

void writeMonth () {
  writeNumber(months / 10, 17);
  writeNumber(months % 10, 23);
  lc.setLed(3, 7, 5, 1);
}

void writeYear() {
  int thousands = years / 1000;
  int hunderts = (years % 1000) / 100;
  int tens = (years % 100) / 10;
  int ones = years % 10;

  int offest = 4;

  writeNumber(thousands, 0 + offest);
  writeNumber(hunderts, 6 + offest);
  writeNumber(tens, 12 + offest);
  writeNumber(ones, 18 + offest);
}

// ispiši broj/slovo na ekran
// inNum - broj znaka iz font_5x7
// inCol kolona od koje počinjemo ispis
void writeNumber(int inNum, int inCol) {
  int _segNo;
  int _colNo;
  int _currCol;

  if (inCol < 0 || inCol > 31) {
    return;
  }

  _currCol = inCol;

  for (int i = 0; i < 5; i++) {
    if (_currCol > 31) return;
    lc.setColumn(_currCol / 8, _currCol % 8, font_5x7[inNum * 5 + i]);
    _currCol++;
  }
}

void loop() {

  if (RTC.read(tm)) {
    // Normalan način rada kada uspijemo očitati vrijeme iz RTC-a
    /*
          Serial.print("Ok, Time = ");
          print2digits(tm.Hour);
          Serial.write(':');
          print2digits(tm.Minute);
          Serial.write(':');
          print2digits(tm.Second);
          Serial.print(", Date (D/M/Y) = ");
          Serial.print(tm.Day);
          Serial.write('/');
          Serial.print(tm.Month);
          Serial.write('/');
          Serial.print(tmYearToCalendar(tm.Year));
          Serial.println();
    */
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
      initializeTime();
      if (RTC.write(tm_set)) {
        Serial.println("Time set to init time!");
      }
      else {
        showError();
        lc.flush();
        while (1);
      }
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
      showError();
      lc.flush();
      while (1);
    }
  }

  //Serial.print("Calibration = [");
  //Serial.print(RTC.getCalibration(),BIN);
  //Serial.println("]");

  hours = tm.Hour;
  minutes = tm.Minute;
  seconds = tm.Second;

  days = tm.Day;
  months = tm.Month;
  years = tmYearToCalendar(tm.Year);

  
  // PRIKAZ MODA
  // gledamo u kojem smo modu programa i prema tome prikazujemo na ekran
  switch (programMode) {
    // SHOW MODES
    case MODE_SHOW_TIME:
      writeTimeHHiMM();
      break;
    case MODE_SHOW_DATE:
      writeDay();
      writeMonth();
      break;
    case MODE_SHOW_YEAR:
      writeYear();
      break;

    // SET MODES
    case MODE_SET_HOUR:
      showSetHour();
      break;
    case MODE_SET_MINUTE:
      showSetMinute();
      break;
    case MODE_SET_SECOND:
      showSetSecond();
      break;
    case MODE_SET_DAY:
      showSetDay();
      break;
    case MODE_SET_MONTH:
      showSetMonth();
      break;
    case MODE_SET_YEAR:
      showSetYear();
      break;

    // TEST MODES
    case MODE_TEST:
      writeString("test");
      break;
    case MODE_TEST_ALL_LEDS_ON:
      showAllLedsOn();
      break;
    case MODE_TEST_ALL_LEDS_OFF:
      showAllLedsOff();
      break;
    case MODE_TEST_BLINK_ALL_LEDS:
      blinkAllLeds();
      break;
    case MODE_TEST_MINIMUM:
      writeMinimum();
      break;
    case MODE_TEST_FUNCTION:
      writeString("test");
      break;

    case MODE_BRIGHTNESS:
      showSetBrightness();
      break;

    // DEFAULT
    default:
      showError();
      break;
  }
  lc.flush();
  delay(100);
  checkButtons();
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

// set modes
// prikaz podešavanja sata, pali i gasi znamenke sata svake sekunde
void showSetHour() {
  if ((millis() / 500) % 2) {
    writeNumber(12, 1);
    writeNumber(12, 7);
  }
  else {
    writeHours(hours);
  }
  writeSeparator(1);
  writeMinutes(minutes);
}

// prikaz podešavanja minuta, pali i gasi znamenke minute svake sekunde
void showSetMinute() {
  if ((millis() / 500) % 2) {
    writeNumber(12, 19);
    writeNumber(12, 25);
  }
  else {
    writeMinutes(minutes);
  }
  writeSeparator(1);
  writeHours(hours);
}

// prikaz podešavanja sekundi, pali i gasi znamenke minute svake sekunde
void showSetSecond() {
  if ((millis() / 500) % 2) {
    clearDisplay();
  }
  else {
    writeMinutes(seconds);
  }
}

// prikaz podešavanja dana, pali i gasi znamenke dana
void showSetDay() {
  if ((millis() / 500) % 2) {
    writeNumber(12, 2);
    writeNumber(12, 8);
    lc.setLed(1, 7, 6, 0);
  }
  else {
    writeDay();
  }
  writeMonth();
}

// prikaz podešavanja mjeseca
void showSetMonth() {
  if ((millis() / 500) % 2) {
    writeNumber(12, 17);
    writeNumber(12, 23);
    lc.setLed(3, 7, 5, 0);
  }
  else {
    writeMonth();
  }
  writeDay();
}

// prikaz podešavanja godine
void showSetYear() {
  if ((millis() / 500) % 2) {
    clearDisplay();
  }
  else {
    writeYear();
  }
}

// test modes

void showAllLedsOn() {
  for (int i = 0; i < 32; i++)
    lc.status[i] = 0xFF;
}

void showAllLedsOff() {
  for (int i = 0; i < 32; i++)
    lc.status[i] = 0x00;
}

void blinkAllLeds() {
  if ((millis() / 1000) % 2) showAllLedsOn();
  else showAllLedsOff();
}

void clearDisplay() {
  for (int d = 0; d < 4; d++)
    lc.clearDisplay(d);
}

void setDisplayIntensity(int i) {
  for (int d = 0; d < 4; d++)
    lc.setIntensity(d, i);
}

void showSunce() {
  for (int i = 0; i < 8; i++) {
    lc.setRow(0,i, sunce[i]);
  }
}

void showSetBrightness() {
  clearDisplay();
  showSunce();
  
  writeNumber(ledBrightness/10, 16);
  writeNumber(ledBrightness%10, 22);
}


void setBrigrtness(int newBrightness) {
  if (newBrightness >= INTENSITY_MAX) {
    ledBrightness = INTENSITY_MAX;
  }
  else if (newBrightness <= INTENSITY_MIN) {
    ledBrightness = INTENSITY_MIN;
  }
  else {
    ledBrightness = newBrightness;
  }

  lc.setIntensity(0, ledBrightness);
  lc.setIntensity(1, ledBrightness);
  lc.setIntensity(2, ledBrightness);
  lc.setIntensity(3, ledBrightness);

  Serial.print("Brightness set to: [");
  Serial.print(ledBrightness);
  Serial.println("]");
}

void changeProgramMode(int modeNumber) {
  int modeBase = modeNumber/100;
  if(modeBase > 0 && modeBase <= NO_OF_MODES && modeNumber <= modeMaxes[modeBase-1]) {
    clearDisplay();
    programMode = modeNumber;
    Serial.print("Program mode set to: [");
    Serial.print(programMode, DEC);
    Serial.println("]");
  }
  else if(modeBase > 0 && modeBase <= NO_OF_MODES && modeNumber > modeMaxes[modeBase-1]) {
    clearDisplay();
    programMode = modeBase*100+1;
    Serial.print("Program mode set to: [");
    Serial.print(programMode, DEC);
    Serial.println("]");
  }
  else {
    Serial.print("Program mode does not exists: [");
    Serial.print(modeNumber, DEC);
    Serial.println("]");
  }
}

void serialEvent() {
  String inString = "";
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    if (isDigit(inChar)) {
      // convert the incoming byte to a char and add it to the string:
      inString += (char)inChar;
    }
  }
  changeProgramMode(inString.toInt());
}

void showError() {
  Serial.println("Error");
  writeString("error");
}

void writeString(char * mssg) {
  for (int i = 0; i < strlen(mssg); i++) {
    writeChar(mssg[i], i * 6);
  }
}

void writeChar(uint8_t chr, int inCol) {
  int _segNo;
  int _colNo;
  int _currCol;

  if (inCol < 0 || inCol > 31) {
    return;
  }

  _currCol = inCol;

  for (int i = 0; i < 5; i++) {
    if (_currCol > 31) return;
    lc.setColumn(_currCol / 8, _currCol % 8, pgm_read_word_near(font_5x7_asci + (chr - 32) * 5 + i));
    _currCol++;
  }
}

void writeMinimum() {
  int x_offset = 1;
  int y_offset = 3;
  for (int i = 0; i < 29; i++) {
    lc.setColumn((i + x_offset) / 8, (i + x_offset) % 8, pgm_read_word_near(minimum_mssg + i) << y_offset);
  }
}

// bug 2023-03-15 - uvijek se ažuriraju svi elemeti vrmena, pa tako i sekunde sa RTC.write(tm) - zamijeniti sa RTC.setSecond, minute,...
// ako se brzo više puta promjeni godina, svaki puta se ažuriraju i sat, minuta, sekunda, 
// na taj način sekunda može zaostati nakon podešavanja, jer se svaki puta podesi na istu vrijednost, 
// a promjena se desi unutar jedne sekunde
void changeModeValue(int programMode, int valueDelta) {
  int updateRtc = 1;
  int updateOk = 0;
  
  if(programMode == MODE_SET_HOUR) {
    tm.Hour = (24+tm.Hour+valueDelta)%24;
    updateOk = RTC.setHour(tm.Hour);
  }
  else if(programMode == MODE_SET_MINUTE) {
    tm.Second=0;
    tm.Minute=(60+tm.Minute+valueDelta)%60;
    updateOk = RTC.setSecond(tm.Second);
    updateOk = RTC.setMinute(tm.Minute);
  }
  else if(programMode == MODE_SET_SECOND) {
    tm.Second=(60+tm.Second+valueDelta)%60;
    updateOk = RTC.setSecond(tm.Second);
  }
  else if(programMode == MODE_SET_YEAR) {
    tm.Year=(tm.Year+valueDelta);
    updateOk = RTC.setYear(tm.Year);
  }
  else if(programMode == MODE_SET_MONTH) {
    tm.Month=(tm.Month+valueDelta)%12 == 0 ? 12 : (12+tm.Month+valueDelta)%12;
    updateOk = RTC.setMonth(tm.Month);
  }
  else if(programMode == MODE_SET_DAY) {
    tm.Day=(tm.Day+valueDelta)%31 == 0 ? 31 : (31+tm.Day+valueDelta)%31;
    Serial.print("Day = ");
    Serial.println(tm.Day);
    updateOk = RTC.setDay(tm.Day);
    //updateOk = RTC.write(tm);
    Serial.print("updateOk = ");
    Serial.println(updateOk);
  }
  else if(programMode == MODE_BRIGHTNESS) {
    setBrigrtness(ledBrightness+valueDelta);
    updateRtc = 0;
  }
  else
    return;

  // bugfix 2023-03-14 update RTC-a se radio i na promjeni svjetline, ubačen updateRtc
  if (updateRtc==1) {
    //if (RTC.write(tm)) { -- ovo ponovo ažurira datetime nepotrebno
    if(updateOk) {
      Serial.println("Time set to NEW time!");
      Serial.print("Ok, Time = ");
      print2digits(tm.Hour);
      Serial.write(':');
      print2digits(tm.Minute);
      Serial.write(':');
      print2digits(tm.Second);
      Serial.print(", Date (D/M/Y) = ");
      Serial.print(tm.Day);
      Serial.write('/');
      Serial.print(tm.Month);
      Serial.write('/');
      Serial.print(tmYearToCalendar(tm.Year));
      Serial.println();
    }
    else {
      showError();
      lc.flush();
      while (1);
    }
  }
}

void increaseModeValue(int programMode) {
  changeModeValue(programMode, 1);
}

void decreaseModeValue(int programMode) {
  changeModeValue(programMode, -1);
}

/* ********** BUTTONS *************** */

void checkButtons() {
  static long lastPressedTime[3] = {0L, 0L, 0L};
  static long longPress[3] = {0,0,0};
  static long shortPress[3] = {0,0,0};

  // imamo tri botuna, gledamo za svaki od njih da li je stisnut
  for (int i = 0; i < 3; i++) {

    // detektirali smo pritisak
    if(button[i].isPressed() && button[i].stateChanged()) {
      Serial.print("Button pressed:");
      Serial.println(i);
      lastPressedTime[i]=millis();
    }
    // detektirali smo promjenu stanja, otpuštanje gumba, promjena stanja gumba, ali nije trenutno pritisnut
    else if(button[i].stateChanged()) {
      Serial.print("Button released ");
      if(millis() - lastPressedTime[i] < BUTTON_LONG_PRESS_MIN) {
        Serial.print("[short press]:");
        Serial.println(i);
        shortPress[i]++;
        // Ako smo u prikazu, mijenjamo prikaz
        if(i==0 && programMode%100==0) changeProgramMode(programMode%(NO_OF_MODES*100)+100);
        else if(i==0 && programMode%100!=0) changeProgramMode(programMode+1);
        else if(i==1 && (programMode%100>0 || programMode == MODE_BRIGHTNESS)) increaseModeValue(programMode);
        else if(i==2 && (programMode%100>0 || programMode == MODE_BRIGHTNESS)) decreaseModeValue(programMode);
      } else {
        Serial.print("[long press]:");
        Serial.println(i);
        longPress[i]=0;
      }
    }
    // ako smo detektirali dugi pritisak gumba - ulazimo u mod ili izlazimo iz moda podešavanja
    else if(button[i].wasPressed() && !button[i].stateChanged() 
            && longPress[i]!=1 && millis()-lastPressedTime[i] > BUTTON_LONG_PRESS_MIN) {
      longPress[i] = 1;
      Serial.print("Long pres detected on button:");
      Serial.println(i);
      // ako je prvi botun, a u programu prikaza smo (ne podešavanje), uđi u potprogram za podešavanje
      if(i==0 && programMode%100==0) changeProgramMode(programMode+1);
      // ako smo u programu podešavanja, vrati se na program prikaza
      else if (i==0 && programMode%100!=0) changeProgramMode((programMode/100)*100);
    }
  }
}
