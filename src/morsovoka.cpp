#include "morseovka.h"

int rozpoznavacPrvku(int pocitadloZnaku)
{
    int znak = 2;
    if ((pocitadloZnaku < 3500) && (pocitadloZnaku >= 1))
    {
        Serial.printf(".");
        znak = 0;
    }
    else if ((pocitadloZnaku < 10000) && (pocitadloZnaku >= 3500))
    {
        Serial.printf("-");
        znak = 1;
    }
    return znak;
}

char dekodovanaMorseovka(int dlzkaPismena, int Znak[5])
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
bool overOdpoved(char odpoved[10])
{
    if (odpoved[0] == 'F')
        if (odpoved[1] == 'R')
            if (odpoved[2] == 'I')
                if (odpoved[3] == 'U')
                    if (odpoved[4] == 'N')
                        if (odpoved[5] == 'I')
                            if (odpoved[6] == 'Z')
                                if (odpoved[7] == 'A')
                                    return true;
    return false;
}