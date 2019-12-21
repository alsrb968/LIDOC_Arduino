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
  *  영아 모드 
  *  소아 모드
  *  성인 모드 
  * 
  * 
  */
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494


  
#include <Adafruit_NeoPixel.h>
#define PIN        5 // 네오픽셀 7번 핀 
#define NUMPIXELS  4 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#include "HX711.h"
#include <SoftwareSerial.h>
SoftwareSerial BT(A4,A5);

// HX711 circuit wiring
boolean BT_FLAG;
boolean now_state, pre_state;
boolean load_cell_flag;
boolean buzzer_flag;
////////////////////   GPIO 설정 부분   //////////////////////////////////

const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;
int START_BUTTON =8;
int HELP_BUTTON =9;
int PASSIVE_BUZZER =A2;
int data_arr[4];
////////////////////   GPIO 설정 부분   //////////////////////////////////
int mode = 0; // 0 영유아 / 1 초등생 / 2 성인
int press_depth; // 가슴이 눌린 깊이 저장하는 변수
int count_good;
int count_bad_low;
int count_bad_high;
HX711 scale;

int  get_weight()
{
  int reading; 
  if (scale.is_ready())
  {
    String temp = String(scale.get_units());
    if(temp.charAt(0) == '-')
      reading = (-1)*temp.toInt();
    else
      reading = temp.toInt();  
    //Serial.print("HX711 reading: ");
    //Serial.println(reading); 
    return reading;
    // 로드셀 값을 강제로 양수로 만들어주는 함수
    // return scale?
  }  
}
void clear_all()
{
  for(int i=0; i< NUMPIXELS; i++)
   pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  pixels.show(); 
}
void red_all()
{
  for(int i=0; i< NUMPIXELS; i++)
   pixels.setPixelColor(i, pixels.Color(150, 0, 0));
  pixels.show(); 
}
void green_all()
{
  for(int i=0; i< NUMPIXELS; i++)
   pixels.setPixelColor(i, pixels.Color(0, 150, 0));
  pixels.show(); 
}
void yellow_all()
{
  for(int i=0; i< NUMPIXELS; i++)
   pixels.setPixelColor(i, pixels.Color(255, 255, 0));
  pixels.show(); 
}
void CPR()
{
  int num = get_weight(); // 로드셀 무게 업데이트
  int num2;
  if( num <= 12)
  {
    load_cell_flag = false;
    if(data_arr[3] == 1)  {count_bad_high++;       red_all();}
    else if(data_arr[2] == 1) {count_good++;     green_all();}
    else if(data_arr[1] == 1) {count_good++;     green_all();}
    else if(data_arr[0] == 1) {count_bad_low++; yellow_all();}
    for(int i=0; i<4; i++)
    {
      data_arr[i] = 0;
    }
  }
  else
    load_cell_flag = true;
////////////////////////////////////////////////////////////////////////////////  
  if( mode == 0)
    num2 = num / 30;
  else if( mode == 1)
    num2 = num / 50; 
  else if( mode == 2)
    num2 = num / 60;
  num2 = num2-1;
  Serial.println(num);
  for(int i=0; i< num2; i++){
   data_arr[i] = 1;
  clear_all();}



////////////////////////////////////////////////////////////////////////////////
  if(millis() % 500 >200)
  {
    buzzer_flag = false;
    noTone(PASSIVE_BUZZER);
  }
  if(millis() % 500 <200 && !buzzer_flag)
  {
     buzzer_flag = true;
     tone(PASSIVE_BUZZER, NOTE_C4);
  }
  
}

void setup()
{
  Serial.begin(9600);
  BT.begin(9600);
  pixels.begin();
  pixels.clear();
  scale.power_up();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.power_up();
  scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();                // reset the scale to 0
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(HELP_BUTTON, INPUT_PULLUP);
  pinMode(PASSIVE_BUZZER, OUTPUT);
  clear_all();
}

void loop()
{
  if(Serial.available())
  {
    char data = Serial.read();
    if(data == 'A')
      BT_FLAG =true;
  }
  if(BT_FLAG)
  { 
    BT_FLAG = false;
    count_good =0;
    count_bad_low   =0;    
    count_bad_high  =0;
    Serial.println("TIMR START");
    tone(PASSIVE_BUZZER, NOTE_C4); delay(200); noTone(PASSIVE_BUZZER); delay(100);
    tone(PASSIVE_BUZZER, NOTE_C4); delay(200); noTone(PASSIVE_BUZZER); delay(100);
    tone(PASSIVE_BUZZER, NOTE_G4); delay(600);
    noTone(PASSIVE_BUZZER);
    unsigned long temp_time = millis();
    while(1)
    {
      if(millis() - temp_time < 15000)
      {
          CPR();
      }
      else
      {
          // 점수 환산 알고리즘
          // { count_good 횟수 * 20 - ( count_low 횟수)*15 - ( count_high 횟수 )*5 } - 절댓값[30 - (전체횟수)]*25 }+100 = total Score (1000 점 만점)
          int total_score = 30*(count_good) - 15*(count_bad_low) -5*(count_bad_high) -10*abs(30-count_good-count_bad_low-count_bad_high)+100;
          String temp_BT;
          temp_BT += count_bad_low;
          temp_BT +=",";
          temp_BT += count_good;
          temp_BT +=",";
          temp_BT += count_bad_high;
          temp_BT +=",";  
          temp_BT +=total_score;
          temp_BT +=",END"  ;      
          BT.println(temp_BT);
          Serial.println(temp_BT);
          tone(PASSIVE_BUZZER, NOTE_G4); delay(200); noTone(PASSIVE_BUZZER); delay(100);
          tone(PASSIVE_BUZZER, NOTE_G4); delay(200); noTone(PASSIVE_BUZZER); delay(100);
          tone(PASSIVE_BUZZER, NOTE_C4); delay(400); noTone(PASSIVE_BUZZER); delay(100);
           BT.println(temp_BT); 
           clear_all();
          break;
      }
    }
  }
  if(BT.available())
  {
    char data = BT.read();
    Serial.println(data);
    if( data == 'Q')
    {
      mode = 0;
      Serial.println("MODE 0");
    }
    else if( data == 'W')
    {
      mode = 1;
      Serial.println("MODE 1");
    }
    else if( data == 'E')
    {
      mode = 2;
      Serial.println("MODE 2");
    }
    else if( data == 'R')
    {
      BT_FLAG = true;
      BT.println("0,0,0,0");
    }
  }
 /* Serial.print("GOOD  "); Serial.print(count_good);
  Serial.print("  BAD_LOW "); Serial.print(count_bad_low);
  Serial.print("  BAD_HIGH "); Serial.println(count_bad_high);*/

}