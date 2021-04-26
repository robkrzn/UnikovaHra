#include "morseovka.h"

int rozpoznavacPrvku(int pocitadloZnaku)
{
    int znak = 2;//prazny znak
    if ((pocitadloZnaku < 3500) && (pocitadloZnaku >= 1))//podla dlzky znaku sa rozpozna ci ide o bodku alebo ciarku
    {
        Serial.printf(".");
        znak = 0;
    }
    else if ((pocitadloZnaku < 10000) && (pocitadloZnaku >= 3500))
    {
        Serial.printf("-");
        znak = 1;
    }
    return znak;//rozpoznany znak sa navrati
}

char dekodovanaMorseovka(int dlzkaPismena, int Znak[5])//podla bodiek a ciarok sa z tabulky vyberie prislusny znak
{
    char najdenePiseneno = 0;
    for (int i = 0; i < 36; i++)//cyklus prejde a porovna celu tabulku s ulozenymi znakmi
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
    return najdenePiseneno;//najdene pismeno sa vrati
}
bool overOdpoved(char odpoved[10])//porovnanie spravy s klucom odpovede
{
    if (odpoved[0] == 'F')
        if (odpoved[1] == 'R')
            if (odpoved[2] == 'I')
                if (odpoved[3] == 'U')
                    if (odpoved[4] == 'N')
                        if (odpoved[5] == 'I')
                            if (odpoved[6] == 'Z')
                                if (odpoved[7] == 'A')
                                    return true;//sprava je spravna
    return false;
}