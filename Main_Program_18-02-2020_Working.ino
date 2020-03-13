#include <QuadDisplay.h>

// библиотека для работы с протоколом 1-Wire
#include <OneWire.h>
// библиотека для работы с датчиком DS18B20
#include <DallasTemperature.h>

#include <QuadDisplay.h>

// номер цифрового пина дисплея
#define DISPLAY_PIN  33

#define PIN_SPOON_RUN 2
#define PIN_SPOON_DIR 5
#define PIN_DRUM_RUN 3
#define PIN_DRUM_DIR 6
/*#define PIN_Z_RUN 4
#define PIN_Z_DIR 7  */
#define PIN_EN 8
#define ONE_WIRE_BUS 44
// сигнальный провод датчика
//const int ONE_WIRE_BUS = 44;

unsigned long now_time_1 = 0;
unsigned long next_time_1;
int timeout_1 = 100;

unsigned long now_time_2 = 0;
unsigned long next_time_2;
int timeout_2 = 100;

const int UP_STEPS = 4000;

const int STEPS_FOR_1_POS = 104;
const int STEPS_FOR_2_POS = 231;
const int STEPS_FOR_3_POS = 357;
const int STEPS_FOR_4_POS = 483;
const int STEPS_FOR_5_POS = 607;

const int BUTTON_LOAD_PIN = 25;
const int BUTTON_UNLOAD_PIN = 24;
const int BUTTON_PLUS_PIN = 26;
const int BUTTON_MINUS_PIN = 27;
const int BUTTON_TIME = 15;

const int ENDER_DOWN_PIN = 39;
const int ENDER_DRUM_PIN = 40;

const int OPTO_1_PIN = 34;
const int OPTO_2_PIN = 36;
const int OPTO_3_PIN = 35;
const int OPTO_4_PIN = 38;
const int OPTO_5_PIN = 37;

const int LUM_1_PIN = 29;
const int LUM_2_PIN = 30;// не работает
const int LUM_3_PIN = 32;
const int LUM_4_PIN = 31;// не работает
const int LUM_5_PIN = 28;

// создаём объект для работы с библиотекой OneWire
OneWire oneWire(ONE_WIRE_BUS);
// создадим объект для работы с библиотекой DallasTemperature
DallasTemperature sensor(&oneWire);

// Описание класса обработки сигналов кнопок
class Button {
  public:
    Button(int pin, int timeButton);  // описание конструктора
    boolean flagPress;    // признак кнопка сейчас нажата
    boolean flagClick;    // признак кнопка была нажата (клик)
    void scanState();     // метод проверки состояние сигнала

  private:
    byte  _buttonCount;    // счетчик подтверждений стабильного состояния
    byte _timeButton;      // время подтверждения состояния кнопки
    int _pin;             // номер вывода
};

class Temp {
  public:
    Button but_plus = Button(BUTTON_PLUS_PIN, BUTTON_TIME);
    Button but_minus = Button(BUTTON_MINUS_PIN, BUTTON_TIME);
    Temp();
    void run_temp();
  private:
    void check_buts();
    void change_temp();
    void en_pelte();
    void dis_pelte();
    int now_temp;
    int need_temp;
    bool but_plus_state;
    bool but_minus_state;
    bool temp_changed;
    bool disp_need;
    int now_time;
    int timeout;
    int next_time;
};

Temp temp = Temp();

class Drum {
  public:
    Drum();
    Button but_load = Button(BUTTON_LOAD_PIN, BUTTON_TIME);
    Button but_unload = Button(BUTTON_UNLOAD_PIN, BUTTON_TIME);
    void run_drum();
    void check_optopars();
    void move_to_default();
    void show_lumines();
    void spoon_down();
    
  private:
    bool check_buts();
    void load_unload(int _num_pos);
    int choose_num_pos();
    void spoon_up();
    void move_to_pos(int _num_pos);
    
    bool but_load_state;
    bool but_unload_state;
    int num_pos; // если 0, то не надо ничего делать, если от 1 до 5 - то это номер позиции куда надо крутиться
    int step_need;
    bool optopars[5];
    bool press_any;
    bool ender_down;
    bool ender_drum;
};

Drum drum = Drum();

void setup() {
  Serial.begin(9600);
  temp.run_temp();
  drum.spoon_down();
  drum.move_to_default();
  drum.check_optopars();
  drum.show_lumines();
}

void loop() {

  now_time_1 = millis();
  if (now_time_1 >= next_time_1) {
    next_time_1 = now_time_1 + timeout_1;
    drum.run_drum();
  }

  now_time_2 = millis();
  if (now_time_2 >= next_time_2) {
    next_time_2 = now_time_2 + timeout_1;
    temp.run_temp();
  }

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Drum: Begin
Drum::Drum() {
  pinMode(PIN_SPOON_RUN, OUTPUT);
  pinMode(PIN_SPOON_DIR, OUTPUT);

  pinMode(PIN_DRUM_RUN, OUTPUT);
  pinMode(PIN_DRUM_DIR, OUTPUT);

  pinMode(PIN_EN, OUTPUT);
  digitalWrite(PIN_EN, LOW);

   // кнопки
  for (int i = 24; i <= 27; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  // светодиоды
  for (int i = 28; i <= 32; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  //оптопары
  for (int i = 34; i <= 38; i++) {
    pinMode(i, INPUT);
  }

  but_load_state = false;
  but_unload_state = false;
  num_pos = -1; // если -1, то не надо ничего делать, если от 0 до 4 - то это номер позиции куда надо крутиться
  press_any = false;
  ender_down = false;
  ender_drum = false;
  step_need = 0;
}

void Drum::run_drum() {
  press_any = drum.check_buts();
  if (press_any) {
    drum.load_unload(drum.choose_num_pos());
    drum.check_optopars();
    drum.show_lumines();
  }
}

bool Drum::check_buts() {
  but_load.scanState();
  if (but_load.flagClick) {
    but_load.flagClick = false;
    but_load_state = true;
  } else but_load_state = false;
  
  but_unload.scanState();
  if (but_unload.flagClick) {
    but_unload.flagClick = false;
    but_unload_state = true;
  } else but_unload_state = false;

  if (but_unload_state || but_load_state) {
    Serial.print("Load but: ");
    Serial.print(but_load_state);
    Serial.print(" Unload but: ");
    Serial.println(but_unload_state);
    return true;
  }
  return false;
}

int Drum::choose_num_pos() {
  num_pos = -1;
  for(int i = 0; i < 5; i++) {
    if (but_unload_state) {
      if(optopars[i]) {
        num_pos = i+1;
        break;
      }
    } else if (but_load_state){
      if(!optopars[i]) {
        num_pos = i+1;
        break;
      }
    }
  }
  Serial.print("Choosed pos: ");
  Serial.println(num_pos);
  return num_pos;
}

void Drum::load_unload(int _num_pos) {
  if (_num_pos != -1) {
    move_to_pos(_num_pos);
    spoon_up();
    Serial.println("Waiting...");
    delay(2000);
    spoon_down();
    move_to_default();
  }
}

void Drum::spoon_down() {
  digitalWrite(PIN_SPOON_DIR, LOW); // экспериметально подбираем направление движения
  Serial.println("Down spooooon...");
  while (true) {
    ender_down = digitalRead(ENDER_DOWN_PIN);
    delayMicroseconds(10);
    Serial.println(ender_down);
    if (ender_down) {
      break;
    }
    digitalWrite(PIN_SPOON_RUN, 1);
    delayMicroseconds(5);
    digitalWrite(PIN_SPOON_RUN, 0);
    delayMicroseconds(2000);
  }
}

void Drum::spoon_up() {
  digitalWrite(PIN_SPOON_DIR, HIGH); // экспериметально подбираем направление движения
  Serial.println("Up spoooooon...");
  for (int i = 0; i <= UP_STEPS; i++) {
    digitalWrite(PIN_SPOON_RUN, 1);
    delayMicroseconds(5);
    digitalWrite(PIN_SPOON_RUN, 0);
    delayMicroseconds(2000);
  }
}

void Drum::move_to_pos(int _num_pose) {
  digitalWrite(PIN_DRUM_DIR, HIGH); // экспериметально подбираем направление движения
  Serial.println("Moving to pos...");
  switch (_num_pose) {
  case 1:
    step_need = STEPS_FOR_1_POS;
    break;
  case 2:
    step_need = STEPS_FOR_2_POS;
    break;
  case 3:
    step_need = STEPS_FOR_3_POS;
    break;
  case 4:
    step_need = STEPS_FOR_4_POS;
    break;
  case 5:
    step_need = STEPS_FOR_5_POS;
    break;
  }
  for (int i = 0; i <= step_need; i++) {
    digitalWrite(PIN_DRUM_RUN, 1);
    delayMicroseconds(5);
    digitalWrite(PIN_DRUM_RUN, 0);
    delayMicroseconds(5000);
  }
}

void Drum::move_to_default() {
  digitalWrite(PIN_DRUM_DIR, LOW); // экспериметально подбираем направление движения
  Serial.println("Moving to default");
  do {
    ender_drum = digitalRead(ENDER_DRUM_PIN);
    digitalWrite(PIN_DRUM_RUN, 1);
    delayMicroseconds(5);
    digitalWrite(PIN_DRUM_RUN, 0);
    delayMicroseconds(5000);
  } while (!ender_drum);
  
  digitalWrite(PIN_DRUM_DIR, HIGH); // экспериметально подбираем направление движения
  for (int i = 0; i < 30; i++) {
    digitalWrite(PIN_DRUM_RUN, 1);
    delayMicroseconds(5);
    digitalWrite(PIN_DRUM_RUN, 0);
    delayMicroseconds(5000);
  }
}

void Drum::check_optopars() {
  optopars[0] = !digitalRead(OPTO_1_PIN);
  optopars[1] = !digitalRead(OPTO_2_PIN);
  optopars[2] = !digitalRead(OPTO_3_PIN);
  optopars[3] = !digitalRead(OPTO_4_PIN);
  optopars[4] = !digitalRead(OPTO_5_PIN);
  Serial.print(" Optopars: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(optopars[i]);
    Serial.print(" ");
  }
  Serial.print("\n");
}

void Drum::show_lumines() {
  Serial.print(" Lumines: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(optopars[i]);
    Serial.print(" ");
  }
  Serial.print("\n");
  digitalWrite(LUM_1_PIN, optopars[0]);
  digitalWrite(LUM_2_PIN, optopars[1]);
  digitalWrite(LUM_3_PIN, optopars[2]);
  digitalWrite(LUM_4_PIN, optopars[3]);
  digitalWrite(LUM_5_PIN, optopars[4]);
}

// Drum: End
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ------------------------------------------------------------
// Temp: Begin
Temp::Temp() {
  // начинаем работу с датчиком
  sensor.begin();
  // устанавливаем разрешение датчика от 9 до 12 бит
  sensor.setResolution(12);
  
  now_temp = 0;
  need_temp = 10;

  but_plus_state = false;
  but_minus_state = false;
  temp_changed = false;
  disp_need = false;
  now_time = millis();
  next_time = 0;
  timeout = 3000;
}

void Temp::check_buts() {
  but_plus.scanState();
  if (but_plus.flagClick) {
    but_plus.flagClick = false;
    but_plus_state = true;
    need_temp++;
  } else but_plus_state = false;
  
  but_minus.scanState();
  if (but_minus.flagClick) {
    but_minus.flagClick = false;
    but_minus_state = true;
    need_temp--;
  } else but_minus_state = false;

  if (but_plus_state || but_minus_state) {
    Serial.print("Plus but: ");
    Serial.print(but_plus_state);
    Serial.print(" Minus but: ");
    Serial.println(but_minus_state);
    temp_changed = true;
  }
}

void Temp::en_pelte() {
  Serial.println("Pelte turned on");
}

void Temp::dis_pelte() {
  Serial.println("Pelte turned off");
}

void Temp::change_temp() {
  // отправляем запрос на измерение температуры
  sensor.requestTemperatures();
  // считываем данные из регистра датчика
  now_temp = sensor.getTempCByIndex(0);

  if (temp_changed) {
    disp_need = true;
    now_time = millis();
    next_time = now_time += timeout;
    temp_changed = false;
  }

  if (disp_need) {
    now_time = millis();
    if (now_time >= next_time) {
      disp_need = false;
    }
    displayTemperatureC(DISPLAY_PIN, need_temp);
  } else {
    // выводим температуру окружающей среды на дисплей
    displayTemperatureC(DISPLAY_PIN, now_temp);
  }

  if (now_temp > need_temp) {
    en_pelte();
  } else {
    dis_pelte();
  }
}

void Temp::run_temp() {
  check_buts();
  change_temp();
}

// Temp: End
// ------------------------------------------------------------

// ============================================================
// Button: Begin
void Button::scanState() {
  int k = digitalRead(_pin);
  if (flagPress == (!k)) {
    
  }
  else {
    flagPress = ! flagPress; // инверсия признака состояния

    if ( flagPress == true ) flagClick = true; // признак клика на нажатие
  }
}

// описание конструктора класса Button
Button::Button(int pin, int timeButton) {
  _pin = pin;
  _timeButton = timeButton;
  pinMode(_pin, INPUT_PULLUP);  // определяем вывод как вход

  flagPress = true;    // признак кнопка сейчас нажата / 2019-11-21:ПО УМОЛЧАНИЮ БЫЛО FALSE, МЫ СДЕЛАЛИ TRUE
  flagClick = false;    // признак кнопка была нажата (клик)
}
// Button: End
// ============================================================
