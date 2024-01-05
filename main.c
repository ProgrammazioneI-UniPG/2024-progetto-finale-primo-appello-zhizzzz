#include "gamelib.h"
#include <stdio.h>
#include <stdlib.h>

extern void svuotaBuffer();
extern void color(char color);

int main( ) {
    system("clear"); // Pulizia schermo
    unsigned short scelta = 0;
    printf("Benvenuto in ");
    color('r'); printf("S");
    color('g'); printf("C");
    color('y'); printf("A");
    color('b'); printf("L");
    color('p'); printf("O");
    color('c'); printf("G");
    color('r'); printf("N");
    color('g'); printf("A ");
    color('y'); printf("Q");
    color('b'); printf("U");
    color('p'); printf("E");
    color('c'); printf("S");
    color('r'); printf("T");
    color('0'); printf("! (versione: 1.0) (data di pubblicazione: 05/01/2024)\n");

    do {
        printf("Menu principale: \n1. Imposta gioco\n2. Gioca\n3. Termina gioco.\n>> ");
        scanf("%hu", &scelta);
        svuotaBuffer();

        switch(scelta) {
            case 1:
                imposta_gioco();
                break;

            case 2:
                gioca();
                break;

            case 3:
                termina_gioco();
                break;

            default:
                printf("Il comando non riconosciuto!\n");
        }
        printf("\n");
    } while(scelta != 3);
}