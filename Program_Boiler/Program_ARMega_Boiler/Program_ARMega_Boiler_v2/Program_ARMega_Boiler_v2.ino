#include <LiquidCrystal_I2C.h>
#include <max6675.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//--> Termocouple(Suhu) Pin
#define pin_S0 4
#define pin_CS 5
#define pin_SCK 6

MAX6675 sensor_suhu(pin_SCK, pin_CS, pin_S0);

//--> Sensor Tinggi Air Pin
#define batas_atas 2
#define batas_bawah 3

//--> Pressure(Tekanan) Pin
#define pin_ADC A0

//--> Pin Lainnya
#define pin_HEATER 8
#define pin_PUMP 9
#define pin_SL02_OUT 10
#define pin_SL01_IN 11
#define pin_pematik 12

//--> Kelompok Data
bool data_select[5] = { false, false, false, false, false };
float data_suhu = 0;
float data_tekanan = 0;
int data_terima;
int counter = 0;
unsigned long time = 0;

void setup() {

  Serial.begin(115200);

  //--> INPUT
  pinMode(pin_ADC, INPUT);
  pinMode(batas_atas, INPUT_PULLUP);
  pinMode(batas_bawah, INPUT_PULLUP);

  //--> OUTPUT
  pinMode(pin_HEATER, OUTPUT);
  pinMode(pin_PUMP, OUTPUT);
  pinMode(pin_SL01_IN, OUTPUT);
  pinMode(pin_SL02_OUT, OUTPUT);
  pinMode(pin_pematik, OUTPUT);

  //-- Setup LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");

  //--> Checking...
  while (data_terima != 56) {
    kirim_data(1, data_suhu);
    delay(1000);
    if (Serial.available() > 0) {
      data_terima = Serial.readString().toInt();
    }
    if (data_terima == 56) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print("OK");
      break;
    }
  }
  delay(2000);
  lcd.clear();
}

void loop() {

  //--> Kirim Data
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Mohon Tunggu...");
    delay(1000);
    kirim_data(1, data_suhu);
    if (Serial.available() > 0) {
      data_terima = Serial.readString().toInt();
    }
    if (data_terima == 56) {
      if (counter == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Status:");
        lcd.setCursor(0, 1);
        lcd.print("Ready");
        delay(2000);
        lcd.clear();
      }
      if (counter != 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Status:");
        lcd.setCursor(0, 1);
        lcd.print("Isi Ulang");
        delay(2000);
        lcd.clear();
      }
      break;
    }
  }

  //--> Pompa & Selonoid Air Masuk
  while (true) {
    if (digitalRead(batas_atas) == LOW && digitalRead(batas_bawah) == LOW) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status:");
      lcd.setCursor(0, 1);
      lcd.print("Air Penuh");
      data_select[0] = true;
      data_select[2] = true;
      digitalWrite(pin_PUMP, LOW);
      delay(3000);
      digitalWrite(pin_SL01_IN, LOW);
      delay(3000);
      lcd.clear();
      break;
    }
    lcd.setCursor(0, 0);
    lcd.print("Status:");
    lcd.setCursor(0, 1);
    lcd.print("Pengisian Air");
    if (data_select[0] == false) {
      digitalWrite(pin_SL01_IN, HIGH);
      delay(1000);
      digitalWrite(pin_PUMP, HIGH);
    }
  }

  //--> Kirim Data & Baca Suhu & Baca Tekanan & Kendali Output Selonoid Gas
  while (true) {
    if (millis() - time > 500) {
      baca_suhu_dan_tekanan();
      time = millis();
    }
    if (data_select[3] == false) {
      while (true) {
        kirim_data(44, data_suhu);
        delay(1000);
        if (Serial.available() > 0) {
          data_terima = Serial.readString().toInt();
        }
        if (data_terima == 56) {
          data_select[3] = true;
          break;
        }
      }
    }
    if (data_select[4] == false) {
      while (true) {
        kirim_data(23, data_suhu);
        delay(1000);
        if (Serial.available() > 0) {
          data_terima = Serial.readString().toInt();
        }
        if (data_terima == 56) {
          data_select[4] = true;
          break;
        }
      }
    }
    kirim_data(50, data_suhu);
    delay(1000);
    if (data_tekanan >= 4.00) data_select[1] = true;
    if (data_select[1] == true) digitalWrite(pin_SL02_OUT, HIGH);
    if (data_select[2] == true) digitalWrite(pin_pematik, HIGH);
    if (digitalRead(batas_atas) == HIGH && digitalRead(batas_bawah) == HIGH) {
      data_select[0] = false;
      data_select[1] = false;
      data_select[2] = false;
      data_select[3] = false;
      data_select[4] = false;
      digitalWrite(pin_SL02_OUT, LOW);
      delay(1000);
      digitalWrite(pin_pematik, LOW);
      counter += 1;
      lcd.clear();
      break;
    }
  }
}

void kirim_data(int data_1, float data_2) {
  Serial.println('#' + String(data_1) + '@' + String(data_2) + '&');
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
