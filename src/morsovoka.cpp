#include "morseovka.h"

int rozpoznavacPrvku(int pocitadloZnaku){
    int znak=2;
    if ((pocitadloZnaku < 15000) && (pocitadloZnaku >= 1))
      {
        Serial.printf(".");
        znak = 0;
      }
      else if ((pocitadloZnaku < 40000) && (pocitadloZnaku >= 15000))
      {
        Serial.printf("-");
        znak=1;
      }
      return znak;
}

char SDekodovanaMorseovka(int dlzkaPismena, int Znak[5])
{
    char najdenePiseneno = 0;
    for (int i = 0; i < 36; i++)
    {
        if (Znak[0] == MorseTabulka[i].morse[0])
        {
            if (Znak[1] == MorseTabulka[i].morse[1])
            {
                if (Znak[2] == MorseTabulka[i].morse[2])
                {
                    if (Znak[3] == MorseTabulka[i].morse[3])
                    {
                        if (Znak[4] == MorseTabulka[i].morse[4])
                        {
                            najdenePiseneno = MorseTabulka[i].pismeno;
                        }
                    }
                }
            }
        }
    }
    return najdenePiseneno;
}