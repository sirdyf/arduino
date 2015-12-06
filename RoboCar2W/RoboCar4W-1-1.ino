/*
 * Arduino Robot Car 2 Wheel with Motor Shield, UltraSonic distance sensor
 * Arduino робот машина, 2-х колесный, с платой управления моторами, ультразвуковой измеритель расстояния
 * Версия без сервомотора
 * Copyright 2015 Dmitry
 * original:
 * Copyright 2015 Yuriy Tim
 * Полное описание создания на сайте http://tim4dev.com
*/

#define VERSION "RoboCar2W ver.2015.11.27"

/*
 * Уровень отладки.
 * Чем больше, тем подробнее.
 * пока используются уровни:
 * Больше 1 - выдача отладочных сообщений
 * Больше 5 - реально моторы не включаются и дистанция не замеряется, а генерируется рандомно
 */
byte debug = 2;

// первый двигатель
int enA = 10;
int in1 = 9;
int in2 = 8;

// второй двигатель
int enB = 5;
int in3 = 7;
int in4 = 6;

byte SPEED_CURRENT = 0; // текущая скорость моторов

// Constants that the user passes in to the motor calls
#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3
#define RELEASE 4

/*
 * Виды поворотов
 */
const byte MOTOR_ROTATE_RIGHT = 0;  // вправо резкий разворот на месте (все левые колеса крутятся вперед, все правые - назад)
const byte MOTOR_TURN_RIGHT   = 1;  // вправо плавный поворот
const byte MOTOR_ROTATE_LEFT  = 2;  // влево резкий разворот на месте
const byte MOTOR_TURN_LEFT    = 3;  // влево плавный поворот
const byte MOTOR_TURN_BACK_RIGHT = 4; // поворот вправо задним ходом
const byte MOTOR_TURN_BACK_LEFT  = 5;

byte MOTOR_PREV_DIRECTION; // предыдущее выполненное направление движения

/*
 * Задержки для езды, поворотов на месте и плавных поворотов.
 * Подбираются экспериментально.
 */
const int DELAY_RUN    = 20;
const int DELAY_RUN_BACK = 500;
const int DELAY_ROTATE = 500;
const int DELAY_TURN   = 500;
const int DELAY_TURN_BACK = 500;

// в сантиметрах (distance threshold) Пороги расстояний до препятствия
// Если ближе, то резкий разворот на месте, иначе плавный доворот
const int DST_TRH_TURN = 30;
// Если ближе, то стоп и назад
const int DST_TRH_BACK = 10;

/* пины для подключения HC-SR04 Ultrasonic Module Distance Measuring
 * 13, 2 цифровые пины
 * 14, 15 аналоговые пины A0 и A1 соответственно
 */
#define SONIC_PIN_TRIG 14 //13
#define SONIC_PIN_ECHO 15 //2
// Detection distance: 2cm--450cm
const int SONIC_DISTANCE_MAX = 450;
const int SONIC_DISTANCE_MIN = 2;

/******************************************
  Main program
******************************************/
void setup() {
  // инициализируем все пины для управления двигателями как outputs
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  Serial.begin(9600); // set up Serial library at 9600 bps
  if (debug > 1) Serial.println(VERSION);

  pinMode(SONIC_PIN_TRIG, OUTPUT);
  pinMode(SONIC_PIN_ECHO, INPUT);

  motorInit();
  if (debug  > 1) {
    delay(3000);
  }
}
void loop() {
  if (debug  > 1) Serial.println("\n*** new loop() start ***\n");
  // сравнить измеренные расстояния до препятствий
  // и определить направления движения
  int distance, ch;
  // замер расстояния
  distance = measureDistance();
  if (debug > 1) {
    Serial.print("distance = "); Serial.println(distance);
  }
  // препятствие так близко что надо ехать назад ?
  if ( distance <= DST_TRH_BACK ) {
    if (debug > 1) Serial.println("ALARM! Distance too small!!!");
    // стоп
    motorStop();
    if (debug  > 1) delay(1000);
    // ранее уже поворачивали задним ходом влево?
    if (MOTOR_TURN_BACK_LEFT == MOTOR_PREV_DIRECTION) {
      motorTurnBackRight();
    } else {
      motorTurnBackLeft();
    }
    if (debug  > 1) delay(1000);
    motorRunBack();
    return; // начать новый loop()
  }
  // определить направление поворота
  // прямо
  if ( distance > DST_TRH_TURN )   {
    motorRunSlow();
    motorRunForward();
  } else {
    motorStopSlow();
    // направление поворота выбираем рандомно
    int rnd = random(1, 10);
    if (rnd > 5) {
      if (debug  > 1) delay(500);
      motorTurnLeft();
    } else {
      if (debug  > 1) delay(500);
      motorTurnRight();
    }
  }
}

/******************************************
  Functions
******************************************/

// инициализация моторов
void motorInit()  {
  if (debug > 1) Serial.println("motor Init");
  if (debug > 5) return;
  // turn on motor
  motorSetSpeed(190); // скорость мотора 0--255, реально меньше 100 не работает
  motorStop();
}

// движение вперед по прямой
void motorRunForward()  {
  if (debug > 1) Serial.println("Forward");
  if (debug > 5) return;
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(FORWARD);
  delay(DELAY_RUN);
}

// движение назад по прямой
void motorRunBack()  {
  if (debug > 1) Serial.println("Backward");
  if (debug > 5) return;
  motorFrontLeftRun(BACKWARD);
  motorFrontRightRun(BACKWARD);
  // motorRearLeft.run(BACKWARD);
  // motorRearRight.run(BACKWARD);
  delay(DELAY_RUN_BACK);
}

// правый разворот на месте
void motorRotateRight()  {
  MOTOR_PREV_DIRECTION = MOTOR_ROTATE_RIGHT;
  if (debug > 1) Serial.println("Rotate R");
  if (debug > 5) return;
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(BACKWARD);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(BACKWARD);
  delay(DELAY_ROTATE);
}

// правый плавный поворот (при движении вперед)
void motorTurnRight()  {
  MOTOR_PREV_DIRECTION = MOTOR_TURN_RIGHT;
  if (debug > 1) Serial.println("Turn R");
  if (debug > 5) return;
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(RELEASE);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(RELEASE);
  delay(DELAY_TURN);
}

// правый плавный поворот (при движении назад)
void motorTurnBackRight()  {
  MOTOR_PREV_DIRECTION = MOTOR_TURN_BACK_RIGHT;
  if (debug > 1) Serial.println("Turn Back R");
  if (debug > 5) return;
  motorFrontLeftRun(BACKWARD);
  motorFrontRightRun(RELEASE);
  // motorRearLeft.run(BACKWARD);
  // motorRearRight.run(RELEASE);
  delay(DELAY_TURN_BACK);
}

// левый разворот на месте
void motorRotateLeft()  {
  MOTOR_PREV_DIRECTION = MOTOR_ROTATE_LEFT;
  if (debug > 1) Serial.println("Rotate L");
  if (debug > 5) return;
  motorFrontLeftRun(BACKWARD);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(BACKWARD);
  // motorRearRight.run(FORWARD);
  delay(DELAY_ROTATE);
}

// левый плавный поворот (при движении вперед)
void motorTurnLeft()  {
  MOTOR_PREV_DIRECTION = MOTOR_TURN_LEFT;
  if (debug > 1) Serial.println("Turn L");
  if (debug > 5) return;
  motorFrontLeftRun(RELEASE);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(RELEASE);
  // motorRearRight.run(FORWARD);
  delay(DELAY_TURN);
}

// левый плавный поворот (при движении назад)
void motorTurnBackLeft()  {
  MOTOR_PREV_DIRECTION = MOTOR_TURN_BACK_LEFT;
  if (debug > 1) Serial.println("Turn Back L");
  if (debug > 5) return;
  motorFrontLeftRun(RELEASE);
  motorFrontRightRun(BACKWARD);
  // motorRearLeft.run(RELEASE);
  // motorRearRight.run(BACKWARD);
  delay(DELAY_TURN_BACK);
}

// стоп резко
void motorStop()  {
  if (debug > 1) Serial.println("Stop");
  if (debug > 5) return;
  motorFrontLeftRun(RELEASE);
  motorFrontRightRun(RELEASE);
  // motorRearLeft.run(RELEASE);
  // motorRearRight.run(RELEASE);
}
void motorFrontLeftRun(int mode) {
  if (mode == RELEASE) {
    // выключаем двигатели
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  } else if (mode == FORWARD) {
    // запуск двигателя A
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (mode == BACKWARD) {
    // запуск двигателя A реверс
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    if (debug > 1) Serial.println("Unknown mode");
  }
}
void motorFrontRightRun(int mode) {
  if (mode == RELEASE) {
    // выключаем двигатели
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  } else if (mode == FORWARD) {
    // запуск двигателя A
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } else if (mode == BACKWARD) {
    // запуск двигателя A реверс
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  } else {
    if (debug > 1) Serial.println("Unknown mode");
  }
}

// стоп плавно
void motorStopSlow()  {
  if (debug > 1) Serial.println("Stop slow");
  if (debug > 5) return;
  int speed;
  int diff = SPEED_CURRENT / 5; // сбрасываем скорость в 5 приема
  for (speed = SPEED_CURRENT; speed <= 0; speed -= diff) {
    motorSetSpeed(speed);
    delay(150);
  }
  motorStop(); // тормозим еще раз на всякий случай
}

// разгон плавно
void motorRunSlow()  {
  if (debug) Serial.println("Stop slow");
  if (debug > 5) return;
  int speed;
  int diff = (255 - SPEED_CURRENT) / 5; // набираем скорость в 5 приема
  for (speed = SPEED_CURRENT; speed > 255; speed += diff) {
    motorSetSpeed(speed);
    delay(150);
  }
  motorSetSpeed(255); // устанавливаем максималку еще раз на всякий случай
}

// установить скорость 0--255
void motorSetSpeed(int speed) {
  // скорость мотора 0--255
  if (speed > 255)
    speed = 255;
  if (speed < 0)
    speed = 0;
  if (debug) {
    Serial.print("Motor set Speed = ");
    Serial.println(speed);
  }
  if (debug > 5) return;
  motorFrontLeftSetSpeed(speed);
  motorFrontRightSetSpeed(speed);
  // motorRearLeft.setSpeed(speed);
  // motorRearRight.setSpeed(speed);
  // запоминаем текущую скорость
  SPEED_CURRENT = speed;
}
void motorFrontLeftSetSpeed(int speed) {
  analogWrite(enA, speed);
}
void motorFrontRightSetSpeed(int speed) {
  analogWrite(enB, speed);
}
// Возвращает расстояние до препятствия в сантиметрах
int measureDistance()  {
  if (debug > 5) return random(SONIC_DISTANCE_MIN, 50);
  long duration;
  int  distance;
  /* Для запуска передатчика нужно подать на Trig сигнал, длительностью 10мкс.
   * Передатчик который посылает 8 коротких импульсов с частотой 40kHz.
   * Приемник получает отраженный сигнал и на входе Echo генерируется сигнал,
   * длительность которого равна времени прохождения звукового сигнала.
   */
  digitalWrite(SONIC_PIN_TRIG, LOW); // инициализация перед замером
  delayMicroseconds(5); // 3
  digitalWrite(SONIC_PIN_TRIG, HIGH);
  delayMicroseconds(12); // 10
  digitalWrite(SONIC_PIN_TRIG, LOW);

  duration = pulseIn(SONIC_PIN_ECHO, HIGH);
  // Скорость звука 340 м/с или 29 микросекунд на сантиметр.
  // Звук идет вперед и возвращается назад, таким образом время нужно делить на два
  distance = duration / 58; // = microseconds / 29 / 2

  if (distance < SONIC_DISTANCE_MIN )  // out of range
    return SONIC_DISTANCE_MIN;
  if (distance > SONIC_DISTANCE_MAX )  // out of range
    return SONIC_DISTANCE_MAX;

  return distance;
}
