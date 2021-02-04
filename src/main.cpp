#include <Arduino.h>
#include <U8x8lib.h>
#include <movingAvg.h>

#include "morseovka.h"

#define MAXMOZNOSTI 5                                                                                           //max moznosti hier ktoru su k dispozicii
const String nazvyHier[MAXMOZNOSTI] = {"Svetelna brana", "Morseovka", "LED HRA", "Miesaj farby", "TEST HRY 5"}; //nazvy hier z menu

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE); //displej definicia

movingAvg priemerMerani(20);
//definicie pinov
const int TlacidloModre = 18;   //pintlacidla
const int TlacidloCervene = 19; //pintlacidla
const int TlacidloZelene = 5;   //pintlacidla
const int PhotoresistorPin = 2; //pin footorezistora

//RGB MODUL
const int redDioda = 32;
const int greenDioda = 33;
const int blueDioda = 25;

//globalne pomocne premenne
int y = 0, x = 1; //pomocne premenne
bool zapnutaHra = false;
int StavModrehoTlacidla; //nove pomocne tlacidlo
int hodnotaPhotorezistora;
bool zmenaModrehoTlacidla;
int StavCervenehoTlacidla;
bool zmenaCervenehoTlacidla;

bool LEDHraKoniec = false;
bool MiesanieFariebKoniec = false;

void setup()
{
  u8x8.begin();
  Serial.begin(115200);

  pinMode(TlacidloModre, INPUT);
  pinMode(TlacidloCervene, INPUT);
  pinMode(TlacidloZelene, INPUT);

  pinMode(redDioda, OUTPUT); //RGB dioda
  pinMode(greenDioda, OUTPUT);
  pinMode(blueDioda, OUTPUT);

  priemerMerani.begin(); //klzavy priemer
}

void svetelnaBrana()
{
  hodnotaPhotorezistora = analogRead(PhotoresistorPin);

  u8x8.setFont(u8x8_font_inb33_3x6_n);
  u8x8.drawString(0, 2, u8x8_u16toa(hodnotaPhotorezistora, 4));
}

void morseovka()
{
  bool pismenoHotovo = false;
  int dlzkaPismena = 0;
  int znak[5] = {2, 2, 2, 2, 2};
  int prahovaUroven = 180;
  while (pismenoHotovo == false)
  {
    int pocitadloZnaku = 0;
    int pocitadloMedzery = 0;

    if (priemerMerani.reading(analogRead(PhotoresistorPin)) < prahovaUroven)
    {
      while ((priemerMerani.reading(analogRead(PhotoresistorPin)) < prahovaUroven) && pocitadloZnaku < 60000)
      {
        pocitadloZnaku++;
      }
      delay(10);
      while ((priemerMerani.reading(analogRead(PhotoresistorPin)) >= prahovaUroven) && pocitadloMedzery < 200000)
      {
        pocitadloMedzery++;
      }
      int pomZnak = rozpoznavacPrvku(pocitadloZnaku); //rozpoznavac znaku
      if (pomZnak != 2)
      {
        znak[dlzkaPismena] = pomZnak;
        dlzkaPismena++;
      }
      else
        pismenoHotovo = true;
    }
    if (pocitadloMedzery > 20000)
      pismenoHotovo = 1;

    if (digitalRead(TlacidloCervene) == true)
    {
      u8x8.setCursor(0, 3);
      u8x8.print("              ");
      u8x8.setCursor(0, 3);
    }
  }
  char vyslednyZnak = SDekodovanaMorseovka(dlzkaPismena, znak);
  //Serial.printf(" %c\n", vyslednyZnak);
  u8x8.print(vyslednyZnak);
}

void LEDHra()
{
  int pocitadloKol = 0;
  int maxKol = 5;
  while (LEDHraKoniec == false)
  {
    delay(1000);
    bool prehra = false;
    int nahodneCislo = random(1, 4);
    bool koloUkoncene = false;
    if (nahodneCislo == 1)
    {
      while (koloUkoncene == false)
      {
        digitalWrite(redDioda, HIGH);
        if (digitalRead(TlacidloCervene) == true)
        {
          digitalWrite(redDioda, LOW);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nCervena  dobre %d", pocitadloKol);
        }
        else if (digitalRead(TlacidloModre) || digitalRead(TlacidloZelene))
        {
          digitalWrite(redDioda, LOW);
          prehra = true;
          koloUkoncene = true;
          pocitadloKol = 0;
          Serial.printf("\nCervena  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 2)
    {
      while (koloUkoncene == false)
      {
        digitalWrite(greenDioda, HIGH);
        if (digitalRead(TlacidloZelene) == true)
        {
          digitalWrite(greenDioda, LOW);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nZelena  dobre %d", pocitadloKol);
        }
        else if (digitalRead(TlacidloModre) || digitalRead(TlacidloCervene))
        {
          digitalWrite(greenDioda, LOW);
          prehra = true;
          koloUkoncene = true;
          pocitadloKol = 0;
          Serial.printf("\nZelena  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 3)
    {
      while (koloUkoncene == false)
      {
        digitalWrite(blueDioda, HIGH);
        if (digitalRead(TlacidloModre) == true)
        {
          digitalWrite(blueDioda, LOW);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nModra  dobre %d", pocitadloKol);
        }
        else if (digitalRead(TlacidloCervene) || digitalRead(TlacidloZelene))
        {
          digitalWrite(blueDioda, LOW);
          prehra = true;
          koloUkoncene = true;
          pocitadloKol = 0;
          Serial.printf("\nModra  zle %d", pocitadloKol);
        }
      }
    }
    if (prehra == true)
    {
      digitalWrite(redDioda, HIGH);
      delay(500);
      digitalWrite(redDioda, LOW);
      delay(500);
      digitalWrite(redDioda, HIGH);
      delay(500);
      digitalWrite(redDioda, LOW);
      delay(500);
      u8x8.setCursor(0, 4);
      u8x8.print("Zle, znova");
    }
    if (pocitadloKol == maxKol)
    {
      LEDHraKoniec = true;
      Serial.printf("\nVyhra  dobre %d", pocitadloKol);
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Vyhral si");
    }
    u8x8.setCursor(0, 3);
    u8x8.print("                ");
    u8x8.setCursor(0, 3);
    u8x8.printf("%d/%d", pocitadloKol, maxKol);
  }
}

void MiesanieFarieb()
{
  int pocitadloKol = 0;
  int maxKol = 5;
  while (MiesanieFariebKoniec == false)
  {
    delay(1000);
    digitalWrite(greenDioda, LOW);
    digitalWrite(redDioda, LOW);
    digitalWrite(blueDioda, LOW);
    bool prehra = false;
    int nahodneCislo = random(1, 5);
    bool koloUkoncene = false;
    if (nahodneCislo == 1)
    {
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Fialova");
      while (koloUkoncene == false)
      {
        if (digitalRead(TlacidloCervene) && digitalRead(TlacidloModre))
        {
          digitalWrite(redDioda, HIGH);
          digitalWrite(blueDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nFiaolova  dobre %d", pocitadloKol);
        }
        else if (digitalRead(TlacidloZelene))
        {
          prehra = true;
          koloUkoncene = true;
          pocitadloKol = 0;
          Serial.printf("\nFiaolova  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 2)
    {
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Tyrkysova");
      while (koloUkoncene == false)
      {
        if (digitalRead(TlacidloZelene) && digitalRead(TlacidloModre))
        {
          digitalWrite(greenDioda, HIGH);
          digitalWrite(blueDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nTyrkysova  dobre %d", pocitadloKol);
        }
        else if (digitalRead(TlacidloCervene))
        {
          prehra = true;
          koloUkoncene = true;
          pocitadloKol = 0;
          Serial.printf("\nTyrkysova  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 3)
    {
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Zlta");
      while (koloUkoncene == false)
      {
        if (digitalRead(TlacidloZelene) && digitalRead(TlacidloCervene))
        {
          digitalWrite(greenDioda, HIGH);
          digitalWrite(redDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nZlta  dobre %d", pocitadloKol);
        }
        else if (digitalRead(TlacidloModre))
        {
          prehra = true;
          koloUkoncene = true;
          pocitadloKol = 0;
          Serial.printf("\nZlta  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 4)
    {
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Biela");
      while (koloUkoncene == false)
      {
        if (digitalRead(TlacidloZelene) && digitalRead(TlacidloCervene) && digitalRead(TlacidloModre))
        {
          digitalWrite(greenDioda, HIGH);
          digitalWrite(redDioda, HIGH);
          digitalWrite(blueDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nBiela  dobre %d", pocitadloKol);
        }
      }
    }
    if (prehra == true)
    {
      digitalWrite(redDioda, HIGH);
      delay(500);
      digitalWrite(redDioda, LOW);
      delay(500);
      digitalWrite(redDioda, HIGH);
      delay(500);
      digitalWrite(redDioda, LOW);
      delay(500);
      u8x8.setCursor(0, 4);
      u8x8.print("Zle, znova");
    }
    if (pocitadloKol == maxKol)
    {
      MiesanieFariebKoniec = true;
      Serial.printf("\nVyhra  dobre %d", pocitadloKol);
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Vyhral si");
    }
    u8x8.setCursor(0, 3);
    u8x8.print("                ");
    u8x8.setCursor(0, 3);
    u8x8.printf("%d/%d", pocitadloKol, maxKol);
  }
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
    u8x8.print(nazvyHier[x]);
    u8x8.setCursor(0, 3);
  }
  //MENU
  if (zapnutaHra == true && y == 0)
    svetelnaBrana();
  if (zapnutaHra == true && y == 1)
    morseovka();
  if (zapnutaHra == true && y == 2)
    LEDHra();
  if (zapnutaHra == true && y == 3)
    MiesanieFarieb();
}
