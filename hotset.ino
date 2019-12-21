/*
 * 라이브러리 링크 : https://github.com/bogde/HX711
 * ** 앱과 연동되서 점수 보낼 때 
   고려해야하는 부분들 ------------------------------

1. 말씀드렸던 가슴압박정도가
너무 강하거나 약하면 감점
한두번정도는 괜찮지만 지속될시 감점

2. 15-18초안에 30회정도를해야되는데 그시간이 너무 짧거나 길면 감점

3. 가슴압박의 이완이 충분해야함
0으로 값이 돌아왔다 다시 수치가 올라와야함 그렇게안되면 감점
----------------------------------------------------------------------------
 */
/*
  * 
  *  영아 모드 0
  *  소아 모드 1
  *  성인 모드 2
  * 
  * 
  */

/* Define here include Libraries */
#include <Adafruit_NeoPixel.h>
#include <HX711.h>
#include <SoftwareSerial.h>

// #define TEST

/* Pin Map Configuration */
#define PIN_LOADCELL_1_DAT 2
#define PIN_LOADCELL_1_CLK 3
#define PIN_LOADCELL_2_DAT 4
#define PIN_LOADCELL_2_CLK 5
#define PIN_LOADCELL_3_DAT 6
#define PIN_LOADCELL_3_CLK 7
#define PIN_BUTTON_1 8 // use for start
#define PIN_BUTTON_2 9 // use for help
#define PIN_NEOPIXEL 10
#define PIN_BUZZER 12
#define PIN_BLUETOOTH_RX A0
#define PIN_BLUETOOTH_TX A1

/* Additionial Configuration */
#define NEOPIXEL_NUM 4
#define SERIAL_BAUDRATE 9600

/* Buzzer Note Configuration */
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494

/* Mode Configuration */
#define MODE_BABY 0
#define MODE_CHILD 1
#define MODE_ADULT 2
#define MODE_BABY_DIVIDE 30
#define MODE_CHILD_DIVIDE 50
#define MODE_ADULT_DIVIDE 60

/* Code Configuration */
#define CODE_BABY 'Q'
#define CODE_CHILD 'W'
#define CODE_ADULT 'E'
#define CODE_START 'R'

/* Color Selection */
#define COLOR_CLEAR 100
#define COLOR_RED 101
#define COLOR_GREEN 102
#define COLOR_YELLOW 103

Adafruit_NeoPixel pixels(NEOPIXEL_NUM, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
SoftwareSerial BT(PIN_BLUETOOTH_RX, PIN_BLUETOOTH_TX);

// HX711 circuit wiring
boolean BT_FLAG;
boolean now_state, pre_state;
boolean load_cell_flag;
boolean buzzer_flag;
////////////////////   GPIO 설정 부분   //////////////////////////////////
int data_arr[4];
int mode = MODE_BABY; // MODE_BABY: 영유아 / MODE_CHILD: 초등생 / MODE_ADULT: 성인
int press_depth;      // 가슴이 눌린 깊이 저장하는 변수
int count_good;
int count_bad_low;
int count_bad_high;
HX711 scale;

int get_weight()
{
  int reading;
  if (scale.is_ready())
  {
    String temp = String(scale.get_units());
    if (temp.charAt(0) == '-')
      reading = (-1) * temp.toInt();
    else
      reading = temp.toInt();
    //Serial.print("HX711 reading: ");
    //Serial.println(reading);
    return reading;
    // 로드셀 값을 강제로 양수로 만들어주는 함수
    // return scale?
  }
}

void pixelsColorAll(int color)
{
  uint32_t pixelColor;
  switch (color)
  {
  case COLOR_CLEAR:
    pixelColor = pixels.Color(0, 0, 0);
    break;

  case COLOR_RED:
    pixelColor = pixels.Color(150, 0, 0);
    break;

  case COLOR_GREEN:
    pixelColor = pixels.Color(0, 150, 0);
    break;

  case COLOR_YELLOW:
    pixelColor = pixels.Color(255, 255, 0);
    break;

  default:
    return;
  }

  for (int i = 0; i < NEOPIXEL_NUM; i++)
    pixels.setPixelColor(i, pixelColor);
  pixels.show();
}

void CPR()
{
  int num = get_weight(); // 로드셀 무게 업데이트
  int num2;
  if (num <= 12)
  {
    load_cell_flag = false;
    if (data_arr[3] == 1)
    {
      count_bad_high++;
      pixelsColorAll(COLOR_RED);
    }
    else if (data_arr[2] == 1)
    {
      count_good++;
      pixelsColorAll(COLOR_GREEN);
    }
    else if (data_arr[1] == 1)
    {
      count_good++;
      pixelsColorAll(COLOR_GREEN);
    }
    else if (data_arr[0] == 1)
    {
      count_bad_low++;
      pixelsColorAll(COLOR_YELLOW);
    }
    for (int i = 0; i < 4; i++)
    {
      data_arr[i] = 0;
    }
  }
  else
    load_cell_flag = true;
  ////////////////////////////////////////////////////////////////////////////////
  if (mode == MODE_BABY)
    num2 = num / MODE_BABY_DIVIDE;
  else if (mode == MODE_CHILD)
    num2 = num / MODE_CHILD_DIVIDE;
  else if (mode == MODE_ADULT)
    num2 = num / MODE_ADULT_DIVIDE;
  num2 = num2 - 1;
  Serial.println(num);
  for (int i = 0; i < num2; i++)
  {
    data_arr[i] = 1;
    pixelsColorAll(COLOR_CLEAR);
  }

  ////////////////////////////////////////////////////////////////////////////////
  if (millis() % 500 > 200)
  {
    buzzer_flag = false;
    noTone(PIN_BUZZER);
  }
  if (millis() % 500 < 200 && !buzzer_flag)
  {
    buzzer_flag = true;
    tone(PIN_BUZZER, NOTE_C4);
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUDRATE);
  BT.begin(SERIAL_BAUDRATE);
  pixels.begin();
  pixels.clear();
  scale.power_up();
  scale.begin(PIN_LOADCELL_1_DAT, PIN_LOADCELL_1_CLK);
  scale.power_up();
  scale.set_scale(2280.f); // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();            // reset the scale to 0
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pixelsColorAll(COLOR_CLEAR);
}

void loop()
{
#ifdef TEST
  tone(PIN_BUZZER, NOTE_A4);
#else
  if (Serial.available())
  {
    char data = Serial.read();
    if (data == 'A')
      BT_FLAG = true;
  }

  if (BT_FLAG)
  {
    BT_FLAG = false;
    count_good = 0;
    count_bad_low = 0;
    count_bad_high = 0;
    Serial.println("TIMR START");
    tone(PIN_BUZZER, NOTE_C4);
    delay(200);
    noTone(PIN_BUZZER);
    delay(100);
    tone(PIN_BUZZER, NOTE_C4);
    delay(200);
    noTone(PIN_BUZZER);
    delay(100);
    tone(PIN_BUZZER, NOTE_G4);
    delay(600);
    noTone(PIN_BUZZER);
    unsigned long temp_time = millis();

    while (1)
    {
      if (millis() - temp_time < 15000)
      {
        CPR();
      }
      else
      {
        // 점수 환산 알고리즘
        // { count_good 횟수 * 20 - ( count_low 횟수)*15 - ( count_high 횟수 )*5 } - 절댓값[30 - (전체횟수)]*25 }+100 = total Score (1000 점 만점)
        int total_score = 30 * (count_good)-15 * (count_bad_low)-5 * (count_bad_high)-10 * abs(30 - count_good - count_bad_low - count_bad_high) + 100;
        String temp_BT;
        temp_BT += count_bad_low;
        temp_BT += ",";
        temp_BT += count_good;
        temp_BT += ",";
        temp_BT += count_bad_high;
        temp_BT += ",";
        temp_BT += total_score;
        temp_BT += ",END";
        BT.println(temp_BT);
        Serial.println(temp_BT);
        tone(PIN_BUZZER, NOTE_G4);
        delay(200);
        noTone(PIN_BUZZER);
        delay(100);
        tone(PIN_BUZZER, NOTE_G4);
        delay(200);
        noTone(PIN_BUZZER);
        delay(100);
        tone(PIN_BUZZER, NOTE_C4);
        delay(400);
        noTone(PIN_BUZZER);
        delay(100);
        BT.println(temp_BT);
        pixelsColorAll(COLOR_CLEAR);
        break;
      }
    }
  }
  
  if (BT.available())
  {
    char data = BT.read();
    Serial.println(data);
    switch (data)
    {
    case CODE_BABY:
      mode = MODE_BABY;
      Serial.println("MODE BABY");
      break;
    
    case CODE_CHILD:
      mode = MODE_CHILD;
      Serial.println("MODE CHILD");
      break;
    
    case CODE_ADULT:
      mode = MODE_ADULT;
      Serial.println("MODE ADULT");
      break;
    
    case CODE_START:
      BT_FLAG = true;
      BT.println("0,0,0,0");
      break;
    
    default:
      break;
    }
  }
  /* Serial.print("GOOD  "); Serial.print(count_good);
  Serial.print("  BAD_LOW "); Serial.print(count_bad_low);
  Serial.print("  BAD_HIGH "); Serial.println(count_bad_high);*/
#endif
}