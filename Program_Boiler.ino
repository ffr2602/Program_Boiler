#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <max6675.h>

//--> Motor STEPPER pin
#define pin_STEP 3
#define pin_DIR 2

//--> Termocouple(Suhu) Pin
#define pin_S0 4
#define pin_CS 5
#define pin_SCK 6

//--> Sensor Tinggi Air Pin
#define batas_atas 11
#define batas_bawah 10

//--> Pressure(Tekanan) Pin
#define pin_ADC A0

//--> BUZZER pin
#define pin_BUZZ 8

//--> Sensor Api pin
#define pin_SENSOR_API 9

//--> Pin Lainnya
#define pin_PUMP 36
#define pin_SL02_OUT 32
#define pin_SL01_IN 34
#define pin_PEMATIK 50


//--> Kelompok Data
bool data_select = false;
const int speed_set[2] = { 100, 1000 };
const int data_open[2] = { 0, 50 };
float data_suhu = 0;
float data_tekanan = 0;
int counter = 0;
unsigned long time = 0;

AccelStepper stepper(AccelStepper::DRIVER, pin_STEP, pin_DIR);
MAX6675 sensor_suhu(pin_SCK, pin_CS, pin_S0);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {

  Serial.begin(115200);

  //--> INPUT
  pinMode(pin_ADC, INPUT);
  pinMode(batas_atas, INPUT_PULLUP);
  pinMode(batas_bawah, INPUT_PULLUP);
  pinMode(pin_SENSOR_API, INPUT_PULLUP);

  //--> OUTPUT
  pinMode(pin_PUMP, OUTPUT);
  pinMode(pin_SL01_IN, OUTPUT);
  pinMode(pin_SL02_OUT, OUTPUT);
  pinMode(pin_PEMATIK, OUTPUT);
  pinMode(pin_BUZZ, OUTPUT);

  //--> Set Motor STEPPER
  stepper.setMaxSpeed(speed_set[1]);
  stepper.setSpeed(speed_set[0]);

  relay(1, 1, 1, 1);

  //-- Setup LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");
  delay(3000);
  lcd.clear();
}

void loop() {

  //--> Checking...
  if (counter == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    lcd.print("READY...");
    delay(2000);
    lcd.clear();
  } else if (counter > 0) {
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    lcd.print("Isi Ulang");
    delay(2000);
    lcd.clear();
  }

  //--> Pompa & Selonoid Air Masuk
  while (true) {
    if (digitalRead(batas_atas) == LOW && digitalRead(batas_bawah) == LOW) {
      data_select = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print("Air Penuh");
      relay(1, 0, 0, 1);
      delay(3000);
      relay(1, 1, 0, 1);
      delay(3000);
      relay(1, 1, 1, 1);
      lcd.clear();
      break;
    }
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    lcd.print("Pengisian Air");
    if (data_select == false) {
      relay(1, 0, 1, 1);
      delay(2000);
      relay(1, 0, 0, 1);
      delay(2000);
      relay(0, 0, 0, 1);
      delay(2000);
    }
    Serial.println("batas bawah: " + String(batas_bawah) + "|| batas atas: " + String(batas_atas));
  }

  //--> Kirim Data & Baca Suhu & Baca Tekanan & Kendali Output Selonoid Gas
  while (true) {
    if (millis() - time > 500) {
      baca_suhu_dan_tekanan();
      time = millis();
    }
    digitalWrite(pin_PEMATIK, LOW);
    if (digitalRead(pin_SENSOR_API) == HIGH) {
      digitalWrite(pin_BUZZ, HIGH);
      digitalWrite(pin_PEMATIK, LOW);
      motor_stepper_move(data_open[1]);
    } else if (digitalRead(pin_SENSOR_API) == LOW) {
      digitalWrite(pin_BUZZ, LOW);
      digitalWrite(pin_PEMATIK, HIGH);
      if (data_suhu >= 100) {
        motor_stepper_move(80);
      } else if (data_suhu < 100) {
        motor_stepper_move(data_open[0]);
      }
    }
    if (data_tekanan > 4.00) {
      digitalWrite(pin_SL02_OUT, LOW);
    } else if (digitalRead(batas_bawah == HIGH) && digitalRead(batas_atas) == HIGH) {
      motor_stepper_move(100);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print("Air Habis");
      digitalWrite(pin_PEMATIK, HIGH);
      digitalWrite(pin_SL02_OUT, HIGH);
      counter += 1;
      lcd.clear();
      break;
    }
  }
}

void motor_stepper_move(float posisi) {
  volatile int step_posisi = map(posisi, 100.00, 0.00, 0, 800) / 4;
  stepper.moveTo(step_posisi);
  stepper.setSpeed(speed_set[0]);
  stepper.runSpeedToPosition();
}

void baca_suhu_dan_tekanan() {
  lcd.setCursor(0, 0);
  lcd.print("Suhu: " + String(data_suhu) + char(223) + "C");
  lcd.setCursor(0, 1);
  lcd.print("Press: " + String(data_tekanan) + " Bar");
  int ADC_in = analogRead(pin_ADC);
  float voltage = 5.00 * ADC_in / 1023.00;
  float pascal = (3.0 * ((float)voltage - 0.47)) * 1000000.0;
  float data_temp = pascal / 10e5;
  data_tekanan = map(data_temp, 0.00, 12.00, 0.00, 4.00);
  data_suhu = sensor_suhu.readCelsius();
}

void relay(int a, int b, int c, int d) {
  digitalWrite(pin_PUMP, a);
  digitalWrite(pin_SL01_IN, b);
  digitalWrite(pin_SL02_OUT, c);
  digitalWrite(pin_PEMATIK, d);
}