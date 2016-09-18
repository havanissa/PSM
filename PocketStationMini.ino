#include <Wire.h>

#include <SPI.h>
#include <Adafruit_Si4713.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <Bounce2.h>
#include <avr/wdt.h>
#include <MemoryFree.h>


#define RESETPIN 12
#define OLED_RESET 4
#define SERIALDEBUG false

Adafruit_Si4713 radio = Adafruit_Si4713(RESETPIN);
Adafruit_SSD1306 display(OLED_RESET);
SoftwareSerial mp3player(7, 8); // RX, TX

Bounce btn1 = Bounce(); 
Bounce btn2 = Bounce(); 
Bounce btn3 = Bounce(); 
Bounce btn4 = Bounce(); 

int FMSTATION = 9150;
int song = 0;
boolean variance;
byte mp3_state = 0x00;
byte vol = 28;
byte h = 12;
byte dispState = 1; // 0. main 1. main menu 2. set Frequency 3. fq scan 4. Volume
byte menuState = 0;
byte radioState = 0; // 0. off 1. on
byte sysState = 1; // 0. standby 1. on 2. locked
byte playState = 2; // 0. Normal 1. Random 2. Super Random
byte error_count = 0;

byte rnd_current = 0;
byte rnd_songs = 0;
byte rnd_buffer[200];

unsigned long menu_timeout;
unsigned long status_timer;

void setup() {
  wdt_enable(WDTO_1S);
  // put your setup code here, to run once:
  // put your setup code here, to run once:
  #if (SERIALDEBUG)
  Serial.begin(115200);
  Serial.println(F("Adafruit Radio - Si4713 Test"));
  #endif
  mp3player.begin(9600);
  if (! radio.begin()) {  // begin with address 0x63 (CS high default)
    #if (SERIALDEBUG)
    Serial.println(F("Couldn't find radio?"));
    #endif
    while (1);
  }
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  
  pinMode(2, INPUT_PULLUP); // Antenna
  pinMode(3, INPUT_PULLUP); // Center
  pinMode(4, INPUT_PULLUP); // Up
  pinMode(5, INPUT_PULLUP); // Down

  btn1.attach(4); 
  btn1.interval(0);
  btn2.attach(3);
  btn2.interval(0);
  btn3.attach(5);
  btn3.interval(0);
  btn4.attach(2); // Antenna
  btn4.interval(0);
/*
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
*/
  attachInterrupt(digitalPinToInterrupt(2), interrupt1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), interrupt2, CHANGE);


  radio_start();
  mp3_send_cmd(0x0C);
  wdt_reset();
  delay(500);
  wdt_reset();
  mp3_send_cmd(0x0D);
  delay(500);
  wdt_reset();
  mp3_send_cmd(0x06,vol); // set volume
  delay(500);
  wdt_reset();
  delay(500);
  wdt_reset();
  delay(500);
  mp3_send_cmd(0x48); // Number of songsu
  wdt_reset();
  delay(500);
  wdt_reset();
  play_random();
  reset_menu_timer();
  super_random_rebuffer();
  display.clearDisplay();
}
void interrupt1()
{
   #if (SERIALDEBUG)
  Serial.println(F("Interrupts 1 changed"));
  #endif
}
void interrupt2()
{
  #if (SERIALDEBUG)
  Serial.println(F("Interrupts 2 changed"));
  #endif
  //mp3_send_cmd(0x01);
}
/*
 * byte rnd_current = 0;
 * byte rnd_buffer[255];
 * byte rnd_songs = 156;
 */
void super_random_next()
{
  rnd_current++;
  if(rnd_current>rnd_songs)
    super_random_rebuffer();
  mp3_send_cmd(0x03,rnd_buffer[rnd_current]);
}
void super_random_previous()
{
  rnd_current--;
  if(rnd_current>rnd_songs)
    rnd_current = 0;
  mp3_send_cmd(0x03,rnd_buffer[rnd_current]);
}

void super_random_rebuffer()
{
  int i;
  byte rnd;
  randomSeed(analogRead(0));
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(1,0);
  display.print(F("Randomizing...")); 
  display.display();
  // Clear buffer
  for(i=0;i<200;i++)
    rnd_buffer[i] = 0;
  // Refill buffer
  for(i=1;i<=rnd_songs;i++)
  {
    rnd = random(1,rnd_songs+1);
    if(rnd_buffer[rnd]==0)
      rnd_buffer[rnd] = i;
  }
  rnd_current = 0;
}
bool super_exist(byte num)
{

}
void play_next()
{
  if(playState==2)
    super_random_next();
  else
    mp3_send_cmd(0x01);
}
void play_previous()
{
  if(playState==2)
    super_random_previous();
  else
    mp3_send_cmd(0x02);
}
void radio_start()
{
  radio.powerUp();
  radio.setTXpower(115);  // dBuV, 88-115 max
  radio.tuneFM(FMSTATION);
  radio.beginRDS();
  radio.setRDSstation("Yarrrdio");
  radio.setRDSbuffer("Pirate radio!");
  radio.powerUp();
}
void radio_stop()
{
  radio.powerDown();
}
void vuBar()
{
  // Get radio data
  radio.readASQ();
  // Clear bar
  display.fillRect(0,54,display.width(),display.height(),0);
  int w = radio.currInLevel;
  if(w < -30)
    w = -30;
  w = map(w,-30,0,0,128);
  display.fillRect(0,54,w,display.height(),1);
}
void minivuBar()
{
  // Get radio data
  radio.readASQ();
  // Clear bar
  display.fillRect(0,0,48,10,0);
  int w = radio.currInLevel;
  if(w < -30)
    w = -30;
  w = map(w,-30,0,0,48);
  display.fillRect(0,0,w,10,1);
}
void reset_menu_timer()
{
  menu_timeout = millis() + 5000;
}
void play_random()
{
  int _song = song;
  mp3_send_cmd(0x0C);
  delay(500);
  wdt_reset();
  mp3_send_cmd(0x03,_song);
  delay(500);
  wdt_reset();
  mp3_send_cmd(0x06,vol);
  delay(500);
  wdt_reset();
  mp3_send_cmd(0x0D);
  playState = 0;
}
void buttons()
{
  btn1.update();
  btn2.update();
  btn3.update();
  btn4.update();

  switch(dispState)
  {
    case 1:
      // Main menu
      if(btn1.fell())
      {
        menuState--; // Menu up
        reset_menu_timer();
      }
      if(btn3.fell())
      {
        menuState++; // Menu down
        reset_menu_timer();
      }
      if(btn2.fell()) // Menu push
      {
        reset_menu_timer();
        switch(menuState)
        {
          case 0:
            // Go back
            dispState = 0;
            menuState = 0;
            break;
          case 1:
            // lock screen
            dispState = 0;
            menuState = 0;
            sysState = 2;
            btn2.interval(1000); // 5sek til unlock
            break;
          case 2:
            // set frequency
            dispState = 2;
            menuState = 2;
            btn1.interval(0);
            btn3.interval(0);
            break;
          case 3:
            // Random play
            if(playState==0)
            {
              playState = 2;
            }else{
              playState = 0;
            }
            super_random_rebuffer();
            /*
            if(playState==0)
            {
              mp3_send_cmd(0x18);
              delay(500);
              wdt_reset();
              mp3_send_cmd(0x0D);
              delay(500);
              wdt_reset();
              mp3_send_cmd(0x06,vol);
              playState = 1;
            }else{
              int _song = song;
              mp3_send_cmd(0x0C);
              delay(500);
              wdt_reset();
              mp3_send_cmd(0x03,_song);
              delay(500);
              wdt_reset();
              mp3_send_cmd(0x06,vol);
              delay(500);
              wdt_reset();
              mp3_send_cmd(0x0D);
              playState = 0;
            }
            */
            break;
          case 4:
            // Volume
            dispState = 4;
            break;  
          case 5:
            dispState = 3;
            // Find best frequency
            break;
          case 6:
            // Soft reset
            asm volatile ("jmp 0");  
            break;
        }
      }
      break;
    case 3:
      break;
    case 2:
      // Change frequency
      if(btn1.read())
      {
        FMSTATION -= 10; // Frequency up
        reset_menu_timer();
      }
      if(btn3.read())
      {
        FMSTATION += 10; // Frequency down
        reset_menu_timer();
      }
      if(FMSTATION>10800)
      {
        FMSTATION = 8750;
      }
      if(FMSTATION<8750)
      {
        FMSTATION = 10800;
      }
      delay(50);
      if(btn2.fell()) // Frequency select
      {
        radio_stop();
        radio_start();
        dispState = 0;
        menuState = 0;
        btn2.interval(500);
        wdt_reset();
        btn2.interval(500);
        wdt_reset();
        reset_menu_timer();
      }
       
      break;
    case 4:
       if(btn1.fell())
       {
         vol++;
         if(vol>30)
          vol = 30;
         mp3_send_cmd(0x06,vol);
         reset_menu_timer();
       }
       if(btn3.fell())
       {
         vol--;
         if(vol<0)
          vol = 0;
         mp3_send_cmd(0x06,vol);
         reset_menu_timer();
       }
       if(btn2.fell())
       {
        dispState = 1;
        reset_menu_timer();
       }
      break;
    default:
      if(btn2.fell())
      {
        reset_menu_timer();
        if(sysState==2)
        {// Unlocka
           btn2.interval(1000);
           sysState = 1;
        }else{
          
          // Activate menu!
          dispState = 1;
          menuState = 0;
          btn2.interval(1000);
          btn2.interval(500);
          wdt_reset();
          btn2.interval(500);
          wdt_reset();
        }
      }
      if(btn1.fell() && sysState==1)
        play_next();
      if(btn3.fell() && sysState==1 && btn2.read()==HIGH)
        play_previous(); // Previous!
      
      return;
  }
  return;
  if(btn1.fell())
  {
    #if (SERIALDEBUG)
    Serial.println(F("Button1"));
    #endif
    h++;
   
    vol++;
    if(vol>30)
      vol = 30;
    mp3_send_cmd(0x06,vol);
   
    
    //radio_start();
  }
  if(btn2.fell())
  {
    #if (SERIALDEBUG)
    Serial.println(F("Button2"));
    #endif
    play_next();
  }
  if(btn3.fell())
  {
    #if (SERIALDEBUG)
    Serial.println(F("Button3"));
    #endif
    h--;
    
   
    vol--;
    if(vol<0)
      vol = 0;
    mp3_send_cmd(0x06,vol);
   
    //radio.powerDown();
  }
}
void best_freq()
{
  int row = 0;
  radio_stop();
  radio.powerUp();
  int steps = (10800-8750) / 128;
  int best_f = 0;
  int best_m = 99999;
  for (uint16_t f  = 8750; f<10750; f+=steps) {
    radio.readTuneMeasure(f);
    
    radio.readTuneStatus();
    int messurement = radio.currNoiseLevel;
    if(messurement < best_m)
    {
      best_m = messurement;
      best_f = f;
    }
    messurement = map(messurement,20,100,0,64);
    display.drawLine(row+1, 0, row+1, 64, WHITE);
    display.drawLine(row, 0, row, 64, BLACK);
    display.drawLine(row, 64-messurement, row, 64, WHITE);
    row++;
    if(row>128)
      row=0;
    display.display();
    wdt_reset();
  }
  radio_stop();
  FMSTATION = best_f;
  dispState = 2;
}
void main_screen()
{
  byte batt = 0;
  // Frequency
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(1,0);
  if(FMSTATION/100 < 100)
    display.print(F(" ")); 
  display.print(FMSTATION/100); 
  display.print(F("."));
  display.print((FMSTATION % 100)/10);
  display.setTextSize(2);
  display.print(F("MHz")); 
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Volume meter
  radio.readASQ();
  display.fillRect(0,29,display.width(),21,0);
  int w = radio.currInLevel;
  if(w < -30)
    w = -30;
  w = map(w,-30,0,0,128);
  display.fillRect(0,29,w,21,1);
  display.fillRect(0,39,display.width(),1,0);

  /*
  // Lock
  static const unsigned char PROGMEM locked8b[] =
  {
    B00111100,
    B01100110,
    B11000011,
    B10000001,
    B11111111,
    B11100111,
    B11100111,
    B11111111,
    B01111110
  };
  static const unsigned char PROGMEM unlocked8b[] =
  {
    B00111100,
    B01100000,
    B11000000,
    B10000000,
    B11111111,
    B11100111,
    B11100111,
    B11111111,
    B01111110
  };

  if(sysState==2)
    display.drawBitmap(1,54,locked8b,8,9,1);
  else
    display.drawBitmap(1,54,unlocked8b,8,9,1);
*/

  // Status
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  display.setCursor(3,55); // 12 við lás
  /*
  if(mp3_state == 0x01)
    display.print(F("Spaelur"));
  if(mp3_state == 0x00)
    display.print(F("Stoppa"));
  */
  if(mp3_state == 0x01)
    display.print(F("SP"));
  if(mp3_state == 0x00)
    display.print(F("ST"));
  display.print(F(" R"));
  display.print(rnd_current);
  display.print(F("/"));
  display.print(rnd_songs);
  
  
  
  display.setCursor(90,55); // x 60, við battarí
  if (song<1000)display.print(F("0"));
  if (song<100)display.print(F("0"));
  if (song<10) display.print(F("0"));
  display.print(song);
/*
  //Battery
  display.fillRect(91,52,35,12,1);
  display.fillRect(92,53,33,10,0);
  display.fillRect(126,54,2,8,1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(94,55);
  display.print(F(" N/A"));
*/
/*
  display.setTextColor(BLACK);
  display.setCursor(1,30);
  display.print(vol);
*/
}
void show_menu()
{
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15,0);
  display.println(F("Aftur"));
  display.setCursor(15,10);
  display.println(F("Laes skerm"));
  display.setCursor(15,20);
  display.println(F("Stilla frekvens"));
  display.setCursor(15,30);
  if(playState==2)
    display.println(F("Tilvildarligt   On"));
  else
    display.println(F("Tilvildarligt  Off"));
  
  display.setCursor(15,40);
  display.println(F("Ljod styrki"));
  display.setCursor(15,50);
  display.println(F("Finn besta frekven"));
  display.setCursor(15,60);
  display.println(F("Bleytt reset"));
  
  

  if(menuState>6)
    menuState = 6;
  if(menuState<0)
    menuState = 0;
  
  display.setCursor(0,menuState*10);
  display.println(F(">"));
}
void show_fq_change()
{
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(1,0);
  if(FMSTATION/100 < 100)
    display.print(F(" ")); 
  display.print(FMSTATION/100); 
  display.print(F("."));
  display.print((FMSTATION % 100)/10);
  display.setTextSize(2);
  display.print(F("MHz")); 
}
void show_volume()
{
  radio.readASQ();
  // Clear bar
  display.fillRect(0,0,display.width(),30,0);
  int w = radio.currInLevel;
  if(w < -30)
    w = -30;
  w = map(w,-30,0,0,128);
  display.fillRect(0,0,w,30,1);
  display.fillRect(0,32,display.width(),10,0);
  w = map(vol,0,30,0,128);
  display.fillRect(0,32,w,10,1);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(60,47);
  if (vol<10) display.print(F("0"));
  display.print(vol);
}
void show_display()
{
  switch(dispState)
  {
    case 1:
      display.clearDisplay();
      show_menu();
      display.display();
      break;
    case 2:
      display.clearDisplay();
      show_fq_change();
      display.display();
      break;
    case 3:
      display.clearDisplay();
      best_freq();
      break;
    case 4:
      display.clearDisplay();
      show_volume();
      display.display();
      break;
    default:
      display.clearDisplay();
      main_screen(); // Mainscreen
      display.display();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  buttons();
  show_display();
  mp3_rec();
  #if (SERIALDEBUG)
    Serial.print(F("dispState: "));
    Serial.println(dispState);
    Serial.print(F("menuState: "));
    Serial.println(menuState);
  #endif
  if(millis() > menu_timeout)
  {
    // Menu timeout, return to main screen
    dispState = 0;
    menuState = 0;
    menu_timeout = millis() + 5000;
    
    mp3_send_cmd(0x48); // Number of songs
    #if (SERIALDEBUG)
    Serial.println(F("Menu timeout"));
    #endif
  }
  if(millis() > status_timer)
  {
    if(variance)
      mp3_send_cmd(0x42); // Current status
    else
      mp3_send_cmd(0x4B); // Current track
    variance = !variance;
    status_timer = millis() + 1000;
  }
  wdt_reset();
}






















