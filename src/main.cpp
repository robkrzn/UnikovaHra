#include <Arduino.h>
#include <U8x8lib.h>
#include <movingAvg.h>
#include <WiFi.h>
#include "esp32-hal-adc.h" // potrebne pre ADC
#include "soc/sens_reg.h" // potrebne pre ADC1

#include "morseovka.h"

#define MAXMOZNOSTI 6                                                                                                  //max moznosti hier ktoru su k dispozicii
const String nazvyHier[MAXMOZNOSTI] = {"Svetelna brana", "Morseovka", "LED HRA", "Miesaj farby", "Tlieskaj", "Dotyk"}; //nazvy hier z menu

// WIFI CAST
const char *ssid = "WifiDomaK";
const char *password = "1krizan2wifi3";

const char *host = "192.168.1.13";
const uint16_t port = 1234;

uint64_t reg_b;

char buff[20];

WiFiClient client;
//KONIEC WIFI CASTI

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE); //displej definicia

movingAvg priemerMerani(20); //klzavy priemer pre meranie morseovky

#define MERANIADOTYKUPREPRIEMER 5
movingAvg dotykMeranie1(MERANIADOTYKUPREPRIEMER); //klzavy priemer pre meranie dotyku
movingAvg dotykMeranie2(MERANIADOTYKUPREPRIEMER); ////klzavy priemer pre meranie dotyku
//definicie pinov
const int TlacidloModre = 18;   //pintlacidla
const int TlacidloCervene = 19; //pintlacidla
const int TlacidloZelene = 5;   //pintlacidla
const int PhotoresistorPin = 15; //pin footorezistora
const int ZvukPin = 39;

//dotyk
const int dotykPin1 = 27;
const int dotykPin2 = 14;

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
  //pinMode(PhotoresistorPin, INPUT);

  pinMode(TlacidloModre, INPUT);
  pinMode(TlacidloCervene, INPUT);
  pinMode(TlacidloZelene, INPUT);

  pinMode(redDioda, OUTPUT); //RGB dioda
  pinMode(greenDioda, OUTPUT);
  pinMode(blueDioda, OUTPUT);

  pinMode(ZvukPin, INPUT);

  priemerMerani.begin(); //klzavy priemer

  dotykMeranie1.begin();
  dotykMeranie2.begin();

  
  //WIFI CAST
  reg_b = READ_PERI_REG(SENS_SAR_READ_CTRL2_REG);

  Serial.print("\n\nPripajam sa k ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print(" : ");
  Serial.println(port);
  //KONIEC WIFI CASTI
  
}
int analogRead2(int pin){ //opravuje ADC pri wifi
  WRITE_PERI_REG(SENS_SAR_READ_CTRL2_REG, reg_b);
  SET_PERI_REG_MASK(SENS_SAR_READ_CTRL2_REG, SENS_SAR2_DATA_INV);
  return analogRead(pin);
}

void svetelnaBrana()
{
  while(true){

  hodnotaPhotorezistora = analogRead2(PhotoresistorPin);

  u8x8.setFont(u8x8_font_inb33_3x6_n);
  u8x8.drawString(0, 2, u8x8_u16toa(hodnotaPhotorezistora, 4));
  }
}

void morseovka()
{
  while(true){
  bool pismenoHotovo = false;
  int dlzkaPismena = 0;
  int znak[5] = {2, 2, 2, 2, 2};
  int prahovaUroven = 180;
  while (pismenoHotovo == false)
  {
    int pocitadloZnaku = 0;
    int pocitadloMedzery = 0;

    if (priemerMerani.reading(analogRead2(PhotoresistorPin)) < prahovaUroven)
    {
      while ((priemerMerani.reading(analogRead2(PhotoresistorPin)) < prahovaUroven) && pocitadloZnaku < 60000)
      {
        pocitadloZnaku++;
      }
      delay(10);
      while ((priemerMerani.reading(analogRead2(PhotoresistorPin)) >= prahovaUroven) && pocitadloMedzery < 200000)
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
      u8x8.print("                ");
      u8x8.setCursor(0, 3);
    }
  }
  char vyslednyZnak = SDekodovanaMorseovka(dlzkaPismena, znak);
  //Serial.printf(" %c\n", vyslednyZnak);
  u8x8.print(vyslednyZnak);
  }
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

void Tlieskanie()
{
  bool zap = false;
  int casovac = 0;
  int pocetTlesknuty = 0;
  while (true)
  {
    //WRITE_PERI_REG(SENS_SAR_READ_CTRL2_REG, read_ctl2);
    int hodnota = digitalRead(ZvukPin);

    if (hodnota == 1)
    {
      pocetTlesknuty++;
      digitalWrite(greenDioda, true);
      delay(100);
      digitalWrite(greenDioda, false);

      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 3);
      u8x8.printf("%d", pocetTlesknuty);
    }
    if (pocetTlesknuty == 3)
    {
      pocetTlesknuty = 0;
      casovac = 0;
      zap = !zap;
      if (zap)
      {
        digitalWrite(redDioda, true);
        u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
        u8x8.drawString(0, 4, "ZAP");
      }
      else
      {
        digitalWrite(redDioda, false);
        u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
        u8x8.drawString(0, 4, "VYP");
      }
      delay(100);
    }
    if (pocetTlesknuty > 0)
    {
      casovac++;
    }
    if (casovac > 2000)
    {
      pocetTlesknuty = 0;
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 3);
      u8x8.printf("%d", pocetTlesknuty);
      casovac = 0;
      digitalWrite(blueDioda, true);
      u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
      u8x8.drawString(0, 4, "NULL");
      delay(100);
      digitalWrite(blueDioda, false);
      u8x8.drawString(0, 4, "    ");
    }
    delay(1);
  }
}

void Dotyk()
{
  const int prahovaHodnota = 30;
  int dotykHodnota1 = dotykMeranie1.reading(touchRead(dotykPin1));
  int dotykHodnota2 = dotykMeranie2.reading(touchRead(dotykPin2));
  //u8x8.setFont(u8x8_font_inb33_3x6_n);
  //u8x8.drawString(0, 2, u8x8_u16toa(dotykMeranie1.reading(touchRead(dotykPin1)), 4));
  if (dotykHodnota1 < prahovaHodnota)
    digitalWrite(redDioda, HIGH);
  else
    digitalWrite(redDioda, LOW);
  if (dotykHodnota2 < prahovaHodnota)
    digitalWrite(greenDioda, HIGH);
  else
    digitalWrite(greenDioda, LOW);

  if (dotykHodnota1 < prahovaHodnota && dotykHodnota2 < prahovaHodnota)
  {
    u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
    u8x8.drawString(0, 4, "VYHRA");
  }
}

void loop()
{
  
  //WIFI CAST
  if (!client.connected())
    if (!client.connect(host, port))
    {
      Serial.println("pripojenie zlyhalo");
      //delay(5000);
      //return;
    }
  int a = 0;
  while (client.available())
  {
    char ch = static_cast<char>(client.read());
    Serial.print(ch);
    buff[a] = ch;
    a++;
  }
  String buff2(buff);
  if (buff[0] == 's' && buff[1] == 't' && buff[2] == 'a' && buff[3] == 'r' && buff[4] == 't')
  {
    zapnutaHra = true;
  }
  else if(buff2!=""){
  {
    //String buff2(buff);
    y = buff2.toInt();
  }
  }
  for(int z=0;z<20;z++){
    buff[z]=0;
  }
  //KONIEC WIFI CASTI
  
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
  
  if(zapnutaHra){
    //client.stop();
    //WiFi.disconnect();
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
  if (zapnutaHra == true && y == 4)
    Tlieskanie();
  if (zapnutaHra == true && y == 5)
    Dotyk();
}
