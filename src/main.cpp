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

#define MERANIADOTYKUPREPRIEMER 5 //pocet merani z ktoreho sa bude pocitat priemer
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
const int redDioda = 25;
const int greenDioda = 33;
const int blueDioda = 32;

//globalne pomocne premenne

bool hotovo = false;
String ipPom; //premena s ulozenou IP adresou
bool infoVypis = false; //pomocna premenna pre konecny vystup

const int relePin = 26;

//HC-SR04
int trigPin = 17;
int echoPin = 16;

//vodny senzor
const int vodnyPin = 34;
const int vodnyPinVcc = 4;

//*****************************setup**********************************
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
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f); //vypis na displej
  u8x8.setCursor(6, 0);
  u8x8.print("MENU");
  u8x8.setCursor(0, 1);
  u8x8.print("~~~~~~~~~~~~~~~~");
}
//*****************************svetelnaBrana**********************************
void svetelnaBrana()
{
  int prahovaUroven = 100;  //pomocne premenne
  int hodnotaPhotorezistora;
  int pocetPocitadla = 10;
  int pocitadlo = 0;
  bool zapnute = false;
  u8x8.setFont(u8x8_font_inb33_3x6_n);  //nastavenie fontu displeja
  while (!hotovo)
  {
    hodnotaPhotorezistora = priemerSvetelnejBrany.reading(analogRead(photoresistorPin));
    //z odcitanej hodnoty sa vypocita plávajuci priemer
    if ((hodnotaPhotorezistora < prahovaUroven))//po prvom osvetelni sa hra zapne
      zapnute = true;
    if (zapnute && hodnotaPhotorezistora > prahovaUroven)//ak je luc preruseny zapocita sa to
    {
      pocitadlo++;
      zapnute = false; //kedze prerusenie môze trvať viac cyklov, opetovne sa zapocita az po novom osvetleni lucom
      Serial.printf("%d\n", pocitadlo);
      u8x8.drawString(0, 2, u8x8_u16toa(pocitadlo, 2)); //info vypis o hodnote
    }
    //u8x8.drawString(0, 2, u8x8_u16toa(hodnotaPhotorezistora, 4)); //info vypis o hodnote

    if (pocitadlo == pocetPocitadla) //po pozadovanom pocte preruseni sa hra vyhodnoti
    {
      pocitadlo = 0;
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      hotovo = true; //ukoncenie cyklu
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 1);
      u8x8.print("ULOHA SPLNENA");
    }
  }
}
//*****************************morseovka**********************************
void morseovka()
{
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f); //vypis na displej
  u8x8.setCursor(0, 0);
  u8x8.print("Text:");
  u8x8.setCursor(0, 3);
  char odpoved[10] = {0}; //pomocne premenne
  int indexOdpovede = 0;
  while (!hotovo)
  {
    bool pismenoHotovo = false;
    int dlzkaPismena = 0;
    int znak[5] = {2, 2, 2, 2, 2};  //prazdny znak, 2 sa pocita ako prazdne miesto
    int prahovaUroven = 350;  //prahova uroven 
    int pocitadloZnaku = 0; 
    int pocitadloMedzery = 0;
    while (pismenoHotovo == false)  //cyklus dokedy sa nezisti pismeno
    {
      pocitadloZnaku = 0;
      pocitadloMedzery = 0;

      if (priemerMerani.reading(analogRead(photoresistorPin)) < prahovaUroven)//po zisteni osvetelnia fotorezistora
      {
        while ((priemerMerani.reading(analogRead(photoresistorPin)) < prahovaUroven) && pocitadloZnaku < 10000)
        {
          pocitadloZnaku++;//zisti sa dlzka osvetlenia
        }
        delay(10);
        while ((priemerMerani.reading(analogRead(photoresistorPin)) >= prahovaUroven) && pocitadloMedzery < 15000)
        {
          pocitadloMedzery++;//zisti sa dlzka medzery
        }
        Serial.printf("Znak %d ", pocitadloZnaku);
        int pomZnak = rozpoznavacPrvku(pocitadloZnaku); //rozpoznavac znaku
        Serial.printf("\nMedzera %d\n", pocitadloMedzery);
        if (pomZnak != 2)//odpoved '2' je neuspesne zistenie znaku
        {
          znak[dlzkaPismena] = pomZnak;//do pola s odpovedou sa prida bodka alebo ciarka
          dlzkaPismena++;//dlzka znaku sa posunie
        }
        else
          pismenoHotovo = true;//ak by bola odpoved '2' jednalo by sa o zly znak a cele pismeno sa neda zistit
      }
      if (pocitadloMedzery > 5000)
        pismenoHotovo = 1;//po dlhej medzere je pismeno hotove
    }
    char vyslednyZnak = dekodovanaMorseovka(dlzkaPismena, znak);//z pola bodiek a ciarok sa ziska znak
    u8x8.print(vyslednyZnak);//vypis na displej
    Serial.printf(" %c\n------------\n", vyslednyZnak);
    odpoved[indexOdpovede] = vyslednyZnak;// znak sa zapise do pola s odpovedou
    indexOdpovede++;//posun v poli na dalsi znak
    if (indexOdpovede == 9)
      indexOdpovede = 0;//ak by odpoved mala viac ako 9 znakov ide sa odznova
    if (overOdpoved(odpoved)) //ak sa zhoduje text s vyhernym textom
    {
      Serial.printf("VYHRA");
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      u8x8.setCursor(0, 4);
      u8x8.print("ULOHA SPLNENA");
      hotovo = true;//ak sa zhoduje klucovy text s desifrovanym, hra je dokoncena
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
//*****************************LEDHra**********************************
void LEDHra()
{
  int pocitadloKol = 0;//pomocne premenne
  int maxKol = 5;
  while (hotovo == false)
  {
    delay(1000);
    bool prehra = false;
    int nahodneCislo = random(1, 4);//nahodne cislo rozhodne o riesenej farbe
    bool koloUkoncene = false;
    if (nahodneCislo == 1)
    {
      while (koloUkoncene == false)//spusti sa kolo 
      {
        digitalWrite(redDioda, HIGH);//rozsvieti sa pozadovana farba
        if (digitalRead(tlacidloCervene) == true)//akceptuje sa len cervene tlacidlo
        {
          digitalWrite(redDioda, LOW);
          pocitadloKol++;//zapocita sa pocet uspesnych kol
          koloUkoncene = true;//nastavi sa na uspesne dokoncene
          Serial.printf("\nCervena  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloModre) || digitalRead(tlacidloZelene))//ak zle tlacidlo
        {
          digitalWrite(redDioda, LOW);
          prehra = true;//nastavia sa premenne
          koloUkoncene = true;
          pocitadloKol = 0;//vynuluje sa pocitadlo
          Serial.printf("\nCervena  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 2)//rovnaky princip ako v pre kolo 1
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
    if (prehra == true)//pri prehre reset hry a ide sa znova
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
    if (pocitadloKol == maxKol)//dokoncena hra
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
//*****************************miesanieFarieb**********************************
void miesanieFarieb()
{
  int pocitadloKol = 0;
  int maxKol = 5;//pocet kol
  while (hotovo == false)
  {
    delay(1000);
    digitalWrite(greenDioda, LOW);//zhasnutie diod
    digitalWrite(redDioda, LOW);//zhasnutie diod
    digitalWrite(blueDioda, LOW);//zhasnutie diod
    bool prehra = false;//pomocne premenne
    int nahodneCislo = random(1, 5);//generator nahodne cisla
    bool koloUkoncene = false;
    if (nahodneCislo == 1)
    {
      u8x8.setCursor(0, 4);
      u8x8.print("                ");
      u8x8.setCursor(0, 4);
      u8x8.print("Fialova");//vypis na displej pozadovanu farbu
      while (koloUkoncene == false)//zaciatok kola
      {
        //overovanie stlačenia požadovanej kombinacie
        if (digitalRead(tlacidloCervene) && digitalRead(tlacidloModre))
        {
          digitalWrite(redDioda, HIGH);//rozvietenie kombinacie uzivatela
          digitalWrite(blueDioda, HIGH);
          pocitadloKol++;//zapocitanie spravnej odpovede
          koloUkoncene = true;//ukoncenie kola
          Serial.printf("\nFiaolova  dobre %d", pocitadloKol);
        }
        else if (digitalRead(tlacidloZelene))//pri zlej odpovedi
        {
          prehra = true;//nastavenie premennych
          koloUkoncene = true;
          pocitadloKol = 0;//vynulovanie uspechov
          Serial.printf("\nFiaolova  zle %d", pocitadloKol);
        }
      }
    }
    if (nahodneCislo == 2)//rovnaky princip ako pri cisle 1
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
    if (prehra == true)//informacia o prehre a reset hry
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
    if (pocitadloKol == maxKol)//hra uspesne dokoncena
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
//*****************************tlieskanie**********************************
void tlieskanie()
{
  int casovac = 0;
  int pocetTlesknuty = 0;
  while (!hotovo)
  {
    int hodnota = digitalRead(zvukPin);//odcitanie dat zo senzora
    if (hodnota == 1)//ak senzor ma log 1
    {
      pocetTlesknuty++;//zapocita sa pocet tliesknuti
      digitalWrite(greenDioda, true); //farebny efekt
      delay(100);
      digitalWrite(greenDioda, false);

      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 3);
      u8x8.printf("%d", pocetTlesknuty);//vypis na displej s poctom tlesknuti
    }
    if (pocetTlesknuty == 3)// ak je pozadovany pocet tlieskuti 
    {
      pocetTlesknuty = 0;//vynuluju sa pomocne premenne
      casovac = 0;
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      hotovo = true;//hra sa berie ako dokoncena
      
      delay(100);
    }
    if (pocetTlesknuty > 0)
    {
      casovac++;//ak je uz po prvom tliesknuti spusti sa casovac
    }
    if (casovac > 2000)//po 2 sekundach ak sa nestihne vykonat uloha, resetuje sa
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
    delay(1);//delay 1 milisekunda pre casovac
  }
}
//*****************************dotyk**********************************
void dotyk()
{
  const int prahovaHodnota = 30;// prahova uroven
  while (!hotovo)
  {
    int dotykHodnota1 = dotykMeranie1.reading(touchRead(dotykPin1));
    int dotykHodnota2 = dotykMeranie2.reading(touchRead(dotykPin2));
    //odcitanie hodnoty do plavajuceho priemeru
    if (dotykHodnota1 < prahovaHodnota)//ak je hodnota1 pod prahovou urovnou
      digitalWrite(redDioda, HIGH);//rozsvietenie info diody
    else
      digitalWrite(redDioda, LOW);//zhasnutie info diody
    if (dotykHodnota2 < prahovaHodnota)//ak je hodnota2 pod prahovou urovnou
      digitalWrite(greenDioda, HIGH);//rozsvietenie info diody
    else
      digitalWrite(greenDioda, LOW);//zhasnutie info diody
    //ak sa hrac naraz dotyka oboch vodicov
    if (dotykHodnota1 < prahovaHodnota && dotykHodnota2 < prahovaHodnota)
    {
      u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
      u8x8.drawString(0, 4, "VYHRA");
      //Firebase.setBool(fireData, cesta + "Hotovo", "true");
      hotovo = true;//hra sa povazuje za dokoncenu
    }
  }
}
//*****************************vzdialenost**********************************
void vzdialenost()
{
  int pozadovanaVzdialenost = 15;//pomocne premenne
  int casovac = 0;
  while (!hotovo)
  {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);//log 1 na 10 microsekund
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    int trvanie = pulseIn(echoPin, HIGH); //zmeranie odpovede 
    int dlzka = trvanie * 0.034 / 2;  //prepocet odpovede na cetimetre

    u8x8.setFont(u8x8_font_inb33_3x6_n);
    u8x8.drawString(0, 2, u8x8_u16toa(dlzka, 4));
    if (dlzka > pozadovanaVzdialenost - 1 && dlzka < pozadovanaVzdialenost + 1)//ak sedi namerany udaj
    {
      casovac++;//spusti sa casovac
      digitalWrite(greenDioda, HIGH);
      if (casovac > 80)//ak udaj sedi urcity cas
      {
        u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
        u8x8.drawString(0, 0, "VYHRA");
        //Firebase.setBool(fireData, cesta + "Hotovo", "true");
        hotovo = true;//hra je uspesne dokoncena
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
//*****************************voda**********************************
void voda(){
  int hodnota = 0;
  int pozadovanaHodnota = 1000;
  int odchylka = 100;
  int casovac = 0;
  while(!hotovo){
    digitalWrite(vodnyPinVcc,HIGH);//zapnutie napajania senzora 
    delay(10); 
    hodnota = dotykMeranie1.reading(analogRead(vodnyPin));
    //odcitanie analogovej hodnoty a prida sa do plavajuceho priemeru
    digitalWrite(vodnyPinVcc,LOW);//vypnutie napajania senzora 
    Serial.printf("Hodnota %d \n", hodnota);//vypis nameranej hodnoty
    if(hodnota>pozadovanaHodnota-odchylka && hodnota < pozadovanaHodnota + odchylka){
      casovac++;//pripocitanie hodnoty casovaca
      digitalWrite(greenDioda, HIGH);
      digitalWrite(redDioda, LOW);
      if(casovac==20){//ak sa vykona 20 cyklov casovaca cca 2 sekundy
        hotovo=true;//ukonci sa hra 
        //Firebase.setBool(fireData, cesta + "Hotovo", "true");
        digitalWrite(greenDioda, LOW);
      }
    }else{
      casovac = 0;//ak sa porusi hladina, casovac sa vynuluje
      digitalWrite(greenDioda, LOW);
      digitalWrite(redDioda, HIGH);
    }
    delay(100);
  }
}
//*****************************LEDidentfikatorObsluha**********************************
void LEDindetifikatorObsluha()
{//sluzi na nastavenie LED identifkacnej diody
  if (LEDzmena)//ak je zaznamenana zmena identifikacnej led diody
  {
    digitalWrite(redDioda, LOW);//zhasnutie vsetkych LED
    digitalWrite(blueDioda, LOW);
    digitalWrite(greenDioda, LOW);
    if (LEDidentifikator == 0)//podla prislusnej poziadavky sa nastavi farba
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
//*****************************loop**********************************
void loop()
{
  if (!zapnutaHra)//ak nieje spustene vykonavanie hry
  {
    if (zmenaModrehoTlacidla)//ak je zaznamenana zmena tlacidla z prerusenia
    {
      y = y + 1;  //pripocita sa moznost do menu
      if (y > MAXMOZNOSTI - 1)//ak je na konci menu tak sa nastavi na prvy prvok
        y = 0;
      Firebase.setInt(fireData, cesta + "Volby", y);  //nastaveny prvok sa odosle do databaze
      zmenaModrehoTlacidla = false; //zmena sa opat deaktivuje
    }
    //WIFI CAST
    Firebase.getJSON(fireData, cesta);//nacitane databazy
    FirebaseJson &json = fireData.jsonObject();//vytovrenie JSON objektu
    FirebaseJsonData jsonData;//vyvorenie dat
    json.get(jsonData, "Volby");//ziskanie info o volbe
    if (jsonData.type == "int")
      y = jsonData.intValue;//ulozenie volby
    json.get(jsonData, "Start");
    if (jsonData.type == "bool")
      zapnutaHra = jsonData.boolValue;
    json.get(jsonData, "Online");
    if (jsonData.type == "bool")
      online = jsonData.boolValue;
    json.get(jsonData, "LED");
    if (jsonData.type == "int")
    {
      if (jsonData.intValue != LEDidentifikator)//ak je nastavena ina led dioda 
        LEDzmena = true;//nastavi sa premenna pre zmenu
      LEDidentifikator = jsonData.intValue;//ulozi sa udaj
    }
    json.iteratorEnd();//ukonci sa praca s JSON datami
    if (!online)//ak nieje zaraidenie online
    {
      Firebase.setBool(fireData, cesta + "Online", "true");//nastavi sa ako online pre odpoved aplikacie
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

    if (digitalRead(tlacidloCervene) || zapnutaHra)//ak je stlacene tlacidlo pre start alebo aplikacia da prikaz
    {
      Firebase.setBool(fireData, cesta + "Start", "true");//zapise sa spustenie aj do databaze
      detachInterrupt(tlacidloModre);//vypne sa reakcia na prerusenie
      zapnutaHra = true;//nastavia sa premenne
      infoVypis = true;
      u8x8.clear();//vycisti sa plocha displeja
      digitalWrite(redDioda, LOW);//vypnu sa LEDdiody
      digitalWrite(blueDioda, LOW);
      digitalWrite(greenDioda, LOW);
    }
    //Serial.printf(".");

    //MENU
    if (zapnutaHra == true && y == 0)//podla volby sa spusti prislusna funkcia
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
  if (hotovo && zapnutaHra)//ak je hra dokoncena caka na reakciu uzivatela
  {
    if(infoVypis==true){//prvy info vypis na displej o dokoncenej hre
      Firebase.setBool(fireData, cesta + "Hotovo", "true");//zapis do databaze
      u8x8.clear();
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
      u8x8.setCursor(0, 0);
      u8x8.print("Hra bola uspesne");
      u8x8.setCursor(3, 2);
      u8x8.print("DOKONCENA");
      infoVypis=false;
      digitalWrite(relePin, HIGH); //zopnutie rele
    }
    
    Firebase.getJSON(fireData, cesta);//ziskanie dat z zadabaze pre opetovne pripravenie zariadenia
    FirebaseJson &json = fireData.jsonObject();
    FirebaseJsonData jsonData;
    json.get(jsonData, "Start");
    if (jsonData.type == "bool")
      zapnutaHra = jsonData.boolValue;
    json.iteratorEnd();
    if (!zapnutaHra)//ak je povel pre spustenie zariadenia
    {
      hotovo = false;//zariadenie sa da do zakladneho nastavenia
      digitalWrite(relePin, LOW);
      Firebase.setBool(fireData, cesta + "Hotovo", "false");
      attachInterrupt(tlacidloModre, tlacidloModrePrerusenie, FALLING);//zapnne sa reakcia na prerusenie
      x = 55;
      LEDzmena = true;
      u8x8.clear();//vypisu sa moznosti k menu na displej
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