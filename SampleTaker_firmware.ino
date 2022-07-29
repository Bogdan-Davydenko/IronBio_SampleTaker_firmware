#include <EEPROM.h>
#include <LiquidCrystal.h>
#define GS_NO_ACCEL
#include <GyverStepper2.h>

/* |---------------|
   | Оборудование: |
   |---------------| */
//Настройка дисплея
LiquidCrystal lcd(40, 42, 59, 64, 44, 66); //Пины дисплея
#define BTN_PIN A11 //Аналоговый пин кнопок

//Настройка концевиков
#define ENDSTOP_FILTER_PIN 0
#define ENDSTOP_CAROUSEL_PIN 0
#define ENDSTOP_NEEDLE_PIN 0
#define TRIGGER_SIGNAL LOW

//Настройка шаговых двигателей:
//Настройки двигателя насоса:
#define PUMP_SPR 100 //Шаги на единицу объёма
#define PUMP_SPEED 10 //Скорость в единицах обЪёма в секунду
#define PUMP_STEP_PIN 0 //Пин шага драйвера
#define PUMP_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> pump_stepper(PUMP_SPR, PUMP_STEP_PIN, PUMP_DIR_PIN); //Шаговый двигатель насоса

//Настройки двигателя фильтра
#define FILTER_SPR 100 //Шаги на единицу перемещения
#define FILTER_SPEED 10 //Скорость в единицах расстояния в секунду
#define FILTER_STEP_PIN 0 //Пин шага драйвера
#define FILTER_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> filter_stepper(FILTER_SPR, FILTER_STEP_PIN, FILTER_DIR_PIN); //Шаговый двигатель фильтра

//Настройки двигателя карусели
#define CAROUSEL_SPR 100 //Шаги на оборот
#define CAROUSEL_SPEED 10 //Скорость в градусах в секунду
#define CAROUSEL_STEP_PIN 0 //Пин шага драйвера
#define CAROUSEL_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> carousel_stepper(CAROUSEL_SPR, CAROUSEL_STEP_PIN, CAROUSEL_DIR_PIN); //Шаговый двигатель карусели

//Настройки двигателя иглы
#define NEEDLE_SPR 100 //Шаги на единицу перемещения
#define NEEDLE_SPEED 10 //Скорость в единицах обЪёма в секунду
#define NEEDLE_STEP_PIN 0 //Пин шага драйвера
#define NEEDLE_DIR_PIN 0 //Пин направления драйвера

GStepper2 <STEPPER2WIRE> needle_stepper(NEEDLE_SPR, NEEDLE_STEP_PIN, NEEDLE_DIR_PIN); //Шаговый двигатель фильтра

/* |--------------------|
   | Параметры прибора: |
   |--------------------| */
//Параметры карусели:
#define MAX_SAMPLE_NUMBER 40 //Максимальное количество проб
#define CIRCLES_NUMBER 2 //Количество кругов на карусели
#define PLACES_ON_CIRCLES {30, 10} //Перечисление количества пробирок на кругах (от внешнего к внутреннему)

//Параметры фильтра:
#define FLUSH_POSITION 50 //Позиция промывки
#define TUBES_POSITIONS {10, 5} //Перечисление позиций пробирок на кругах (расстояние от нуля до фильтра, от внешнего круга к внутреннему)

//Параметры иглы:
#define NEEDLE_SAMPLE_POSITION 10 //На сколько опускается игла над пробиркой для взятия пробы в единицах расстояния (0 - максимально поднятая игла)
#define NEEDLE_FLUSH_POSITION 10 //На сколько опускается игла ёмкостью для промыввки при промывке в единицах расстояния (0 - максимально поднятая игла)

//Параметры дисплея и интерфейса:
#define HELLO_MESSAGE0 "Hello!          " //Приветственное сообщение - строка 16 символа длинной, первая строка
#define HELLO_MESSAGE1 "                " //Приветственное сообщение - строка 16 символа длинной, вторая строка
#define PARAM_NAMES {"SAMPLE_VOL ", "FLUSH_VOL  ", "PAUSE_TIME ", "SAMPLE_NUM ", "GROUP_SIZE "}//Названия параметров - 5 строк по 11 символов - названия параметров
#define SAVE_STRING "SAVE_AS_DEFAULT" //Строка 16 символов - строка сохранения настроек 
//Задаваемые параметры, значения по умолчанию:
unsigned int SAMPLE_VOLUME  = 10; //Объём пробы
unsigned int FLUSH_VOLUME  = 100; //Объём промывки
unsigned int PAUSE_TIME = 10; //Пауза между пробами в минутах
unsigned int SAMPLE_NUMBER = 40; //Количество проб
unsigned int SAMPLES_IN_GROUP = 1; //Количество проб в одной группе
//Ограничения на параметры:
#define SAMPLE_VOLUME_MIN 1
#define SAMPLE_VOLUME_MAX 999
#define FLUSH_VOLUME_MIN 1
#define FLUSH_VOLUME_MAX 999
#define PAUSE_TIME_MIN 1
#define PAUSE_TIME_MAX 999
#define SAMPLE_NUMBER_MIN 1
#define SAMPLE_NUMBER_MAX 76
#define SAMPLES_IN_GROUP_MIN 1
#define SAMPLES_IN_GROUP_MAX 76

/* |-------------|
   | Реализация: |
   |-------------| */
//Движение:
void check_pause(); //Функция паузы реализуется в разделе дисплея
void end_screen(); //Реализуется в разделе дисплея

int carousel[CIRCLES_NUMBER] = PLACES_ON_CIRCLES; //Количество пробирок на кругах
float tubes[CIRCLES_NUMBER] = TUBES_POSITIONS; //Позиции фильтра над пробирками
int current_tube = 0; //номер текущей пробирки, нумерация с 0, до последней

void move_angle(float angle){ //Движение карусели до абсолютного угла
  carousel_stepper.setTargetDeg(angle);
  do{
    check_pause();
    carousel_stepper.tick();
    int e = digitalRead(ENDSTOP_CAROUSEL_PIN);
    if(e == TRIGGER_SIGNAL){
      carousel_stepper.reset();
      carousel_stepper.setTargetDeg(0);
    }
  }while(!carousel_stepper.ready());
}

void move_radius(float pos){ //Движеие фильтра по радиусу на позицию
  filter_stepper.setTarget(pos * FILTER_SPR);
  do{
    check_pause();
    filter_stepper.tick();
    int e = digitalRead(ENDSTOP_FILTER_PIN);
    if(e == TRIGGER_SIGNAL){
      filter_stepper.reset();
      filter_stepper.setTarget(0);
    }
  }while(!filter_stepper.ready());
}

void move_needle(float height){ //Движение иглы по вертикали
  needle_stepper.setTarget(height * NEEDLE_SPR);
  do{
    check_pause();
    needle_stepper.tick();
    int e = digitalRead(ENDSTOP_NEEDLE_PIN);
    if(e == TRIGGER_SIGNAL){
      needle_stepper.reset();
      needle_stepper.setTarget(0);
    }
  }while(!needle_stepper.ready());
}

void move_pump(float volume){ //Прокачка объёма
  pump_stepper.setTarget(volume * PUMP_SPR, RELATIVE);
  do{
    check_pause();
    pump_stepper.tick();
  }while(!pump_stepper.ready());
}

void homing(){ //Парковка на нулевые позиции
  //while(true) check_pause();
  move_needle(-10000);
  move_radius(-10000);
  move_angle(-360);
}

float count_tube_position(int tube_number){ //Посчитать позицию фильтра для пробирки с номером tube_number
  int n=current_tube;
  int i=0;
  for(;i<CIRCLES_NUMBER;i++){
    if(n>=carousel[i]){
      n-=carousel[i];
    }
  }
  return tubes[i];
}

float count_tube_angle(int tube_number){ //Посчитать угл карусели для пробирки с номером tube_number
  int n=current_tube;
  int i=0;
  for(;i<CIRCLES_NUMBER;i++){
    if(n>=carousel[i]){
      n-=carousel[i];
    }
  }
  return n*360.0/carousel[i];
}

double flush_pump(){ //Промывка
  move_radius(FLUSH_POSITION);
  move_needle(NEEDLE_FLUSH_POSITION);
  move_pump(FLUSH_VOLUME);
  move_needle(0);
}

double take_sample(){ //Взятие пробы
  float ang = count_tube_angle(current_tube);
  float pos = count_tube_position(current_tube);
  move_angle(ang);
  move_radius(pos);
  move_needle(NEEDLE_SAMPLE_POSITION);
  move_pump(SAMPLE_VOLUME);
  move_needle(0);
  current_tube++;
  if(current_tube >= SAMPLE_NUMBER){
    end_screen();
  }
}

//Память
void save(int n){
  EEPROM.write(n*10+1,highByte(SAMPLE_VOLUME));
  EEPROM.write(n*10+2,lowByte(SAMPLE_VOLUME));
  EEPROM.write(n*10+3,highByte(FLUSH_VOLUME));
  EEPROM.write(n*10+4,lowByte(FLUSH_VOLUME));
  EEPROM.write(n*10+5,highByte(PAUSE_TIME));
  EEPROM.write(n*10+6,lowByte(PAUSE_TIME));
  EEPROM.write(n*10+7,highByte(SAMPLE_NUMBER));
  EEPROM.write(n*10+8,lowByte(SAMPLE_NUMBER));
  EEPROM.write(n*10+9,highByte(SAMPLES_IN_GROUP));
  EEPROM.write(n*10+10,lowByte(SAMPLES_IN_GROUP));
}

void load(int n){
  SAMPLE_VOLUME = EEPROM.read(n*10+1)*256 + EEPROM.read(n*10+2);
  if(SAMPLE_VOLUME < SAMPLE_VOLUME_MIN || SAMPLE_VOLUME > SAMPLE_VOLUME_MAX){
    SAMPLE_VOLUME = SAMPLE_VOLUME_MIN;
  }
  FLUSH_VOLUME = EEPROM.read(n*10+3)*256 + EEPROM.read(n*10+4);
  if(FLUSH_VOLUME < FLUSH_VOLUME_MIN || FLUSH_VOLUME > FLUSH_VOLUME_MAX){
    FLUSH_VOLUME = FLUSH_VOLUME_MIN;
  }
  PAUSE_TIME = EEPROM.read(n*10+5)*256 + EEPROM.read(n*10+6);
  if(PAUSE_TIME < PAUSE_TIME_MIN || PAUSE_TIME > PAUSE_TIME_MAX){
    PAUSE_TIME = PAUSE_TIME_MIN;
  }
  SAMPLE_NUMBER = EEPROM.read(n*10+7)*256 + EEPROM.read(n*10+8);
  if(SAMPLE_NUMBER < SAMPLE_NUMBER_MIN || SAMPLE_NUMBER > SAMPLE_NUMBER_MAX){
    SAMPLE_NUMBER = SAMPLE_NUMBER_MIN;
  }
  SAMPLES_IN_GROUP = EEPROM.read(n*10+9)*256 + EEPROM.read(n*10+10);
  if(SAMPLES_IN_GROUP < SAMPLES_IN_GROUP_MIN || SAMPLES_IN_GROUP > SAMPLES_IN_GROUP_MAX){
    SAMPLES_IN_GROUP = SAMPLES_IN_GROUP_MIN;
  }
}

//Дисплей:
//Значения кнопок:
#define BTN_UP 1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_RIGHT 4
#define BTN_SELECT 5
#define BTN_NONE 0

int read_button() {
  int keyAnalog =  analogRead(BTN_PIN);
  if (keyAnalog < 90) {
    return BTN_RIGHT;
  } else if (keyAnalog < 200) {
    return BTN_UP;
  } else if (keyAnalog < 400) {
    return BTN_DOWN;
  } else if (keyAnalog < 600) {
    return BTN_LEFT;
  } else if (keyAnalog < 800) {
    return BTN_SELECT;
  }
  return BTN_NONE;
}

void lcd_clear(){
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}

int interface_state = 0;

void hello_screen(){ //Приветственный экран
  lcd_clear();
  lcd.setCursor(0,0);
  lcd.print(HELLO_MESSAGE0);
  lcd.setCursor(0,1);
  lcd.print(HELLO_MESSAGE1);
  delay(2000);
  interface_state = 1;
}

bool program_list_cursor = false;
int program_list_page = 0;
int current_program = 0;

void program_list(){ //Список программ
  do{
    lcd_clear();
    lcd.setCursor(0,0);
    if(program_list_cursor){
      lcd.print(" ");
    }else{
      lcd.print(">");
    }
    lcd.setCursor(1,0);
    lcd.print("PROGRAM_");
    lcd.setCursor(9,0);
    lcd.print(program_list_page+1);
    
    lcd.setCursor(0,1);
    if(program_list_cursor){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.setCursor(1,1);
    lcd.print("PROGRAM_");
    lcd.setCursor(9,1);
    lcd.print(program_list_page+2);

    int btn = read_button();
    while(btn == BTN_NONE){
      btn = read_button();
    }
    switch(btn){
      case BTN_UP:
        if(program_list_cursor){
          program_list_cursor = false;
        }else{
          if(program_list_page > 0){
            program_list_page--;
          }
        }
      break;
      case BTN_DOWN:
        if(!program_list_cursor){
          program_list_cursor = true;
        }else{
          if(program_list_page < 8){
            program_list_page++;
          }
        }
      break;
      case BTN_SELECT:

        if(program_list_cursor){
          current_program = program_list_page + 2;
        }else{
          current_program = program_list_page + 1;
        }
        
        load(current_program);
        interface_state = 2;
      break;
    }
    delay(200);
  }while(interface_state == 1);
}

bool program_menu_cursor = false;
bool program_menu_page = false;

void program_menu(){ //Меню программы
  do{
    lcd_clear();
    lcd.setCursor(0,0);
    if(program_menu_cursor){
      lcd.print(" ");
    }else{
      lcd.print(">");
    }
    lcd.setCursor(1,0);
    if(!program_menu_page){
      lcd.print("START");
    }else{
      lcd.print("SETTINGS");
    }
    lcd.setCursor(0,1);
    if(program_menu_cursor){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.setCursor(1,1);
    if(!program_menu_page){
      lcd.print("SETTINGS");
    }else{
      lcd.print("GO BACK");
    }

    int btn = read_button();
    while(btn == BTN_NONE){
      btn = read_button();
    }
    switch(btn){
      case BTN_UP:
        if(program_menu_cursor){
          program_menu_cursor = false;
        }else{
          if(program_menu_page){
            program_menu_page = false;
          }
        }
      break;
      case BTN_DOWN:
        if(!program_menu_cursor){
          program_menu_cursor = true;
        }else{
          if(!program_menu_page){
            program_menu_page = true;
          }
        }
      break;
      case BTN_SELECT:
        if(program_menu_page){
          if(program_menu_cursor){
            program_menu_cursor = false;
            program_menu_page = false;
            interface_state = 1;
          }else{
            interface_state = 3;
          }
        }else{
          if(program_menu_cursor){
            interface_state = 3;
          }else{
            interface_state = 4;
          }
        }
      break;
    }
    delay(200);
  }while(interface_state == 2);
}

bool settings_cursor = false;
int settings_page = 0;
String param_names[] = PARAM_NAMES;

void print_settings_item(int number, int line){
  lcd.setCursor(1,line);
  switch(number){
    case 0:
      lcd.print(param_names[0]);
      lcd.setCursor(12,line);
      lcd.print(":");
      lcd.setCursor(13,line);
      lcd.print(SAMPLE_VOLUME);
    break;
    case 1:
      lcd.print(param_names[1]);
      lcd.setCursor(12,line);
      lcd.print(":");
      lcd.setCursor(13,line);
      lcd.print(FLUSH_VOLUME);
    break;
    case 2:
      lcd.print(param_names[2]);
      lcd.setCursor(12,line);
      lcd.print(":");
      lcd.setCursor(13,line);
      lcd.print(PAUSE_TIME);
    break;
    case 3:
      lcd.print(param_names[3]);
      lcd.setCursor(12,line);
      lcd.print(":");
      lcd.setCursor(13,line);
      lcd.print(SAMPLE_NUMBER);
    break;
    case 4:
      lcd.print(param_names[4]);
      lcd.setCursor(12,line);
      lcd.print(":");
      lcd.setCursor(13,line);
      lcd.print(SAMPLES_IN_GROUP);
    break;
    case 5:
      lcd.print("SAVE");
    break;
    case 6:
      lcd.print("GO BACK");
    break;
  }
}

void increase_param(int number){
  switch(number){
    case 0:
      if(SAMPLE_VOLUME <= SAMPLE_VOLUME_MAX){
        SAMPLE_VOLUME++;
      }
      if(SAMPLE_VOLUME > SAMPLE_VOLUME_MAX){
        SAMPLE_VOLUME = SAMPLE_VOLUME_MIN;
      }
    break;
    case 1:
      if(FLUSH_VOLUME <= FLUSH_VOLUME_MAX){
        FLUSH_VOLUME++;
      }
      if(FLUSH_VOLUME > FLUSH_VOLUME_MAX){
        FLUSH_VOLUME = FLUSH_VOLUME_MIN;
      }
    break;
    case 2:
      if(PAUSE_TIME <= PAUSE_TIME_MAX){
        PAUSE_TIME++;
      }
      if(PAUSE_TIME > PAUSE_TIME_MAX){
        PAUSE_TIME = PAUSE_TIME_MIN;
      }
    break;
    case 3:
      if(SAMPLE_NUMBER <= SAMPLE_NUMBER_MAX){
        SAMPLE_NUMBER++;
      }
      if(SAMPLE_NUMBER > SAMPLE_NUMBER_MAX){
        SAMPLE_NUMBER = SAMPLE_NUMBER_MIN;
      }
    break;
    case 4:
      if(SAMPLES_IN_GROUP <= SAMPLE_NUMBER_MAX){
        SAMPLES_IN_GROUP++;
      }
      if(SAMPLES_IN_GROUP > SAMPLES_IN_GROUP_MAX){
        SAMPLES_IN_GROUP = SAMPLES_IN_GROUP_MIN;
      }
    break;
  }
}

void decrease_param(int number){
  switch(number){
    case 0:
      if(SAMPLE_VOLUME >= SAMPLE_VOLUME_MIN){
        SAMPLE_VOLUME--;
      }
      if(SAMPLE_VOLUME < SAMPLE_VOLUME_MIN){
        SAMPLE_VOLUME = SAMPLE_VOLUME_MAX;
      }
    break;
    case 1:
      if(FLUSH_VOLUME >= FLUSH_VOLUME_MIN){
        FLUSH_VOLUME--;
      }
      if(FLUSH_VOLUME < FLUSH_VOLUME_MIN){
        FLUSH_VOLUME = FLUSH_VOLUME_MAX;
      }
    break;
    case 2:
      if(PAUSE_TIME >= PAUSE_TIME_MIN){
        PAUSE_TIME--;
      }
      if(PAUSE_TIME < PAUSE_TIME_MIN){
        PAUSE_TIME = PAUSE_TIME_MAX;
      }
    break;
    case 3:
      if(SAMPLE_NUMBER >= SAMPLE_NUMBER_MIN){
        SAMPLE_NUMBER--;
      }
      if(SAMPLE_NUMBER < SAMPLE_NUMBER_MIN){
        SAMPLE_NUMBER = SAMPLE_NUMBER_MAX;
      }
    break;
    case 4:
      if(SAMPLES_IN_GROUP >= SAMPLE_NUMBER_MIN){
        SAMPLES_IN_GROUP--;
      }
      if(SAMPLES_IN_GROUP < SAMPLES_IN_GROUP_MIN){
        SAMPLES_IN_GROUP = SAMPLES_IN_GROUP_MAX;
      }
    break;
  }
}

void settings(){
  do{
    lcd_clear();
    lcd.setCursor(0,0);
    if(settings_cursor){
      lcd.print(" ");
    }else{
      lcd.print(">");
    }
    print_settings_item(settings_page,0);
    
    lcd.setCursor(0,1);
    if(settings_cursor){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.setCursor(1,1);
    print_settings_item(settings_page+1,1);

    int btn = read_button();
    while(btn == BTN_NONE){
      btn = read_button();
    }
    switch(btn){
      case BTN_UP:
        if(settings_cursor){
          settings_cursor = false;
        }else{
          if(settings_page > 0){
            settings_page--;
          }
        }
      break;
      case BTN_DOWN:
        if(!settings_cursor){
          settings_cursor = true;
        }else{
          if(settings_page < 5){
            settings_page++;
          }
        }
      break;
      case BTN_SELECT:
        if(settings_page == 4 && settings_cursor){
          save(current_program);
        }else if(settings_page == 5){
          if(settings_cursor){
            load(current_program);
            settings_page = 0;
            settings_cursor = false;
            interface_state = 2;
          }else{
            save(current_program);
          }
        }
      break;
      case BTN_RIGHT:
        if(settings_page < 4 || (settings_page == 4 && !settings_cursor)){
          if(settings_cursor){
            increase_param(settings_page + 1);
          }else{
            increase_param(settings_page);
          }
        }
      break;
      case BTN_LEFT:
        if(settings_page < 4 || (settings_page == 4 && !settings_cursor)){
          if(settings_cursor){
            decrease_param(settings_page + 1);
          }else{
            decrease_param(settings_page);
          }
        }
      break;
    }
    delay(200);
  }while(interface_state == 3);
}

void process_screen(){
  lcd_clear();
  lcd.setCursor(0,0);
  lcd.print("PROCESSING...");
  lcd.setCursor(0,1);
  lcd.print(">PAUSE");
}

bool pause_cursor = false;

void(* reset_func) (void) = 0;

void pause_menu(){
  do{
    lcd_clear();
    lcd.setCursor(0,0);
    if(pause_cursor){
      lcd.print(" ");
    }else{
      lcd.print(">");
    }
    lcd.setCursor(1,0);
    lcd.print("CONTINUE");
    
    lcd.setCursor(0,1);
    if(pause_cursor){
      lcd.print(">");
    }else{
      lcd.print(" ");
    }
    lcd.setCursor(1,1);
    lcd.print("RESET");

    int btn = read_button();
    while(btn == BTN_NONE){
      btn = read_button();
    }
    switch(btn){
      case BTN_UP:
        if(pause_cursor){
          pause_cursor = false;
        }
      break;
      case BTN_DOWN:
        if(!pause_cursor){
          pause_cursor = true;
        }
      break;
      case BTN_SELECT:
        if(pause_cursor){
          reset_func();
          //end_screen();
        }else{
          process_screen();
          interface_state = 4;
        }
      break;
    }
    delay(200);
  }while(interface_state == 5);
}

void check_pause(){
  if(read_button() == BTN_SELECT){
    interface_state = 5;
    lcd_clear();
    delay(1000);
    pause_menu();
  }
}

void end_screen(){
  interface_state = 6;
  lcd_clear();
  lcd.setCursor(0,0);
  lcd.print("COMPLETED!");
  lcd.setCursor(0,1);
  lcd.print(">RESET");
  delay(500);
  int btn = read_button();
  while(btn != BTN_SELECT){
    btn = read_button();
  }
  reset_func();
}

//Исполнение:
void setup(){
  pinMode(ENDSTOP_FILTER_PIN, INPUT);
  pinMode(ENDSTOP_CAROUSEL_PIN, INPUT);
  pinMode(ENDSTOP_NEEDLE_PIN, INPUT);

  program_list_cursor = false;
  program_list_page = 0;
  current_program = 0;
  pause_cursor = false;
  settings_cursor = false;
  settings_page = 0;
  program_menu_cursor = false;
  program_menu_page = false;
  
  lcd.begin(16, 2);
  interface_state = 0;
  
  do{
    switch(interface_state){
      case 0:
        hello_screen();
      break;
      case 1:
        program_list();
      break;
      case 2:
        program_menu();
      break;
      case 3:
        settings();
      break;
      default:
        lcd_clear();
    }
    delay(200);
  }while(interface_state <= 3);

  process_screen();
  
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

