#include <Arduino.h>
#include <U8x8lib.h>
#include <movingAvg.h>
#include <FirebaseESP32.h>

#include "morseovka.h"

#define MAXMOZNOSTI 8                                                                                                                 //max moznosti hier ktoru su k dispozicii
const String nazvyHier[MAXMOZNOSTI] = {"Svetelna brana", "Morseovka", "LED HRA", "Miesaj farby", "Tlieskaj", "Dotyk", "Vzdialenost", "Voda"}; //nazvy hier z menu

// WIFI CAST
#define FIREBASE_HOST "unikova-hra-default-rtdb.firebaseio.com" //databaza udaje
#define FIREBASE_AUTH "gvsLlFof5OhtvW9SEFnpdLYzI6lVgJujVxD7HIHc"
#define WIFI_SSID "WifiDomaK"         //wifi meno
#define WIFI_PASSWORD "1krizan2wifi3" //wifi heslo

FirebaseData fireData;         //databaza
String cesta = "/Zariadenie/"; //cestat k databaze
bool online = false;
int LEDidentifikator = 0;
bool LEDzmena = true; //prve zopnutie diod
//KONIEC WIFI CASTI

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE); //displej definicia

movingAvg priemerSvetelnejBrany(2); //klzavy priemer pre svetlenej brany
movingAvg priemerMerani(20);         //klzavy priemer pre meranie morseovky

#define MERANIADOTYKUPREPRIEMER 5
movingAvg dotykMeranie1(MERANIADOTYKUPREPRIEMER); //klzavy priemer pre meranie dotyku
movingAvg dotykMeranie2(MERANIADOTYKUPREPRIEMER); ////klzavy priemer pre meranie dotyku

//tlacidla
bool zmenaModrehoTlacidla;
int y = 0, x = 55;            //pomocne premenne
const int tlacidloModre = 18; //pintlacidla
void IRAM_ATTR tlacidloModrePrerusenie() { zmenaModrehoTlacidla = true; }

bool zapnutaHra = false;
const int tlacidloCervene = 19;  //pintlacidla
const int tlacidloZelene = 5;    //pintlacidla
const int photoresistorPin = 35; //pin footorezistora
const int zvukPin = 39;

//dotyk
const int dotykPin1 = 27;
const int dotykPin2 = 14;

//RGB MODUL
const int redDioda = 32;
const int greenDioda = 33;
const int blueDioda = 25;

//globalne pomocne premenne

int stavModrehoTlacidla; //nove pomocne tlacidlo

bool hotovo = false;
String ipPom;
bool infoVypis = false; //pomocna premenna pre konecny vystup

const int relePin = 26;

//HC-SR04
int8_t trigPin = 17;
int8_t echoPin = 16;

//vodny senzor
const int vodnyPin = 34;
const int vodnyPinVcc = 4;

void setup()
{
  u8x8.begin();         //inicializacia displeja
  Serial.begin(115200); //nestavenie komunikacnej rychlosti
  //pinMode(photoresistorPin, INPUT);

  pinMode(tlacidloModre, INPUT); //nastavenie vystupov
  attachInterrupt(tlacidloModre, tlacidloModrePrerusenie, FALLING);
  pinMode(tlacidloCervene, INPUT);
  pinMode(tlacidloZelene, INPUT);

  pinMode(redDioda, OUTPUT); //RGB dioda
  pinMode(greenDioda, OUTPUT);
  pinMode(blueDioda, OUTPUT);
  pinMode(relePin, OUTPUT); //relevystup

  pinMode(zvukPin, INPUT);

  pinMode(trigPin, OUTPUT); //HC-SR04
  pinMode(echoPin, INPUT);  //HC-SR04

  pinMode(vodnyPinVcc, OUTPUT); //VODA START

  priemerMerani.begin();         //klzavy priemer
  priemerSvetelnejBrany.begin(); //klzavy priemer

  dotykMeranie1.begin(); //klzavy priemer
  dotykMeranie2.begin(); //klzavy priemer

  //WIFI CAST

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //nastavenie udajov wifi
  Serial.print("\nPripajam sa k Wifi"); //info vypis
  while (WiFi.status() != WL_CONNECTED) //pokusanie sa pripojit
  {
    Serial.print(".");
    delay(300);
  }
  Serial.print(".OK\nPripojene s IP: ");
  IPAddress ip = WiFi.localIP(); //ulozenie IP adresy
  Serial.println(ip);

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //nastavenie udajov databazy
  Firebase.reconnectWiFi(true);                 //aktivovane znovu pripajanie
  if (!Firebase.beginStream(fireData, cesta))   //pokus pripojit sa k databaze
    Serial.println("Problem: " + fireData.errorReason());

  ipPom = ip.toString();

  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f); //vypis na displej
  u8x8.setCursor(0, 6);
  u8x8.print("~~~~~~~~~~~~~~~~");
  u8x8.setCursor(0, 7);
  u8x8.print(ipPom);
  for (int i = 0; i < ipPom.length(); i++) //v ip adrese nahradenie bodiek ciarkami
  {
    if (ipPom[i] == '.')
      ipPom[i] = '-';
  }
  cesta += ipPom + "/"; //doplnenie cesty
  Serial.print("Nacitanie dat z FireBase");
  if (Firebase.getInt(fireData, cesta + "Volby"))
  {                                             //pri dostupnosti dat sa nastavia predchadzajuce parametre
    Firebase.getJSON(fireData, cesta);          //ziskanie udajov
    FirebaseJson &json = fireData.jsonObject(); //formatovanie JSON dat
    FirebaseJsonData jsonData;
    json.get(jsonData, "Volby");
    if (jsonData.type == "int")
      y = jsonData.intValue;
    json.get(jsonData, "LED");
    if (jsonData.type == "int")
      LEDidentifikator = jsonData.intValue;
    json.iteratorEnd();
    Firebase.setBool(fireData, cesta + "Start", false); //nastavanie udajov
    Firebase.setBool(fireData, cesta + "Hotovo", false);
    Firebase.setBool(fireData, cesta + "Online", true);
  }
  else
  {                                                    //pri nedostupnosti dat sa parametre nastavia nanovo
    Firebase.setString(fireData, cesta + "Id", ipPom); //nastavenie noveho zariadenia
    Firebase.setBool(fireData, cesta + "Start", false);
    Firebase.setInt(fireData, cesta + "Volby", 0);
    Firebase.setInt(fireData, cesta + "LED", LEDidentifikator);
    Firebase.setBool(fireData, cesta + "Hotovo", false);
    Firebase.setBool(fireData, cesta + "Posledne", false);
    Firebase.setBool(fireData, cesta + "Online", true);
  }
  Serial.print("...OK\n");
  //KONIEC WIFI CASTI
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
  u8x8.setCursor(6, 0);
  u8x8.print("MENU");
  u8x8.setCursor(0, 1);
  u8x8.print("~~~~~~~~~~~~~~~~");
}

void svetelnaBrana()
{
  int prahovaUroven = 350;
  int hodnotaPhotorezistora;
  int pocetPocitadla = 10;
  int pocitadlo = 0;
  bool zapnute = false;
  u8x8.setFont(u8x8_font_inb33_3x6_n);
  while (!hotovo)
  {
    hodnotaPhotorezistora = priemerSvetelnejBrany.reading(analogRead(photoresistorPin));

    if ((hodnotaPhotorezistora < prahovaUroven))
      zapnute = true;
    if (zapnute && hodnotaPhotorezistora > prahovaUroven)
    {
      pocitadlo++;
      zapnute = false;
      Serial.printf("%d\n", pocitadlo);
    }
    u8x8.drawString(0, 2, u8x8_u16toa(hodnotaPhotorezistora, 4)); //info vypis o hodnote

    if (pocitadlo == pocetPocitadla)
    {
      pocitadlo = 0;
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      hotovo = true;
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 1);
      u8x8.print("ULOHA SPLNENA");
    }
  }
}

void morseovka()
{
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
  u8x8.setCursor(0, 0);
  u8x8.print("Text:");
  u8x8.setCursor(0, 3);
  char odpoved[10] = {0};
  int indexOdpovede = 0;
  while (!hotovo)
  {
    bool pismenoHotovo = false;
    int dlzkaPismena = 0;
    int znak[5] = {2, 2, 2, 2, 2};
    int prahovaUroven = 350;
    int pocitadloZnaku = 0;
    int pocitadloMedzery = 0;
    while (pismenoHotovo == false)
    {
      pocitadloZnaku = 0;
      pocitadloMedzery = 0;

      if (priemerMerani.reading(analogRead(photoresistorPin)) < prahovaUroven)
      {
        while ((priemerMerani.reading(analogRead(photoresistorPin)) < prahovaUroven) && pocitadloZnaku < 10000)
        {
          pocitadloZnaku++;
        }
        delay(10);
        while ((priemerMerani.reading(analogRead(photoresistorPin)) >= prahovaUroven) && pocitadloMedzery < 15000)
        {
          pocitadloMedzery++;
        }
        Serial.printf("Znak %d ", pocitadloZnaku);
        int pomZnak = rozpoznavacPrvku(pocitadloZnaku); //rozpoznavac znaku
        Serial.printf("\nMedzera %d\n", pocitadloMedzery);
        if (pomZnak != 2)
        {
          znak[dlzkaPismena] = pomZnak;
          dlzkaPismena++;
        }
        else
          pismenoHotovo = true;
      }
      if (pocitadloMedzery > 5000)
        pismenoHotovo = 1;
    }
    char vyslednyZnak = dekodovanaMorseovka(dlzkaPismena, znak);
    u8x8.print(vyslednyZnak);
    Serial.printf(" %c\n------------\n", vyslednyZnak);
    odpoved[indexOdpovede] = vyslednyZnak;
    indexOdpovede++;
    if (indexOdpovede == 9)
      indexOdpovede = 0;
    if (overOdpoved(odpoved)) //ak sa zhoduje text s vyhernym textom
    {
      Serial.printf("VYHRA");
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      u8x8.setCursor(0, 4);
      u8x8.print("ULOHA SPLNENA");
      hotovo = true;
    }
    if (pocitadloMedzery == 15000) //po zobrazeni slova sa text zmaze
    {
      delay(2000);
      u8x8.setCursor(0, 3);
      u8x8.print("                ");
      u8x8.setCursor(0, 3);
      indexOdpovede = 0;
      for (size_t i = 0; i < 9; i++)
        odpoved[i] = 0;
    }
  }
}

void LEDHra()
{
  int pocitadloKol = 0;
  int maxKol = 5;
  while (hotovo == false)
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
        if (digitalRead(tlacidloCervene) == true)
        {
          digitalWrite(redDioda, LOW);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nCervena  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloModre) || digitalRead(tlacidloZelene))
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
        if (digitalRead(tlacidloZelene) == true)
        {
          digitalWrite(greenDioda, LOW);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nZelena  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloModre) || digitalRead(tlacidloCervene))
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
        if (digitalRead(tlacidloModre) == true)
        {
          digitalWrite(blueDioda, LOW);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nModra  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloCervene) || digitalRead(tlacidloZelene))
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
      hotovo = true;
      Serial.printf("\nVyhra  dobre %d", pocitadloKol);
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Vyhral si");
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
    }
    u8x8.setCursor(0, 3);
    u8x8.print("                ");
    u8x8.setCursor(0, 3);
    u8x8.printf("%d/%d", pocitadloKol, maxKol);
  }
}

void miesanieFarieb()
{
  int pocitadloKol = 0;
  int maxKol = 5;
  while (hotovo == false)
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
        if (digitalRead(tlacidloCervene) && digitalRead(tlacidloModre))
        {
          digitalWrite(redDioda, HIGH);
          digitalWrite(blueDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nFiaolova  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloZelene))
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
        if (digitalRead(tlacidloZelene) && digitalRead(tlacidloModre))
        {
          digitalWrite(greenDioda, HIGH);
          digitalWrite(blueDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nTyrkysova  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloCervene))
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
        if (digitalRead(tlacidloZelene) && digitalRead(tlacidloCervene))
        {
          digitalWrite(greenDioda, HIGH);
          digitalWrite(redDioda, HIGH);
          pocitadloKol++;
          koloUkoncene = true;
          Serial.printf("\nZlta  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloModre))
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
        if (digitalRead(tlacidloZelene) && digitalRead(tlacidloCervene) && digitalRead(tlacidloModre))
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
      hotovo = true;
      Serial.printf("\nVyhra  dobre %d", pocitadloKol);
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Vyhral si");
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
    }
    u8x8.setCursor(0, 3);
    u8x8.print("                ");
    u8x8.setCursor(0, 3);
    u8x8.printf("%d/%d", pocitadloKol, maxKol);
  }
}

void tlieskanie()
{
  bool zap = false;
  int casovac = 0;
  int pocetTlesknuty = 0;
  while (!hotovo)
  {
    int hodnota = digitalRead(zvukPin);
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
        //Firebase.setBool(fireData, cesta + "Hotovo", "true");
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

void dotyk()
{
  const int prahovaHodnota = 30;
  while (!hotovo)
  {
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
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      hotovo = true;
    }
  }
}

void vzdialenost()
{
  int pozadovanaVzdialenost = 15;
  int casovac = 0;
  while (!hotovo)
  {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    int trvanie = pulseIn(echoPin, HIGH);
    int dlzka = trvanie * 0.034 / 2;

    u8x8.setFont(u8x8_font_inb33_3x6_n);
    u8x8.drawString(0, 2, u8x8_u16toa(dlzka, 4));
    if (dlzka > pozadovanaVzdialenost - 1 && dlzka < pozadovanaVzdialenost + 1)
    {
      casovac++;
      digitalWrite(greenDioda, HIGH);
      if (casovac > 100)
      {
        u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
        u8x8.drawString(0, 0, "VYHRA");
        //Firebase.setBool(fireData, cesta + "Hotovo", "true");
        hotovo = true;
        digitalWrite(greenDioda, LOW);
      }
    }
    else
    {
      digitalWrite(greenDioda, LOW);
      Serial.printf("Reset %d\n", casovac);
      casovac = 0;
    }
  }
}

void voda(){
  int hodnota = 0;
  int pozadovanaHodnota = 1000;
  int odchylka = 100;
  int casovac = 0;
  while(!hotovo){
    digitalWrite(vodnyPinVcc,HIGH);
    delay(10); 
    hodnota = dotykMeranie1.reading(analogRead(vodnyPin));
    digitalWrite(vodnyPinVcc,LOW);
    //u8x8.setFont(u8x8_font_inb33_3x6_n);
    //u8x8.drawString(0, 2, u8x8_u16toa(hodnota, 4));
    Serial.printf("Hodnota %d \n", hodnota);
    if(hodnota>pozadovanaHodnota-odchylka && hodnota < pozadovanaHodnota + odchylka){
      casovac++;
      digitalWrite(greenDioda, HIGH);
      digitalWrite(redDioda, LOW);
      if(casovac==50){
        hotovo=true;
        //Firebase.setBool(fireData, cesta + "Hotovo", "true");
        digitalWrite(greenDioda, LOW);
      }
    }else{
      casovac = 0;
      digitalWrite(greenDioda, LOW);
      digitalWrite(redDioda, HIGH);
    }
    delay(100);
  }
}

void LEDindetifikatorObsluha()
{
  if (LEDzmena)
  {
    digitalWrite(redDioda, LOW);
    digitalWrite(blueDioda, LOW);
    digitalWrite(greenDioda, LOW);
    if (LEDidentifikator == 0)
      digitalWrite(redDioda, HIGH);
    if (LEDidentifikator == 1)
      digitalWrite(greenDioda, HIGH);
    if (LEDidentifikator == 2)
      digitalWrite(blueDioda, HIGH);
    if (LEDidentifikator == 3)
    { //zlta
      digitalWrite(greenDioda, HIGH);
      digitalWrite(redDioda, HIGH);
    }
    if (LEDidentifikator == 4)
    { //fialova
      digitalWrite(redDioda, HIGH);
      digitalWrite(blueDioda, HIGH);
    }
    if (LEDidentifikator == 5)
    { //tyrkysova
      digitalWrite(greenDioda, HIGH);
      digitalWrite(blueDioda, HIGH);
    }
    if (LEDidentifikator == 6)
    { //biela
      digitalWrite(greenDioda, HIGH);
      digitalWrite(blueDioda, HIGH);
      digitalWrite(redDioda, HIGH);
    }
  }
  LEDzmena = false;
}

void loop()
{
  if (!zapnutaHra)
  {
    if (zmenaModrehoTlacidla)
    {
      y = y + 1;
      if (y > MAXMOZNOSTI - 1)
        y = 0;
      Firebase.setInt(fireData, cesta + "Volby", y);
      zmenaModrehoTlacidla = false;
    }
    //WIFI CAST
    Firebase.getJSON(fireData, cesta);
    FirebaseJson &json = fireData.jsonObject();
    FirebaseJsonData jsonData;
    json.get(jsonData, "Volby");
    if (jsonData.type == "int")
      y = jsonData.intValue;
    json.get(jsonData, "Start");
    if (jsonData.type == "bool")
      zapnutaHra = jsonData.boolValue;
    json.get(jsonData, "Online");
    if (jsonData.type == "bool")
      online = jsonData.boolValue;
    json.get(jsonData, "LED");
    if (jsonData.type == "int")
    {
      if (jsonData.intValue != LEDidentifikator)
        LEDzmena = true;
      LEDidentifikator = jsonData.intValue;
    }
    json.iteratorEnd();
    if (!online)
    {
      Firebase.setBool(fireData, cesta + "Online", "true");
      online = true;
    }
    //KONIEC WIFI CASTI
    LEDindetifikatorObsluha();
    if (x != y) // zapis na displej len pri zmene
    {
      x = y;
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 3);
      u8x8.print("                ");
      u8x8.setCursor(0, 3);
      u8x8.print(nazvyHier[x]);
      //u8x8.setCursor(0, 3);
    }

    if (digitalRead(tlacidloCervene) || zapnutaHra)
    {
      Firebase.setBool(fireData, cesta + "Start", "true");
      detachInterrupt(tlacidloModre);
      zapnutaHra = true;
      infoVypis = true;
      u8x8.clear();
      digitalWrite(redDioda, LOW);
      digitalWrite(blueDioda, LOW);
      digitalWrite(greenDioda, LOW);
    }
    //Serial.printf(".");

    //MENU
    if (zapnutaHra == true && y == 0)
      svetelnaBrana();
    if (zapnutaHra == true && y == 1)
      morseovka();
    if (zapnutaHra == true && y == 2)
      LEDHra();
    if (zapnutaHra == true && y == 3)
      miesanieFarieb();
    if (zapnutaHra == true && y == 4)
      tlieskanie();
    if (zapnutaHra == true && y == 5)
      dotyk();
    if (zapnutaHra == true && y == 6)
      vzdialenost();
    if (zapnutaHra == true && y == 7)
      voda();
  }
  if (hotovo && zapnutaHra)
  {
    if(infoVypis==true){
      Firebase.setBool(fireData, cesta + "Hotovo", "true");
      u8x8.clear();
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 0);
      u8x8.print("Hra bola uspesne");
      u8x8.setCursor(3, 2);
      u8x8.print("DOKONCENA");
      infoVypis=false;
    }
    digitalWrite(relePin, HIGH); //zopnutie rele
    Firebase.getJSON(fireData, cesta);
    FirebaseJson &json = fireData.jsonObject();
    FirebaseJsonData jsonData;
    json.get(jsonData, "Start");
    if (jsonData.type == "bool")
      zapnutaHra = jsonData.boolValue;
    json.iteratorEnd();
    if (!zapnutaHra)
    {
      hotovo = false;
      digitalWrite(relePin, LOW);
      Firebase.setBool(fireData, cesta + "Hotovo", "false");
      attachInterrupt(tlacidloModre, tlacidloModrePrerusenie, FALLING);
      x = 55;
      LEDzmena = true;
      u8x8.clear();
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 0);
      u8x8.print("MENU");
      u8x8.setCursor(0, 1);
      u8x8.print("~~~~~~~~~~~~~~~~");
      u8x8.setCursor(0, 6);
      u8x8.print("~~~~~~~~~~~~~~~~");
      u8x8.setCursor(0, 7);
      u8x8.print(ipPom);
    }
  }
}