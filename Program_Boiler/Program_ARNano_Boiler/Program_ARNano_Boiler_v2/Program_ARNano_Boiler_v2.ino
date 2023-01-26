#include <AccelStepper.h>
#include <Servo.h>
#include <SoftwareSerial.h>
SoftwareSerial data_serial(7, 6);  //--> RX, TX
Servo servo;

//--> Sensor Api pin
#define pin_sensor_api 5

//--> Motor STEPPER pin
#define pin_STEP 4
#define pin_DIR 3

AccelStepper stepper(AccelStepper::DRIVER, pin_STEP, pin_DIR);

//--> BUZZER pin
#define pin_BUZZ 2

//--> Kelompok Data
bool data_servo = false;
int data_terima[2];
int data_terima_servo;
int data_terima_motor;
int awal_pos = 0;
unsigned long time_1 = 0;

//--> PID
float Kp = 2;
float Ki = 0;
float Kd = 0.001;

//--> Variable
float cv, cv_1;
float err, err_1, err_2;
float tm = 0.01;

void setup() {

  data_serial.begin(115200);
  Serial.begin(57600);

  //--> OUTPUT & INPUT
  pinMode(pin_sensor_api, INPUT_PULLUP);
  pinMode(pin_BUZZ, OUTPUT);

  //--> Set Motor STEPPER
  stepper.setMaxSpeed(2000);
  stepper.setSpeed(1000);

  //--> Set Servo
  servo.attach(8);
  servo.write(awal_pos);
}

void loop() {

  //--> Baca Data
  while (data_serial.available() > 0) {
    String data_temp = data_serial.readString();
    if (data_temp.length() > 0) {
      int index_1 = data_temp.indexOf('#');
      int index_2 = data_temp.indexOf('@', index_1 + 1);
      int index_3 = data_temp.indexOf('&', index_2 + 1);
      data_terima[0] = data_temp.substring(index_1 + 1, index_2).toInt();
      data_terima[1] = data_temp.substring(index_2 + 1, index_3).toFloat();
      data_temp = "";
    }

    //--> Kirim Data Feedback
    if (data_terima[0] == 1) {
      data_terima_servo = 0;
      data_terima_motor = 0;
      data_serial.println("56");
      delay(1000);
    }

    //--> Simpan Data Untuk Servo Akif
    if (data_terima[0] == 23) {
      data_terima_servo = data_terima[0];
      data_serial.println("56");
      delay(1000);
    }

    //--> Simpan Data Untuk Motor Akif
    if (data_terima[0] == 44) {
      data_terima_motor = data_terima[0];
      data_serial.println("56");
      delay(1000);
    }

    Serial.println(data_terima[0]);
  }

  //--> Deteksi Api & Output Buzzer & Servo
  if (data_terima_servo == 23) {
    if (digitalRead(pin_sensor_api) == HIGH) {
      if (millis() - time_1 >= 500) {
        data_servo = !data_servo;
        time_1 = millis();
      }
      if (data_servo == false) servo.write(60);
      if (data_servo == true) servo.write(awal_pos);
      digitalWrite(pin_BUZZ, HIGH);
    } else {
      servo.write(awal_pos);
      digitalWrite(pin_BUZZ, LOW);
    }
  }
}

void motor_stepper_move(float posisi) {
  int step_posisi = map(posisi, 0.00, 100.00, 0, 1000);
  stepper.moveTo(step_posisi);
  stepper.setSpeed(1000);
  stepper.runSpeedToPosition();
}

void pid_data(float Kp, float Ki, float Kd, float tm, float set_point) {
  cv = cv_1 + (Kp + (Kd / tm)) * err + (-Kp + Ki - (2 * Kd / tm)) * err_1 + (Kd / tm) * err_2;
  cv_1 = cv;
  err_2 = err_1;
  err_1 = err;
}
