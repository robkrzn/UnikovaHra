#include <Arduino.h>
#include <U8x8lib.h>

#define MAXMOZNOSTI 5                                                                                             //max moznosti hier ktoru su k dispozicii
const String nazvyHier[MAXMOZNOSTI] = {"Svetelna brana", "TEST HRY 2", "TEST HRY 3", "TEST HRY 4", "TEST HRY 5"}; //nazvy hier z menu

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE); //displej definicia

//definicie pinov
const int TlacidloModre = 18;   //pintlacidla
const int TlacidloCervene = 19; //pintlacidla
const int PhotoresistorPin = 2; //pin footorezistora

//globalne pomocne premenne
int i = 1, y = 0, x = 1; //pomocne premenne
bool zapnutaHra = false;
int StavModrehoTlacidla; //nove pomocne tlacidlo
int hodnotaPhotorezistora;
bool zmenaModrehoTlacidla;
int StavCervenehoTlacidla;
bool zmenaCervenehoTlacidla;

void setup()
{
  u8x8.begin();
  pinMode(TlacidloModre, INPUT);
  pinMode(TlacidloCervene, INPUT);
}

void svetelnaBrana()
{
  hodnotaPhotorezistora = analogRead(PhotoresistorPin);

  u8x8.setFont(u8x8_font_inb33_3x6_n);
  u8x8.drawString(0, 2, u8x8_u16toa(hodnotaPhotorezistora, 4));
}

void loop()
{
  StavModrehoTlacidla = digitalRead(TlacidloModre);
  StavCervenehoTlacidla = digitalRead(TlacidloCervene);
  if (StavCervenehoTlacidla == true)
    zapnutaHra = true;

  if (StavModrehoTlacidla == HIGH && zmenaModrehoTlacidla && zapnutaHra == false)
  {
    y = y + 1;
    if (y > MAXMOZNOSTI - 1)
      y = 0;
    zmenaModrehoTlacidla = false;
  }
  else if (StavModrehoTlacidla == LOW)
  {
    zmenaModrehoTlacidla = true;
    delay(50); //kratky delay kvoli problemom s tlacidlom a doslo len k jednemu stlaceniu
  }

  if (x != y)
  {
    x = y;
    u8x8.clear();
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
    u8x8.setCursor(0, 1);
    u8x8.noInverse();
    u8x8.print(nazvyHier[x]);
  }
  //MENU
  if (zapnutaHra == true && y == 0)
    svetelnaBrana();
}
