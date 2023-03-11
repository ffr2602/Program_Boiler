#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <max6675.h>

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//--> Set Pin
//----------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//--> Kelompok Data
//----------------------------------------------------------------------------------------------------------------

const int speed_set[2] = { 500, 1000 };
const int data_open[3] = { 0, 80, 100 };

const float setpoint_data_tekanan = 0.50;

bool data_select[2] = { false, false };

float data_suhu = 0;
float data_tekanan = 0;

int counter = 0;

unsigned long time = 0;
unsigned long time_1 = 0;
unsigned long data_count = 0;

//----------------------------------------------------------------------------------------------------------------

AccelStepper stepper(AccelStepper::DRIVER, pin_STEP, pin_DIR);
MAX6675 sensor_suhu(pin_SCK, pin_CS, pin_S0);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {

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

  //--> Set OFF All Relay
  digitalWrite(pin_PUMP, HIGH);
  digitalWrite(pin_SL01_IN, HIGH);
  digitalWrite(pin_SL02_OUT, HIGH);
  digitalWrite(pin_PEMATIK, HIGH);

  //--> Set Motor STEPPER
  stepper.setMaxSpeed(speed_set[1]);
  stepper.setSpeed(speed_set[0]);

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
    delay(3000);
    lcd.clear();
  } else if (counter > 0) {
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    lcd.print("Isi Ulang");
    delay(3000);
    lcd.clear();
  }

  //--> Pompa & Selonoid Air Masuk
  while (true) {
    //--> jika Air penuh
    if (digitalRead(batas_atas) == LOW && digitalRead(batas_bawah) == LOW) {
      data_select[0] = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print("Air Penuh");
      digitalWrite(pin_PUMP, HIGH);
      delay(3000);
      digitalWrite(pin_SL01_IN, HIGH);
      delay(3000);
      digitalWrite(pin_SL02_OUT, HIGH);
      delay(2000);
      lcd.clear();
      break;
    }
    //--> Jika Air belum penuh
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    lcd.print("Pengisian Air");
    if (data_select[0] == false) {
      digitalWrite(pin_SL02_OUT, LOW);
      delay(2000);
      digitalWrite(pin_SL01_IN, LOW);
      delay(2000);
      digitalWrite(pin_PUMP, LOW);
      delay(2000);
    }
  }

  //--> Kirim Data & Baca Suhu & Baca Tekanan & Kendali Output Selonoid Gas
  while (true) {
    //--> Baca suhu dan tekanan
    if (millis() - time >= 2000) {
      baca_suhu_dan_tekanan();
      time = millis();
    }
    //--> Menghidupkan Pematik untuk menyalakan Api
    while (data_select[1] == false) {
      digitalWrite(pin_PEMATIK, LOW);
      digitalWrite(pin_BUZZ, HIGH);
      motor_stepper_move(data_open[0]);
      if (digitalRead(pin_SENSOR_API == LOW)) {
        digitalWrite(pin_PEMATIK, HIGH);
        data_select[1] = true;
        break;
      }
    }
    //--> Mematikan Pematik ketika Api menyala
    if (digitalRead(pin_SENSOR_API) == HIGH) {
      data_select[1] = false;
    }
    //--> Mengatur OUTPUT GAS LPG untuk mengatur Besar Kecil Api
    if (digitalRead(pin_SENSOR_API) == LOW) {
      digitalWrite(pin_BUZZ, LOW);
      if (data_tekanan >= setpoint_data_tekanan) {
        motor_stepper_move(data_open[1]);
      } else if (data_tekanan < setpoint_data_tekanan) {
        motor_stepper_move(data_open[0]);
      }
    }
    //--> Mengatur OUTPUT GAS UAP Air
    if (data_tekanan >= setpoint_data_tekanan) {
      digitalWrite(pin_SL02_OUT, LOW);
    }
    //--> Mengatur Ulang Proses jika Air Habis
    if (digitalRead(batas_bawah == HIGH) && digitalRead(batas_atas) == HIGH) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print("Air Habis");
      digitalWrite(pin_PEMATIK, HIGH);
      delay(2000);
      digitalWrite(pin_SL02_OUT, HIGH);
      delay(2000);
      data_select[0] = false;
      while (true) {
        motor_stepper_move(data_open[2]);
        data_count += 1;
        if (data_count == 10000) {
          break;
        }
      }
      data_count = 0;
      counter += 1;
      lcd.clear();
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//--> Sub Program
//----------------------------------------------------------------------------------------------------------------

//--> Fungsi Penggerak Motor Stepper
void motor_stepper_move(float posisi) {
  int step_posisi = map(posisi, 100.00, 0.00, 0, 800) / 4;
  stepper.moveTo(step_posisi);
  stepper.setSpeed(speed_set[0]);
  stepper.runSpeedToPosition();
}

//--> Fungsi Baca Suhu dan Tekanan
void baca_suhu_dan_tekanan() {
  int ADC_in = analogRead(pin_ADC);
  float voltage = 5.00 * ADC_in / 1023.00;
  float pascal = (3.0 * ((float)voltage - 0.47)) * 1000000.0;
  data_tekanan = pascal / 10e5;
  data_suhu = sensor_suhu.readCelsius();
  lcd.setCursor(0, 0);
  lcd.print("Suhu: " + String(data_suhu) + char(223) + "C ");
  lcd.setCursor(0, 1);
  lcd.print("Press: " + String(data_tekanan) + " Bar ");
}
