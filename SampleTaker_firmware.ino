#define GS_NO_ACCEL
#include <GyverStepper2.h>

//Настройка шаговых двигателей:

//Настройки двигателя насоса:
#define PUMP_SPR 100 //Шаги на единицу объёма
#define PUMP_STEP_PIN 0 //Пин шага драйвера
#define PUMP_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> pump_stepper(PUMP_SPR, PUMP_STEP_PIN, PUMP_DIR_PIN); //Шаговый двигатель насоса

//Настройки двигателя фильтра
#define FILTER_SPR 100 //Шаги на единицу перемещения
#define FILTER_STEP_PIN 0 //Пин шага драйвера
#define FILTER_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> filter_stepper(FILTER_SPR, FILTER_STEP_PIN, FILTER_DIR_PIN); //Шаговый двигатель фильтра

//Настройки двигателя карусели
#define CAROUSEL_SPR 100 //Шаги на оборот
#define CAROUSEL_STEP_PIN 0 //Пин шага драйвера
#define CAROUSEL_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> carousel_stepper(CAROUSEL_SPR, CAROUSEL_STEP_PIN, CAROUSEL_DIR_PIN); //Шаговый двигатель карусели

//Задаваемые параметры:
#define SAMPLE_VOLUME 10 //Объём пробы
#define FLUSH_VOLUME 10 //Объём промывки
#define PAUSE_TIME 10 //Пауза между пробами в минутах
#define SAMPLE_NUMBER 40 //Количество проб

//Параметры прибора:
//Параметры насоса:
#define BACK_VOLUME 1 //Объём обратного тока
//Параметры карусели
#define MAX_SAMPLE_NUMBER 40 //Максимальное количество проб
#define CIRCLES_NUMBER 2 //Количество кругов на карусели
#define PLACES_ON_CIRCLES {30, 10} //Перечисление количества пробирок на кругах (от внешнего к внутреннему)
//Параметры фильтра
#define FLUSH_POSITION 0 //Позиция промывки
#define TUBES_POSITIONS {10, 5} //Перечисление позиций пробирок на кругах (расстояние от нуля до фильтра, от внешнего круга к внутреннему)

//Реализация:
int filter_zero_abs = 0; //Позиция нуля для фильтра
int carousel_zero_abs = 0;
int carousel[CIRCLES_NUMBER] = PLACES_ON_CIRCLES; //Количество пробирок на кругах
int tubes[CIRCLES_NUMBER] = TUBES_POSITIONS; //Позиции фильтра над пробирками
int current_tube = 0; //номер текуще1 пробирки, нумерация с 0, до последней

void homing(){//Парковка на нулевые позиции
  //TODO
  filter_zero_abs = filter_stepper.getCurrent();
  carousel_zero_abs = carousel_stepper.getCurrent();
}

void move_to_flush(){//Перемещение на позицию промывки
  filter_stepper.setTarget(filter_zero_abs + FLUSH_POSITION);
  do{
    filter_stepper.tick();
  }while(!filter_stepper.ready());
}

void flush_filter(){//Промывка фильтра
  //Промывка:
  pump_stepper.setTarget(FLUSH_VOLUME*PUMP_SPR, RELATIVE);
  do{
    pump_stepper.tick();
  }while(!pump_stepper.ready());

  //Обратный ток:  
  pump_stepper.setTarget(-BACK_VOLUME*PUMP_SPR, RELATIVE);
  do{
    pump_stepper.tick();
  }while(!pump_stepper.ready());
  
}

int count_tube_position(int tube_number){//Посчитать позицию фильтра для пробирки с номером tube_number
  int n=current_tube;
  int i=0;
  for(;i<CIRCLES_NUMBER;i++){
    if(n>=carousel[i]){
      n-=carousel[i];
    }
  }
  return tubes[i];
}

double count_tube_angle(int tube_number){//Посчитать угл карусели для пробирки с номером tube_number
  int n=current_tube;
  int i=0;
  for(;i<CIRCLES_NUMBER;i++){
    if(n>=carousel[i]){
      n-=carousel[i];
    }
  }
  return n*360.0/carousel[i];
}

void move_to_tube(int tube_number){//Переместиться к пробирке
  int pos = count_tube_position(tube_number);
  double angle = count_tube_angle(tube_number);
  //Перемещение фильтра
  filter_stepper.setTarget(filter_zero_abs + pos);
  do{
    filter_stepper.tick();
  }while(!filter_stepper.ready());
  //Перемещение карусели
  carousel_stepper.setTargetDeg(carousel_zero_abs + angle);
  do{
    carousel_stepper.tick();
  }while(!carousel_stepper.ready());
}

void take_sample(){//Взять пробу
  //Отбор пробы, учитывая обратный ток при промывке:
  pump_stepper.setTarget( (SAMPLE_VOLUME + BACK_VOLUME)*PUMP_SPR, RELATIVE);
  do{
    pump_stepper.tick();
  }while(!pump_stepper.ready());

  /*
  //Обратный ток:  
  pump_stepper.setTarget(-BACK_VOLUME*PUMP_SPR, RELATIVE);
  do{
    pump_stepper.tick();
  }while(!pump_stepper.ready());
  */
}

void setup(){
  homing();//Установка нулевого положения
}

void loop(){
  if(current_tube>=SAMPLE_NUMBER) exit(0); //Остановка при достижении нужного количества проб
  
  move_to_flush(); //Переместить фильтр на позицию промывки
  flush_filter(); //Промывка, прогон в обратном направлении, чтобы не загрязнить образцы
  move_to_tube(current_tube); //Переместить фильтр к пробирке
  take_sample(); //Взять пробу
  current_tube++; //Увеличить номер пробирки
  delay(PAUSE_TIME*1000*60); //Подождать PAUSE_TIME минут
}

