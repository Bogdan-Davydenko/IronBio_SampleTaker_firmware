#define GS_NO_ACCEL
#include <GyverStepper2.h>

//Настройка концевиков
#define ENDSTOP_FILTER_PIN 0
#define ENDSTOP_CAROUSEL_PIN 0
#define ENDSTOP_NEEDLE_PIN 0
#define TRIGGER_SIGNAL LOW

//Настройка шаговых двигателей:

//Настройки двигателя насоса:
#define PUMP_SPR 100 //Шаги на единицу объёма
#define PUMP_SPEED 10//Скорость в единицах обЪёма в секунду
#define PUMP_STEP_PIN 0 //Пин шага драйвера
#define PUMP_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> pump_stepper(PUMP_SPR, PUMP_STEP_PIN, PUMP_DIR_PIN); //Шаговый двигатель насоса

//Настройки двигателя фильтра
#define FILTER_SPR 100 //Шаги на единицу перемещения
#define FILTER_SPEED 10//Скорость в единицах расстояния в секунду
#define FILTER_STEP_PIN 0 //Пин шага драйвера
#define FILTER_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> filter_stepper(FILTER_SPR, FILTER_STEP_PIN, FILTER_DIR_PIN); //Шаговый двигатель фильтра

//Настройки двигателя карусели
#define CAROUSEL_SPR 100 //Шаги на оборот
#define CAROUSEL_SPEED 10//Скорость в градусах в секунду
#define CAROUSEL_STEP_PIN 0 //Пин шага драйвера
#define CAROUSEL_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> carousel_stepper(CAROUSEL_SPR, CAROUSEL_STEP_PIN, CAROUSEL_DIR_PIN); //Шаговый двигатель карусели

//Настройки двигателя иглы
#define NEEDLE_SPR 100 //Шаги на единицу перемещения
#define NEEDLE_SPEED 10//Скорость в единицах обЪёма в секунду
#define NEEDLE_STEP_PIN 0 //Пин шага драйвера
#define NEEDLE_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> needle_stepper(NEEDLE_SPR, NEEDLE_STEP_PIN, NEEDLE_DIR_PIN); //Шаговый двигатель фильтра

//Задаваемые параметры:
#define SAMPLE_VOLUME 10 //Объём пробы
#define FLUSH_VOLUME 10 //Объём промывки
#define PAUSE_TIME 10 //Пауза между пробами в минутах
#define SAMPLE_NUMBER 40 //Количество проб
#define SAMPLES_IN_GROUP 1 //Количество проб в одной группе

//Параметры прибора:
//Параметры карусели
#define MAX_SAMPLE_NUMBER 40 //Максимальное количество проб
#define CIRCLES_NUMBER 2 //Количество кругов на карусели
#define PLACES_ON_CIRCLES {30, 10} //Перечисление количества пробирок на кругах (от внешнего к внутреннему)
//Параметры фильтра
#define FLUSH_POSITION 50 //Позиция промывки
#define TUBES_POSITIONS {10, 5} //Перечисление позиций пробирок на кругах (расстояние от нуля до фильтра, от внешнего круга к внутреннему)
//Параметры иглы
#define NEEDLE_SAMPLE_POSITION 10 //На сколько опускается игла над пробиркой для взятия пробы в единицах расстояния (0 - максимально поднятая игла)
#define NEEDLE_FLUSH_POSITION 10 //На сколько опускается игла ёмкостью для промыввки при промывке в единицах расстояния (0 - максимально поднятая игла)

//Реализация:

int carousel[CIRCLES_NUMBER] = PLACES_ON_CIRCLES; //Количество пробирок на кругах
float tubes[CIRCLES_NUMBER] = TUBES_POSITIONS; //Позиции фильтра над пробирками
int current_tube = 0; //номер текущей пробирки, нумерация с 0, до последней

void move_angle(float angle){//Движение карусели до абсолютного угла
  carousel_stepper.setTargetDeg(angle);
  do{
    carousel_stepper.tick();
    int e = digitalRead(ENDSTOP_CAROUSEL_PIN);
    if(e == TRIGGER_SIGNAL){
      carousel_stepper.reset();
      carousel_stepper.setTargetDeg(0);
    }
  }while(!carousel_stepper.ready());
}

void move_radius(float pos){//Движеие фильтра по радиусу на позицию
  filter_stepper.setTarget(pos * FILTER_SPR);
  do{
    filter_stepper.tick();
    int e = digitalRead(ENDSTOP_FILTER_PIN);
    if(e == TRIGGER_SIGNAL){
      filter_stepper.reset();
      filter_stepper.setTarget(0);
    }
  }while(!filter_stepper.ready());
}

void move_needle(float height){//Движение иглы по вертикали
  needle_stepper.setTarget(height * NEEDLE_SPR);
  do{
    needle_stepper.tick();
    int e = digitalRead(ENDSTOP_NEEDLE_PIN);
    if(e == TRIGGER_SIGNAL){
      needle_stepper.reset();
      needle_stepper.setTarget(0);
    }
  }while(!needle_stepper.ready());
}

void move_pump(float volume){//Прокачка объёма
  pump_stepper.setTarget(volume * PUMP_SPR, RELATIVE);
  do{
    pump_stepper.tick();
  }while(!pump_stepper.ready());
}

void homing(){//Парковка на нулевые позиции
  move_angle(-360);
  move_radius(-10000);
  move_needle(-10000);
}

float count_tube_position(int tube_number){//Посчитать позицию фильтра для пробирки с номером tube_number
  int n=current_tube;
  int i=0;
  for(;i<CIRCLES_NUMBER;i++){
    if(n>=carousel[i]){
      n-=carousel[i];
    }
  }
  return tubes[i];
}

float count_tube_angle(int tube_number){//Посчитать угл карусели для пробирки с номером tube_number
  int n=current_tube;
  int i=0;
  for(;i<CIRCLES_NUMBER;i++){
    if(n>=carousel[i]){
      n-=carousel[i];
    }
  }
  return n*360.0/carousel[i];
}

double flush_pump(){//Промывка
  move_radius(FLUSH_POSITION);
  move_needle(NEEDLE_FLUSH_POSITION);
  move_pump(FLUSH_VOLUME);
  move_needle(0);
}

double take_sample(){
  float ang = count_tube_angle(current_tube);
  float pos = count_tube_position(current_tube);
  move_angle(ang);
  move_radius(pos);
  move_needle(NEEDLE_SAMPLE_POSITION);
  move_pump(SAMPLE_VOLUME);
  move_needle(0);
  current_tube++;
  if(current_tube >= SAMPLE_NUMBER){
    while(true){}
  }
}

void setup(){
  pinMode(ENDSTOP_FILTER_PIN, INPUT);
  pinMode(ENDSTOP_CAROUSEL_PIN, INPUT);
  pinMode(ENDSTOP_NEEDLE_PIN, INPUT);
  pump_stepper.setMaxSpeed(PUMP_SPEED * PUMP_SPR);
  pump_stepper.setSpeed(PUMP_SPEED * PUMP_SPR);
  carousel_stepper.setMaxSpeed(CAROUSEL_SPEED * CAROUSEL_SPR);
  carousel_stepper.setSpeed(CAROUSEL_SPEED * CAROUSEL_SPR);
  filter_stepper.setMaxSpeed(FILTER_SPEED * FILTER_SPR);
  filter_stepper.setSpeed(FILTER_SPEED * FILTER_SPR);
  needle_stepper.setMaxSpeed(NEEDLE_SPEED * NEEDLE_SPR);
  needle_stepper.setSpeed(NEEDLE_SPEED * NEEDLE_SPR);
  homing();
}

void loop(){
  flush_pump();
  for(int i=0; i<SAMPLES_IN_GROUP; i++){
    take_sample();
  }
  delay(PAUSE_TIME*1000*60);
}

