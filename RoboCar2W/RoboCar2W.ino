/*
 * Arduino Robot Car 2 Wheel with Motor Shield, UltraSonic distance sensor
 * Arduino робот машина, 2-х колесный, с платой управления моторами, ультразвуковой измеритель расстояния
 * Версия без сервомотора
 * Copyright 2015 Dmitry
 * original:
 * Copyright 2015 Yuriy Tim
 * Полное описание создания на сайте http://tim4dev.com
*/

#define VERSION "RoboCar2W ver.2015.12.12_2"

/*
 * Уровень отладки.
 * Чем больше, тем подробнее.
 * пока используются уровни:
 * Больше 1 - выдача отладочных сообщений
 * Больше 5 - реально моторы не включаются и дистанция не замеряется, а генерируется рандомно
 */
byte debug = 6;

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
#define SPEED_MAX 120
#define SPEED_MIN 60

/*
 * Виды поворотов
 */
const byte MOTOR_ROTATE_RIGHT = 0;  // вправо резкий разворот на месте (все левые колеса крутятся вперед, все правые - назад)
const byte MOTOR_TURN_RIGHT   = 1;  // вправо плавный поворот
const byte MOTOR_ROTATE_LEFT  = 2;  // влево резкий разворот на месте
const byte MOTOR_TURN_LEFT    = 3;  // влево плавный поворот
const byte MOTOR_TURN_BACK_RIGHT = 4; // поворот вправо задним ходом
const byte MOTOR_TURN_BACK_LEFT  = 5;

byte MOTOR_PREV_TURN_DIRECTION; // предыдущее выполненное направление движения
byte MOTOR_PREV_ROTATE_DIRECTION; // предыдущее выполненное направление движения
byte IS_TURN_ACTIVE;
/*
 * Задержки для езды, поворотов на месте и плавных поворотов.
 * Подбираются экспериментально.
 */
const int DELAY_RUN    = 500; //едем по 0,5 сек
const int DELAY_RUN_BACK = 2000;// назад 2 сек
const int DELAY_ROTATE = 250;// резкий поворот по 0,25 сек
const int DELAY_TURN   = 200;//плавно по 0,2 сек
const int DELAY_TURN_BACK = 500;//плавно назад по 0,5 сек

// в сантиметрах (distance threshold) Пороги расстояний до препятствия
// Если ближе, то резкий разворот на месте, иначе плавный доворот
const int DST_TRH_TURN = 30;
// Если ближе, то стоп и назад
const int DST_TRH_BACK = 15;

/* пины для подключения HC-SR04 Ultrasonic Module Distance Measuring
 * 13, 2 цифровые пины
 * 14, 15 аналоговые пины A0 и A1 соответственно
 */
#define SONIC_PIN_TRIG 14 //13
#define SONIC_PIN_ECHO 15 //2
// Detection distance: 2cm--450cm
const int SONIC_DISTANCE_MAX =100;
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
    delay(1000);
  }
}

void loop() {
  _printDebugString("\n*** new loop() start ***\n");
  if (debug >= 1) delay(1000);
  if (debug >= 3) delay(1000);
  if (debug >= 5) delay(1000);

  int distance = measureDistance();// замер расстояния

  // Препятствие далеко?
  if ( distance > DST_TRH_TURN )   {
    motorRunForward();//едем вперед
    IS_TURN_ACTIVE = false;
  } else if ( distance <= DST_TRH_BACK ) {
  // препятствие так близко что надо ехать назад ?
    stopAndBackward();
  } else {
    runForwardAndTurn();
  }
}
void runForwardAndTurn() {
  _printDebugString("run forward and turn");
  motorSetForward();
  motorRunMin();
  // После начала поворота сохраняем направлене поворота
  if (!IS_TURN_ACTIVE) {
    // направление поворота выбираем рандомно
    int rnd = random(1, 10);
    if (rnd > 5) {
      MOTOR_PREV_TURN_DIRECTION = MOTOR_TURN_LEFT;
    } else {
      MOTOR_PREV_TURN_DIRECTION = MOTOR_TURN_RIGHT;
    }
    IS_TURN_ACTIVE = true;
  }
  if (MOTOR_PREV_TURN_DIRECTION == MOTOR_TURN_LEFT) {
    motorTurnLeft();
  } else {
    motorTurnRight();
  }
}
// движение вперед по прямой
void motorRunForward()  {
  _printDebugString("Forward");
  // едем впред
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(FORWARD);

  // на максимальной скорости
  motorRunMax();
}

void stopAndBackward() {
    _printDebugString("ALARM! Distance too small!!!");
    motorSetStop();
    runBackAndRotate();
}
// стоп резко
void motorSetStop()  {
  _printDebugString("motor set stop");
  motorFrontLeftRun(RELEASE);
  motorFrontRightRun(RELEASE);
  // motorRearLeft.run(RELEASE);
  // motorRearRight.run(RELEASE);
  if (debug >= 3) delay(1000);
}
// движение назад с поворотом
void runBackAndRotate()  {
  _printDebugString("run backward and rotate");
  // Сначала на минимальной скорости назад
  runBackward();
  // Потом назад, поворачиваясь
  if (IS_TURN_ACTIVE) {
    _printDebugString("turn flag active!");
    //значит врезались поворачивая
    if (MOTOR_TURN_LEFT == MOTOR_PREV_TURN_DIRECTION){
      // если поворачивали влево,возвращаемся также
      motorRotateLeft();
    }else{
      motorRotateRight();
    }
  }else{
    _printDebugString("turn flag false!");
    if (MOTOR_ROTATE_RIGHT == MOTOR_PREV_ROTATE_DIRECTION) {
      motorRotateLeft();
    } else {
      motorRotateRight();
    }
  }
  // pause
  motorSetStop();
}
void runBackward(){
  _printDebugString("run backward");
  motorSetBackward();
  motorRunMin();
  delay(DELAY_RUN_BACK);
}
void motorSetBackward() {
  _printDebugString("set all motor Backward");
  motorFrontLeftRun(BACKWARD);
  motorFrontRightRun(BACKWARD);
  // motorRearLeft.run(BACKWARD);
  // motorRearRight.run(BACKWARD);
}
void motorSetForward() {
  _printDebugString("set all motor Forward");
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(FORWARD);
}
void motorRunMin()  {
  _printDebugString("Slow");

 // сбрасываем скорость в 5 шагов
  int diff = (SPEED_MAX - SPEED_MIN) / 5;
  _printDebugStringAndValue("diff = ", diff);

  int curSpeed = SPEED_CURRENT;
  _printDebugStringAndValue("current speed = ", curSpeed);

  for (int speed = curSpeed; speed >= SPEED_MIN; speed -= diff) {
    Serial.println(speed);
    motorSetSpeed(speed);
    delay(50);
  }

  curSpeed = SPEED_CURRENT;
  _printDebugStringAndValue("current speed = ", curSpeed);

  for (int speed = curSpeed; speed <= SPEED_MIN; speed += diff) {
    Serial.println(speed);
    motorSetSpeed(speed);
    delay(50);
  }
}
void _printDebugString(String  message) {
  if (debug >= 1) {
    Serial.println(message);
  }
}
void _printDebugStringAndValue(String message, int value) {
  if (debug >= 1) {
    Serial.print(message);
    Serial.println(value);
  }
}
/******************************************
  Functions
******************************************/

// инициализация моторов
void motorInit()  {
  _printDebugString("motor Init");
  IS_TURN_ACTIVE = false;
  motorSetStop();
}

// правый разворот на месте
void motorRotateRight()  {
  MOTOR_PREV_ROTATE_DIRECTION = MOTOR_ROTATE_RIGHT;
  _printDebugString("Rotate R");
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(BACKWARD);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(BACKWARD);
  delay(DELAY_ROTATE);
}
// левый разворот на месте
void motorRotateLeft()  {
  MOTOR_PREV_ROTATE_DIRECTION = MOTOR_ROTATE_LEFT;
  _printDebugString("Rotate L");
  motorFrontLeftRun(BACKWARD);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(BACKWARD);
  // motorRearRight.run(FORWARD);
  delay(DELAY_ROTATE);
}
// правый плавный поворот (при движении вперед)
void motorTurnRight()  {
  MOTOR_PREV_TURN_DIRECTION = MOTOR_TURN_RIGHT;
  _printDebugString("Turn R");
  motorFrontLeftRun(FORWARD);
  motorFrontRightRun(RELEASE);
  // motorRearLeft.run(FORWARD);
  // motorRearRight.run(RELEASE);
  delay(DELAY_TURN);
}
// левый плавный поворот (при движении назад)
void motorTurnBackLeft()  {
  MOTOR_PREV_TURN_DIRECTION = MOTOR_TURN_BACK_LEFT;
  _printDebugString("Turn Back L");
  motorFrontLeftRun(RELEASE);
  motorFrontRightRun(BACKWARD);
  // motorRearLeft.run(RELEASE);
  // motorRearRight.run(BACKWARD);
  delay(DELAY_TURN_BACK);
}
// правый плавный поворот (при движении назад)
void motorTurnBackRight()  {
  MOTOR_PREV_TURN_DIRECTION = MOTOR_TURN_BACK_RIGHT;
  _printDebugString("Turn Back R");
  motorFrontLeftRun(BACKWARD);
  motorFrontRightRun(RELEASE);
  // motorRearLeft.run(BACKWARD);
  // motorRearRight.run(RELEASE);
  delay(DELAY_TURN_BACK);
}

// левый плавный поворот (при движении вперед)
void motorTurnLeft()  {
  MOTOR_PREV_TURN_DIRECTION = MOTOR_TURN_LEFT;
  _printDebugString("Turn L");
  motorFrontLeftRun(RELEASE);
  motorFrontRightRun(FORWARD);
  // motorRearLeft.run(RELEASE);
  // motorRearRight.run(FORWARD);
  delay(DELAY_TURN);
}

void motorFrontLeftRun(int mode) {
  if (debug > 5) return;
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
  if (debug > 5) return;
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

// разгон плавно
void motorRunMax()  {
  _printDebugString("Run max");
  int speed;
  int diff = (SPEED_MAX - SPEED_MIN) / 5; // набираем скорость в 5 приема
  _printDebugStringAndValue("diff = ", diff);

  int curSpeed = SPEED_CURRENT;
  _printDebugStringAndValue("current speed = ", curSpeed);

  for ( speed = curSpeed; speed < SPEED_MAX; speed += diff) {
    Serial.println(speed);
    motorSetSpeed(speed);
    delay(20);
  }
  motorSetSpeed(SPEED_MAX); // устанавливаем максималку еще раз на всякий случай
  delay(DELAY_RUN);
}

// установить скорость 0--255
void motorSetSpeed(int speed) {
  // скорость мотора 0--255
  if (speed > SPEED_MAX) speed = SPEED_MAX;
  if (speed < 0) speed = 0;
  _printDebugStringAndValue("Motor set Speed = ", speed);

  motorFrontLeftSetSpeed(speed);
  motorFrontRightSetSpeed(speed);
  // motorRearLeft.setSpeed(speed);
  // motorRearRight.setSpeed(speed);
  // запоминаем текущую скорость
  SPEED_CURRENT = speed;
}
void motorFrontLeftSetSpeed(int speed) {
  if (debug >= 5) return;
  analogWrite(enA, speed);
}
void motorFrontRightSetSpeed(int speed) {
  if (debug >= 5) return;
  analogWrite(enB, speed);
}
// Возвращает расстояние до препятствия в сантиметрах
int measureDistance()  {
  if (debug > 15) return random(SONIC_DISTANCE_MIN, 50);
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

  _printDebugStringAndValue("distance = ", distance);
  if (distance < SONIC_DISTANCE_MIN )  // out of range
    return SONIC_DISTANCE_MIN;
  if (distance > SONIC_DISTANCE_MAX )  // out of range
    return SONIC_DISTANCE_MAX;
  return distance;
}