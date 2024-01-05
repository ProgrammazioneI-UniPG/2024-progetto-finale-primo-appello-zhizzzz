#include "gamelib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Definizioni delle funzioni in gamelib.c.
// Piu altre funzioni di supporto.
// Le funzioni richiamabili in main.c non devono essere static.
// Le altre devono essere static (non visibili all'esterno).

int c; // Variabile di controllo input
void svuotaBuffer();
void color(char color);

//funzioni per imposta_gioco()
static void menu_impostazione_giocatori();
static void creazione_giocatori();
static void selezione_classe(Giocatore* giocatore);
static void stampa_giocatori();
static char* verifica_classe_giocatore(Giocatore* giocatore);
static void deallocazione_giocatori();
static void menu_impostazione_mappa();
static void genera_mappa(bool nuova_mappa);
static void cancella_mappa();
static void inserisci_zona();
static void cancella_zona();
static void stampa_mappa();
static char* verifica_tipo_zona(Zona_segrete* pZona);
static char* verifica_tipo_tesoro(Zona_segrete* pZona);
static char* verifica_tipo_porta(Zona_segrete* pZona);
static void chiudi_mappa();
static void menu_impostazione_tempo_pausa();

//funzioni per gioca()
static void genera_ordine_casuale(unsigned short ordine_giocatori[]);
static void avanza(Giocatore* giocatore, unsigned short* movimento);
static void indietreggia(Giocatore* giocatore, unsigned short* movimento);
static bool presenza_abitante_nella_zona_di(Giocatore* giocatore);
static Abitante_segrete* genera_abitante_segrete(unsigned short probabilita, Zona_segrete* zona);
static void deallocazione_abitante_segrete(Abitante_segrete* abitante_da_cancellare);
static void stampa_giocatore(Giocatore* giocatore);
static void stampa_zona(Giocatore* giocatore);
static void apri_porta(Giocatore* giocatore, unsigned short* azione);
static void toglie_punto_vita(Giocatore* giocatore);
static void genera_tesoro(Zona_segrete* zona);
static void prendi_tesoro(Giocatore* giocatore, unsigned short* azione);
static void scappa(Giocatore* giocatore, unsigned short* movimento);
static void combatti(Giocatore* giocatore, Abitante_segrete* abitante_da_combattere);
static unsigned short dado_con_pittogrammi(); 
static void attacco(Giocatore* giocatore, Abitante_segrete* abitante, bool attacco_di_giocatore);
static void gioca_potere_speciale(Giocatore* giocatore, unsigned short* azione);
static void investighi_zona(Giocatore* giocatore);
static void evento_speciale(Giocatore* giocatore);

bool impostato = false;
Giocatore* giocatori[4];
unsigned short num_giocatori = 0;
Zona_segrete* pFirst = NULL;
Zona_segrete* pLast = NULL;
unsigned short numero_zone_create = 0; //tenere conto del numero delle zone già create

unsigned short durata_intervallo = 2;
unsigned short turno = 1;
bool vincitore_comparso = false;
unsigned short num_giocatori_morti = 0;
Abitante_segrete* primo_abitante;
Abitante_segrete* ultimo_abitante;

void imposta_gioco() {
  if(impostato == false) {
    printf("Siccome questa è la prima volta che imposti il gioco, perciò ti chiediamo di innazitutto di creare i giocatori.\n");
    sleep(durata_intervallo);
    creazione_giocatori();
    printf("Adesso hai finito di impostare i giocatori, ora viene generata automaticamente la mappa del gioco\n");
    sleep(durata_intervallo);
    genera_mappa(true);
    menu_impostazione_mappa();
    printf("Hai già terminato la prima impostazione del gioco! Ora puoi procedere con normale impostazione.\n");
    sleep(durata_intervallo);
  }

  unsigned short scelta = 3;
  do {
    printf("\nIl menu dell'impostazione:\n");
    printf("\t0. Uscire dall'impostazione e tornare alla menu principale\n");
    printf("\t1. Vai all'impostazione dei giocatori\n");
    printf("\t2. Vai all'impostazione della mappa\n");
    printf("\t3. Vai all'impostazione del tempo di pausa del gioco\n");

    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();

    if(scelta == 1) {
      menu_impostazione_giocatori();
    } else if(scelta == 2) {
      menu_impostazione_mappa();
    } else if(scelta == 3) {
      menu_impostazione_tempo_pausa();
    } else if(scelta > 3) {
      printf("Il comando non riconosciuto!\n");
    }
  } while(scelta);
  printf("\n");
}

void gioca() {
  if(!impostato) {
    color('r');
    printf("Il gioco non è stato impostato, si prega di impostarlo prima di giocare!\n");
    color('0');
    return;
  }
  color('c');
  printf("Gioco iniziato!\n");
  color('0');

  for(int i = 0; i < num_giocatori; i++) { //portare tutti i giocatori al punto di partenza
    giocatori[i] -> posizione = pFirst;
  }
  
  //Memorizzare l'impostazione iniziale di giocatori e zone, per poter ripristinarlo al termine del gioco
  Giocatore* backup_giocatori[num_giocatori];
  for(int i = 0; i < num_giocatori; i++) {
    backup_giocatori[i] = (Giocatore*) (malloc(sizeof(Giocatore)));
    backup_giocatori[i] -> p_vita = giocatori[i] -> p_vita;
    backup_giocatori[i] -> dadi_attacco = giocatori[i] -> dadi_attacco;
    backup_giocatori[i] -> dadi_difesa = giocatori[i] -> dadi_difesa;
    backup_giocatori[i] -> mente = giocatori[i] -> mente;
    backup_giocatori[i] -> potere_speciale = giocatori[i] -> potere_speciale;
  }

  Tipo_tesoro tipo_tesoro[numero_zone_create];
  Tipo_porta tipo_porta[numero_zone_create];
  Zona_segrete* zona_segrete = pFirst;
  for(int i = 0; i < numero_zone_create; i++) {
    tipo_tesoro[i] = zona_segrete -> tipo_tesoro;
    tipo_porta[i] = zona_segrete -> tipo_porta;
    zona_segrete = zona_segrete -> zona_successiva;
  }

  //Iniziamo a giocare!
  unsigned short ordine_giocatori[num_giocatori];
  
  do {
    genera_ordine_casuale(ordine_giocatori);
    
    for(int i = 0; i < num_giocatori && !vincitore_comparso; i++) {
      color('c');
      printf("\nL'ordine di giocatori del %hu° turno:\n", turno);
      for(int j = 0; j < num_giocatori; j++) { //per stampare l'ordine dei giocatori di questo turno
        printf("\t%d. %s", (j+1), giocatori[ordine_giocatori[j]] -> nome_giocatore);
        if(giocatori[ordine_giocatori[j]] -> p_vita == 0) {
          printf(" (morto)");
        }
        if(i == j)
          printf(" << il suo turno");
        printf("\n");
      }

      color('r');
      if(giocatori[ordine_giocatori[i]] -> p_vita == 0) {
        printf("Siccome il giocatore %s è morto, si passa al turno del prossimo giocatore\n", giocatori[ordine_giocatori[i]] -> nome_giocatore);
        color('0');
        continue;
      }

      printf("Nota: Ogni giocatore può fare 3 azioni nel suo turno (prendere tesoro, combattere, aprire porta, utilizzare il potere specile), e può spostare al massimo una volta (avanzare, indietreggiare o scappare)\n");
      color('0');
      sleep(durata_intervallo);
    
      unsigned short scelta;
      unsigned short movimento = 0; //avanzare, indietreggiare, scappare vengono considerati movimento
      unsigned short azione = 0; //prendi tesoro, combatti, apri porta, gioca potere speciale vengono considerati azioni

      do {
        scelta = 11;
        color('c');
        printf("\nIl turno di %s (Movimento compiuto: %hu / Azioni compiuti: %hu):\n", giocatori[ordine_giocatori[i]] -> nome_giocatore, movimento, azione);
        color('0');
        printf("\t0. Passa il turno\n");
        if(movimento > 0)
          color('r');
        printf("\t1. Avanzare nella zona successiva\n");
        printf("\t2. Inditreggiare nella zona precedente\n");
        color('0');
        printf("\t3. Stampare le proprie informazioni \n");
        printf("\t4. Stampare le informazioni della zona attuale\n");
        if(azione >= 3)
          color('r');
        printf("\t5. Aprire la porta\n");
        printf("\t6. Prendere il tesoro in questa zona\n");
        color('0');
        if(movimento > 0)
          color('r');
        printf("\t7. Scappare\n");
        color('0');
        if(azione >= 3)
          color('r');
        printf("\t8. Combattere con abitante delle segrete in questa zona (con quello comparso più presto)\n");
        printf("\t9. Uccidere l'abitante delle segrete con potere speciale (uccidere quello comparso più presto)\n");
        color('0');
        printf("\t10. Investigare questa zona\n");
        printf("\t666. Suicidare (Se vuole riunciare il gioco)\n");

        printf(">> ");
        scanf("%hu", &scelta);
        svuotaBuffer();

        switch(scelta) {
          case 0:
            color('c');
            printf("Il giocatore %s ha terminato il suo turno\n", giocatori[ordine_giocatori[i]] -> nome_giocatore);
            color('0');
            break;
          case 1:
            avanza(giocatori[ordine_giocatori[i]], &movimento);
            break;
          case 2:
            indietreggia(giocatori[ordine_giocatori[i]], &movimento);
            break;
          case 3:
            stampa_giocatore(giocatori[ordine_giocatori[i]]);
            break;
          case 4:
            stampa_zona(giocatori[ordine_giocatori[i]]);
            break;
          case 5:
            apri_porta(giocatori[ordine_giocatori[i]], &azione);
            break;
          case 6:
            prendi_tesoro(giocatori[ordine_giocatori[i]], &azione);
            break;
          case 7:
            scappa(giocatori[ordine_giocatori[i]], &movimento);
            break;
          case 8:
            if(azione >= 3) {
              color('r');
              printf("Hai già compiuto 3 azioni in questo turno!\n");
              color('0');
            } else {
              combatti(giocatori[ordine_giocatori[i]], NULL);
              azione++;
            }
            break;
          case 9:
            gioca_potere_speciale(giocatori[ordine_giocatori[i]], &azione);
            break;
          case 10:
            if(giocatori[ordine_giocatori[i]] -> posizione -> evento_attivato == true) {
              investighi_zona(giocatori[ordine_giocatori[i]]);
            } else {
              printf("Ora non c'è niente di particolare in questa zona\n");
            }
            break;
          case 666:
            while(giocatori[ordine_giocatori[i]] -> p_vita > 0)
              toglie_punto_vita(giocatori[ordine_giocatori[i]]);
            break;
          default:
            color('r');
            printf("Il comando non riconosciuto!\n");
            color('0');
        }
        sleep(durata_intervallo);
      } while(scelta != 0 && giocatori[ordine_giocatori[i]] -> p_vita > 0 && !vincitore_comparso);
    }
    turno++;
    sleep(2);
  } while(!vincitore_comparso && num_giocatori != num_giocatori_morti);
  color('r');
  printf("Il gioco è terminato!\n");
  color('0');
  sleep(durata_intervallo);

  //Riportare i variabili nello stato iniziale, per assicurare il normale funzionamento della prossima partita
  for(int i = 0; i < num_giocatori; i++) {
    giocatori[i] -> p_vita = backup_giocatori[i] -> p_vita;
    giocatori[i] -> dadi_attacco = backup_giocatori[i] -> dadi_attacco;
    giocatori[i] -> dadi_difesa = backup_giocatori[i] -> dadi_difesa;
    giocatori[i] -> mente = backup_giocatori[i] -> mente;
    giocatori[i] -> potere_speciale = backup_giocatori[i] -> potere_speciale;
    free(backup_giocatori[i]);
    backup_giocatori[i] = NULL;
  }
  turno = 1;
  vincitore_comparso = false;
  num_giocatori_morti = 0;

  zona_segrete = pFirst;
  for(int i = 0; i < numero_zone_create; i++) {
    zona_segrete -> tipo_tesoro = tipo_tesoro[i];
    zona_segrete -> tipo_porta = tipo_porta[i];
    zona_segrete -> evento_attivato = true;
    zona_segrete = zona_segrete -> zona_successiva;
  }

  //Cancelliamo tutti gli abitanti rimasti nel campo
  if(primo_abitante != NULL) {
    Abitante_segrete* abitante_da_cancellare = primo_abitante;
    Abitante_segrete* prossimo_abitante_da_cancellare;
    do {
      prossimo_abitante_da_cancellare = abitante_da_cancellare -> abitante_successivo;
      free(abitante_da_cancellare);
      abitante_da_cancellare = prossimo_abitante_da_cancellare;
    } while(prossimo_abitante_da_cancellare != NULL);
    primo_abitante = ultimo_abitante = NULL;
  }
}

void termina_gioco() {
  printf("Grazie per aver giocato SCALOGNA QUEST!\n");
}

void svuotaBuffer() {
  while ((c = getchar()) != '\n' && c != EOF);
}

void color(char color) { //la funzione per modificare il colore del testo
  switch(color) {
    case 'r': //red
      printf("\033[0;91m");
      break;
    case 'g': //green
      printf("\033[0;92m");
      break;
    case 'y': //yellow
      printf("\033[0;93m");
      break;
    case 'b': //blue
      printf("\033[0;94m");
      break;
    case 'p': //purple
      printf("\033[0;95m");
      break;
    case 'c': //cyan
      printf("\033[0;96m");
      break;
    default: //0
      printf("\033[0m");
  }
}

static void menu_impostazione_giocatori() {
  unsigned short scelta = 3;

  do {
    printf("\nIl menu dell'impostazione dei giocatori:\n");
    printf("\t0. Uscire dall'impostazione e tornare alla menu precedente\n");
    printf("\t1. Stampare la lista dei giocatori\n");
    printf("\t2. Resettare i giocatori\n");

    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();

    if(scelta == 1) {
      stampa_giocatori();
    } else if(scelta == 2) {
      deallocazione_giocatori();
      sleep(durata_intervallo);
      creazione_giocatori();
    } else if(scelta > 2) {
      color('r');
      printf("Il comando non riconosciuto!\n");
      color('0');
    }
  } while(scelta);
}

static void creazione_giocatori() {
  do {
    printf("Inserisci il numero di giocatore(da 1 a 4): ");
    scanf("%hu", &num_giocatori);
    svuotaBuffer();
  } while(num_giocatori == 0 || num_giocatori > 4);

  for(int i = 0; i < num_giocatori; i++) {
      giocatori[i] = (Giocatore*) (malloc(sizeof(Giocatore)));
      printf("Ciao Giocatore %d! Qual'è il tuo nome? (Massimo 9 caratteri): ", i+1); //Chiedo di inserire il nome di giocatore
      do {
        fgets(giocatori[i] -> nome_giocatore, 10, stdin);
        giocatori[i] -> nome_giocatore[strcspn(giocatori[i] -> nome_giocatore, "\n")] = 0; // Per rimuovere il newline finale
        if (strcmp(giocatori[i] -> nome_giocatore, "") == 0)
          printf("Il nome non può essere vuoto!!! Si prega di inserire almeno una carattere:\n");
      } while(strcmp(giocatori[i] -> nome_giocatore, "") == 0);
      selezione_classe(giocatori[i]);
      printf("Il Giocatore %d impostato con successo!\n", i+1);
  }
}

static void selezione_classe(Giocatore* giocatore) {
  unsigned short scelta = 0;

  do {
    printf("Scegli la classe del tuo giocatore:\n");
    printf("  1. Barbaro: (Dadi attacco: 3 / Dadi difesa: 2 / Punti vita: 8; Mente: 1 o 2 / Potere speciale: 0)\n");
    printf("  2. Nano: (Dadi attacco: 2 / Dadi difesa: 2 / Punti vita: 7; Mente: 2 o 3 / Potere speciale: 1)\n");
    printf("  3. Elfo: (Dadi attacco: 2 / Dadi difesa: 2 / Punti vita: 6; Mente: 3 o 4 / Potere speciale: 1)\n");
    printf("  4. Mago: (Dadi attacco: 1 / Dadi difesa: 2 / Punti vita: 4; Mente: 4 o 5 / Potere speciale: 3)\n");

    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();
    
    time_t t; // Variabile per la generazione casuale
    srand((unsigned)time(&t));

    giocatore -> classe_giocatore = scelta - 1;
    if(scelta == 1) {
      giocatore -> dadi_attacco = 3;
      giocatore -> dadi_difesa = 2;
      giocatore -> p_vita = 8;
      giocatore -> mente = 1 + (rand() % 2);
      giocatore -> potere_speciale = 0;
    } else if(scelta == 2) {
      giocatore -> dadi_attacco = 2;
      giocatore -> dadi_difesa = 2;
      giocatore -> p_vita = 7;
      giocatore -> mente = 2 + (rand() % 2);
      giocatore -> potere_speciale = 1;
    } else if(scelta == 3) {
      giocatore -> dadi_attacco = 2;
      giocatore -> dadi_difesa = 2;
      giocatore -> p_vita = 6;
      giocatore -> mente = 3 + (rand() % 2);
      giocatore -> potere_speciale = 1;
    } else if (scelta == 4) {
      giocatore -> dadi_attacco = 1;
      giocatore -> dadi_difesa = 2;
      giocatore -> p_vita = 4;
      giocatore -> mente = 4 + (rand() % 2);
      giocatore -> potere_speciale = 3;
    } else {
      color('r');
      printf("Il comando non riconosciuto, si prega di rinserirlo:\n");
      color('0');
    }
  } while(scelta == 0 || scelta > 4);

  do {
    printf("Vuoi incrementare i propri punti di vita o mente? (punti vita: %hu / mente: %hu)\n", giocatore -> p_vita, giocatore -> mente);
    printf("  0. No, va bene cosi\n");
    printf("  1. Sacrificare un punto di vita per aumentare un punto di mente\n");
    printf("  2. Sacrificare un punto di mente per aumentare punto di vita\n");

    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();
    
    if(scelta == 1) {
      if(giocatore -> mente == 6) {
        printf("6 punti di mente è gia il massimo, quindi non può essere incrementato ulteriormente!\n");
      } else if(giocatore -> p_vita == 1) {
        printf("Ti serve almeno 1 punto di vita per poter giocare!\n");
      } else {
        giocatore -> p_vita--;
        giocatore -> mente++;
        printf("Hai sacrificato un punto di vita per incrementare un punto di mente\n");
      }
    } else if(scelta == 2) {
      if(giocatore -> mente == 0) {
        printf("Non hai punto di mente da sacrificare\n");
      } else {
        giocatore -> mente--;
        giocatore -> p_vita++;
        printf("Hai sacrificato un punto di mente per incrementare un punto di vita\n");
      }
    } else if(scelta != 0) {
      color('r');
      printf("Il comando non riconosciuto\n");
      color('0');
    }
    sleep(durata_intervallo);
  } while(scelta);
}

static void stampa_giocatori() {
  printf("Lista dei giocatori:\n");
  for(int i = 0; giocatori[i] != NULL; i++) {
    printf("%d°Giocatore:\n", i+1);
    printf("\tnome: %s\n", giocatori[i] -> nome_giocatore);
    printf("\tclasse: %s\n", verifica_classe_giocatore(giocatori[i]));
    printf("\tDadi attacco: %hu / Dadi difesa: %hu / Punti vita: %hu \n", giocatori[i] -> dadi_attacco, giocatori[i] -> dadi_difesa, giocatori[i] -> p_vita);
    printf("\tMente: %hu / Potere speciale: %hu\n", giocatori[i] -> mente, giocatori[i] -> potere_speciale);
  }
}

static char* verifica_classe_giocatore(Giocatore* giocatore) {
  switch (giocatore -> classe_giocatore) {
    case 0: return "Barbaro";
    case 1: return "Nano";
    case 2: return "Elfo";
    case 3: return "Mago";
  }
  return "";
}

static void deallocazione_giocatori() {
  printf("Ora l'impostazione precedente dei giocatori viene eliminato...\n");
  for(int i = 0; giocatori[i] != NULL; i++) {
    free(giocatori[i]);
    giocatori[i] = NULL;
  }
}

static void menu_impostazione_mappa() {
  unsigned short scelta = 10;

  do {
    printf("\nIl menu di impostazione della mappa:\n");
    printf("\t0. Uscire dall'impostazione della mappa\n");
    printf("\t1. Generare una nuova mappa (eliminando quella vecchia)\n");
    printf("\t2. Aggiungere 15 zone alla mappa già generata\n");
    printf("\t3. Inserire una zona in una determinata posizione della mappa\n");
    printf("\t4. Cancellare una zona in una determinata posizione della mappa\n");
    printf("\t5. Stampare la mappa\n");

    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();

    switch(scelta) {
      case 0:
        chiudi_mappa();
        if(!impostato) 
          scelta = 10;
        break;
      case 1:
        cancella_mappa();
        genera_mappa(true);
        break;
      case 2:
        genera_mappa(false);
        break;
      case 3:
        inserisci_zona();
        break;
      case 4:
        cancella_zona();
        break;
      case 5:
        stampa_mappa();
        break;
      default:
        color('r');
        printf("Il comando non riconosciuto\n");
        color('0');
    }
    sleep(durata_intervallo);
  } while(scelta);
}

static void genera_mappa(bool nuova_mappa) {
  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));

  Zona_segrete* nuova_zona_segrete = (Zona_segrete*) (malloc(sizeof(Zona_segrete))); //generiamo la prima zona
  nuova_zona_segrete -> zona_precedente = NULL;
  nuova_zona_segrete -> zona_successiva = NULL;

  if(nuova_mappa) {
    pFirst = nuova_zona_segrete; //allora diciamo che questa è la prima zona
  } else { //altrimenti vuol dire che l'utente vuole aggiungere altre 15 zone
    pLast -> zona_successiva = nuova_zona_segrete; //allora diciamo che questa zona appena generata è il successivo dell'ultima zona
  }
      
  for(int i = 0; i < 15; i++) {
    nuova_zona_segrete -> tipo_zona = rand() % 10;
    nuova_zona_segrete -> tipo_tesoro = rand() % 4;
    nuova_zona_segrete -> tipo_porta = rand() % 3;
    nuova_zona_segrete -> evento_attivato = true; //il campo per verificare lo stato di attivazione di evento speciale

    pLast = nuova_zona_segrete; //teniamo l'indirizzo della zona appena generato e processato in pLast

    if(i < 14) {
      nuova_zona_segrete = (Zona_segrete*) (malloc(sizeof(Zona_segrete))); //ora creiamo un'altra nuova zona
      pLast -> zona_successiva = nuova_zona_segrete; //diciamo alla zona generata precedentemente che la sua zona successiva è questa nuova
      nuova_zona_segrete -> zona_precedente = pLast; //e diciamo alla zona nuova che la sua precedente è pLast
      nuova_zona_segrete -> zona_successiva = NULL;
    }
  }

  if(nuova_mappa) {
    numero_zone_create = 15;
    printf("La mappa con 15 zone create con successo!\n");
  } else {
    numero_zone_create += 15;
    printf("Aggiunto 15 zone nuove alla mappa!\n");
  }
  sleep(durata_intervallo);
}

static void cancella_mappa() {
  Zona_segrete* zona_da_cancellare = pFirst;
  Zona_segrete* zona_successiva_da_cancellare;
  while(zona_da_cancellare != NULL) {
    zona_successiva_da_cancellare = zona_da_cancellare -> zona_successiva;
    free(zona_da_cancellare);
    zona_da_cancellare = zona_successiva_da_cancellare;
  }
  pFirst = pLast = NULL;
  printf("Mappa vecchia cancellata con successo\n");
}

static void inserisci_zona() {
  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));

  unsigned short posizione = 0;

  color('r');
  printf("Suggerimento: La prima posizione è 0; il numero delle zone nella mappa è %hu\n", numero_zone_create);
  color('0');
  do {
    printf("Inserisci la posizione che vuoi inserire la nuova zona (un numero minore o uguale di numero delle zone): ");
    scanf("%hu", &posizione);
    svuotaBuffer();
  } while (posizione > numero_zone_create);
  
  Zona_segrete* nuova_zona_segrete = (Zona_segrete*) (malloc(sizeof(Zona_segrete))); 
  nuova_zona_segrete -> tipo_zona = rand() % 10;
  nuova_zona_segrete -> tipo_tesoro = rand() % 4;
  nuova_zona_segrete -> tipo_porta = rand() % 3;
  nuova_zona_segrete -> evento_attivato = true;
  nuova_zona_segrete -> zona_precedente = NULL;
  nuova_zona_segrete -> zona_successiva = NULL;

  if(numero_zone_create == 0) { //Se non ci fosse nessuna zona nella mappa
    pFirst = pLast = nuova_zona_segrete;
  } else if(numero_zone_create == 1 && posizione == 1) { //Se c'è l'unica zona nella mappa
    pFirst -> zona_successiva = nuova_zona_segrete;
    nuova_zona_segrete -> zona_precedente = pFirst;
    pLast = nuova_zona_segrete;
  } else if(posizione == 0) { //Se vogliamo inserire nella prima posizione
    nuova_zona_segrete -> zona_successiva = pFirst; //la successiva della zona nuova è prima zona
    pFirst -> zona_precedente = nuova_zona_segrete; //la precedente della prima zona è zona nuova
    pFirst = nuova_zona_segrete; //ora la prima zona è quella nuova
  } else if(posizione < numero_zone_create) { //Se vogliamo inserire una in mezzo
    Zona_segrete* zona_precedente_della_zona_da_inserire = pFirst;
    bool inserito = false;
    for(int i = 1; posizione < numero_zone_create && !inserito; i++) {
      if(i == posizione) {
        //Immaginiamo che vogliamo insirire una nuova zona nella posizione 1
        nuova_zona_segrete -> zona_precedente = zona_precedente_della_zona_da_inserire; //la sua precedente è zona 0
        nuova_zona_segrete -> zona_successiva = zona_precedente_della_zona_da_inserire -> zona_successiva; //la sua successiva è vecchia zona 1
        zona_precedente_della_zona_da_inserire -> zona_successiva = nuova_zona_segrete; //ora la zona successiva di zona 0 è questa nuova
        nuova_zona_segrete -> zona_successiva -> zona_precedente = nuova_zona_segrete; //e la precedente di vecchia zona 1 è questa nuova
        inserito = true;
      } else { //se non è la posizione che vogliamo
        //allora scorriamo di una posizione nella lista delle zone
        zona_precedente_della_zona_da_inserire = zona_precedente_della_zona_da_inserire -> zona_successiva;
      }
    }
  } else if(posizione == numero_zone_create) { //se vogliomo aggiungere una zona esattamente nell'ultima zona
    nuova_zona_segrete -> zona_precedente = pLast;
    pLast -> zona_successiva = nuova_zona_segrete;
    pLast = nuova_zona_segrete;
  }
  numero_zone_create++;
  printf("Una nuova zona viene inserita nella posizione %hu con successo! Ora la mappa contiene %hu zone\n", posizione, numero_zone_create);
}

static void cancella_zona() {
  if(numero_zone_create == 0) {
    color('r');
    printf("Non ci sono le zone da cancellare nel mappa!\n");
    color('0');
    return;
  }
  unsigned short posizione = 0;
  
  color('r');
  printf("Suggerimento: La prima posizione è 0; il numero delle zone nella mappa è %hu\n", numero_zone_create);
  color('0');
  do {
    printf("Inserisci la posizione della zona che vuoi cancellare (un numero minore di numero delle zone): ");
    scanf("%hu", &posizione);
    svuotaBuffer();
  } while(posizione >= numero_zone_create);

  if(numero_zone_create == 1) {

    free(pFirst);
    pFirst = pLast = NULL;

  } else if(posizione == 0) { //Se vogliamo cancellare la prima zona

    Zona_segrete* zona_da_modificare = pFirst -> zona_successiva;
    zona_da_modificare -> zona_precedente = NULL;
    free(pFirst);
    pFirst = zona_da_modificare;

  } else if(posizione < numero_zone_create - 1) { //Se vogliamo cancellare una in mezzo
    Zona_segrete* zona_da_cancellare = pFirst -> zona_successiva;

    for(int i = 1; (posizione < numero_zone_create - 1) && (zona_da_cancellare != NULL); i++) {
      if(i == posizione) {
        /* Per spiegare il seguente blocco di codice, facciamo un esempio:
            Ipotizziamo che vogliomo cancellare la zona nella posizione 1
            zona_da_cancellare = 1
            la posizione della zona precedente della zona da cancellare è 0
            la posizione della zona successiva della zona da cancellare è 2 */
        zona_da_cancellare -> zona_precedente -> zona_successiva = zona_da_cancellare -> zona_successiva; //diciamo alla zona 0 che la zona successiva è 2
        zona_da_cancellare -> zona_successiva -> zona_precedente = zona_da_cancellare -> zona_precedente; //diciamo alla zona 2 che la zona precedente è 0
        free(zona_da_cancellare); //ora canceliamo la zona 1
        zona_da_cancellare = NULL;
      } else { //se non è la posizione che vogliamo
        zona_da_cancellare = zona_da_cancellare -> zona_successiva; //allora scorriamo di una posizione nell lista delle zone
      }
    }
  } else if(posizione == (numero_zone_create - 1)) { //Se vogliamo cancellare ultima zona
    Zona_segrete* pTemp = pLast -> zona_precedente;
    pTemp -> zona_successiva = NULL;
    free(pLast);
    pLast = pTemp;
  }
  numero_zone_create--;
  printf("La zona nella posizione %hu viene cancellato con successo! Ora la mappa contiene %hu zone", posizione, numero_zone_create);
}

static void stampa_mappa() {
  if(numero_zone_create == 0) {
    printf("La mappa è vuota!\n");
    return;
  }
  Zona_segrete* zona_da_stampare = pFirst;
  unsigned short posizione = 0;
  
  do {
    printf("La %hu° zona: %s, %s, %s \n", posizione, verifica_tipo_zona(zona_da_stampare), verifica_tipo_tesoro(zona_da_stampare), verifica_tipo_porta(zona_da_stampare));
    zona_da_stampare = zona_da_stampare -> zona_successiva;
    posizione++;
  } while(zona_da_stampare != NULL);
}

static char* verifica_tipo_zona(Zona_segrete* pZona) {
  switch (pZona -> tipo_zona) {
    case 0: return "corridoio";
    case 1: return "scala";
    case 2: return "sala_banchetto";
    case 3: return "magazzino";
    case 4: return "giardino";
    case 5: return "posto_guardia";
    case 6: return "prigione";
    case 7: return "cucina";
    case 8: return "armeria";
    case 9: return "tempio";
  }
  return "";
}

static char* verifica_tipo_tesoro(Zona_segrete* pZona) {
  switch (pZona -> tipo_tesoro) {
    case 0: return "nessun_tesoro";
    case 1: return "veleno";
    case 2: return "guarigione";
    case 3: return "doppia_guarigione";
  }
  return "";
}

static char* verifica_tipo_porta(Zona_segrete* pZona) {
  switch (pZona -> tipo_porta) {
    case 0: return "nessuna_porta";
    case 1: return "porta_normale";
    case 2: return "porta_da_scassinare";
  }
  return "";
}

static void chiudi_mappa() {
  if(numero_zone_create < 15) {
    printf("La mappa contiene %hu zone, che non sono sufficienti per giocare (almeno 15 zone), perciò tornerai alla menu dell'impostazione della mappa...", numero_zone_create);
  } else {
    impostato = true;
  }
}

static void menu_impostazione_tempo_pausa() { //permettere all'utente di impostare la velocità di apparizione del testo
  unsigned short scelta = 4;
  do {
    printf("\nDurata di intervallo del gioco: %hu secondi\n", durata_intervallo);
    printf("\t0. Uscire dall'impostazione del tempo di pausa\n");
    printf("\t1. Aumentare di 1 secondo\n");
    printf("\t2. Diminuire di 1 secondo\n");
    printf("\t3. Resettare al tempo di default (2 secondi)\n");

    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();

    if(scelta == 1) {
      if(durata_intervallo < 5) {
        durata_intervallo++;
      } else { 
        color('r');
        printf("La durata massima di intervallo del gioco è 5 secondi! Perciò è impossibile aumentare di più\n");
        color('0');
      }
    } else if(scelta == 2) {
      if(durata_intervallo > 0) {
        durata_intervallo--;
      } else { 
        color('r');
        printf("La durata minima di intervallo del gioco è 0 secondi! Perciò è impossibile diminuire di più\n");
        color('0');
      }
    } else if(scelta == 3) {
      durata_intervallo = 2;
    } else if(scelta != 0) {
      color('r');
      printf("Il comando non riconosciuto!\n");
      color('0');
    }
  } while(scelta);
}

static void genera_ordine_casuale(unsigned short ordine_giocatori[]) {
  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));
  
  ordine_giocatori[0] = rand() % num_giocatori;
  for(int i = 1; i < num_giocatori;i++) {
    ordine_giocatori[i] = rand() % num_giocatori;
    for(int j = i - 1; j >= 0; j--) {
      if(ordine_giocatori[i] == ordine_giocatori[j]) {
        i--;
        break;
      }
    }
  }
}

static void avanza(Giocatore* giocatore, unsigned short* movimento) {
  if(*movimento > 0) {
    color('r');
    printf("Hai già spostato una volta in questo turno!\n");
    color('0');
  } else if(giocatore -> posizione -> tipo_porta == porta_normale){
    printf("Hai una porta da aprire\n");
  } else if(giocatore -> posizione -> tipo_porta == porta_da_scassinare) {
    printf("Hai una porta da scassinare\n");
  } else if(presenza_abitante_nella_zona_di(giocatore)) {
    color('r');
    printf("Non è possibile avanzare prima di aver combattuto tutti gli abitanti in questa zona\n");
    color('0');
  } else if(giocatore -> posizione -> zona_successiva == NULL) {
    vincitore_comparso = true;
    color('g');
    printf("Il giocatore %s ha raggiunto la fine!!! Il vincitore è %s!!!!!\n", giocatore -> nome_giocatore, giocatore -> nome_giocatore);
    color('0');
  } else if(giocatore -> posizione -> tipo_porta == nessuna_porta) {
    giocatore -> posizione = giocatore -> posizione -> zona_successiva;
    genera_tesoro(giocatore -> posizione);
    color('y');
    printf("Ti sei spostato nella zona successiva\n");
    color('0');
    genera_abitante_segrete(33, giocatore -> posizione);
    *movimento = *movimento + 1; //teniamo conto del fatto che il giocatore ha già spostato una volta in questo turno
  }
}

static void indietreggia(Giocatore* giocatore, unsigned short* movimento) {
  if(giocatore -> posizione == pFirst) { //Se il giocatore sta nel punto di partenza
    color('r');
    printf("Stai nel punto di partenza, quindi è impossibile indietreggiare!\n");
    color('0');
  } else if(presenza_abitante_nella_zona_di(giocatore)) {
    color('r');
    printf("Non è possibile indietreggiare prima di aver combattuto tutti gli abitanti in questa zona!\n");
    color('0');
  } else if(*movimento > 0) {
    color('r');
    printf("Hai già spostato una volta in questo turno!\n");
    color('0');
  } else{
    giocatore -> posizione = giocatore -> posizione -> zona_precedente;
    genera_tesoro(giocatore -> posizione);
    color('y');
    printf("Ti sei spostato nella zona precedente\n");
    color('0');
    genera_abitante_segrete(33, giocatore -> posizione);
    *movimento = *movimento + 1;
  }
}

static bool presenza_abitante_nella_zona_di(Giocatore* giocatore) {
  Abitante_segrete* abitante_da_verificare = primo_abitante;
  while(abitante_da_verificare != NULL) {
    if(abitante_da_verificare -> posizione == giocatore -> posizione) { 
      return true;
    }
    abitante_da_verificare = abitante_da_verificare -> abitante_successivo;
  }
  return false;
}

static Abitante_segrete* genera_abitante_segrete(unsigned short probabilita, Zona_segrete* zona) {
  //Primo condizione di primo if, perchè nell'ultima stanza deve apparirà sempre un abitante
  //Per quanto riguarda il resto delle condizioni da testare
  //E' perché lo studente ha deciso di cambiare la probabilità di apparizione di abitante in determinate zone
  //Nelle ultime 3 if, abbiamo anche verificato se la probabilita diverso da 100
  //perché nella funzione apri_porta(), quando giocatore fallisce a scassinare
  //con 50% perde un punto vita, 10% viene mandato al punto di partenza, 40% genera un nuova abitante
  //nell'ultimo caso, viene invocato la funzione genera_abitante_segrete() passando la probabilita = 100
  Tipo_zona tipo_zona = zona -> tipo_zona;
  if(zona == pLast || tipo_zona == prigione)
    probabilita = 100; 
  else if(tipo_zona == corridoio && probabilita != 100)
    probabilita = 0;
  else if(tipo_zona == scala && probabilita != 100)
    probabilita = 16;
  else if(tipo_zona == sala_banchetto && probabilita != 100)
    probabilita = 66;

  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));
  unsigned short numero_casuale = rand() % 100;
  Abitante_segrete* nuovo_abitante_segrete;

  if(numero_casuale < probabilita) {
    nuovo_abitante_segrete = (Abitante_segrete*) (malloc(sizeof(Abitante_segrete)));
    numero_casuale = rand() % 4;
    //Goblin (2, 1, 1) Skeletro (2, 1, 2) Stragone del caos (3, 2, 2) Doccione (3, 3, 2)
    if(numero_casuale == 0) {
      strcpy(nuovo_abitante_segrete -> nome_abitante_segrete, "Goblin");
      nuovo_abitante_segrete -> dadi_attacco = 2;
      nuovo_abitante_segrete -> dadi_difesa = 1;
      nuovo_abitante_segrete -> p_vita = 1;
    } else if(numero_casuale == 1) {
      strcpy(nuovo_abitante_segrete -> nome_abitante_segrete, "Skeletro");
      nuovo_abitante_segrete -> dadi_attacco = 2;
      nuovo_abitante_segrete -> dadi_difesa = 1;
      nuovo_abitante_segrete -> p_vita = 2;
    } else if(numero_casuale == 2) {
      strcpy(nuovo_abitante_segrete -> nome_abitante_segrete, "Stragone del caos");
      nuovo_abitante_segrete -> dadi_attacco = 3;
      nuovo_abitante_segrete -> dadi_difesa = 2;
      nuovo_abitante_segrete -> p_vita = 2;
    } else if(numero_casuale == 3) {
      strcpy(nuovo_abitante_segrete -> nome_abitante_segrete, "Doccione");
      nuovo_abitante_segrete -> dadi_attacco = 3;
      nuovo_abitante_segrete -> dadi_difesa = 3;
      nuovo_abitante_segrete -> p_vita = 2;
    }
    nuovo_abitante_segrete -> posizione = zona;
    color('p');
    printf("È comparso un nuovo abitante delle segrete nella tua zona!\n");
    printf("Nome: %s\n", nuovo_abitante_segrete ->nome_abitante_segrete);
    printf("Dadi attacco: %hu / Dadi difesa: %hu / Punti vita: %hu\n", nuovo_abitante_segrete -> dadi_attacco, nuovo_abitante_segrete -> dadi_difesa, nuovo_abitante_segrete -> p_vita);
    color('0');

    //mettere abitante appena generato nella lista collegata
    if(primo_abitante == NULL)
      primo_abitante = nuovo_abitante_segrete;
    else 
      ultimo_abitante -> abitante_successivo = nuovo_abitante_segrete;
  
    ultimo_abitante = nuovo_abitante_segrete;
    nuovo_abitante_segrete -> abitante_successivo = NULL;
  }
  sleep(durata_intervallo);
  return nuovo_abitante_segrete;
}

static void deallocazione_abitante_segrete(Abitante_segrete* abitante_da_cancellare) {
  if(primo_abitante == ultimo_abitante) { //Se ora c'è un solo abitante nella lista
    free(abitante_da_cancellare);
    primo_abitante = ultimo_abitante = NULL;
  } else if(abitante_da_cancellare == primo_abitante){ //Se fosse primo
    primo_abitante = abitante_da_cancellare -> abitante_successivo;
    free(abitante_da_cancellare);
  } else if(abitante_da_cancellare == ultimo_abitante) { //Se fosse ultimo
  
    Abitante_segrete* abitante_precedente_di_abitante_da_cancellare = primo_abitante;
    do {
      if(abitante_precedente_di_abitante_da_cancellare -> abitante_successivo == abitante_da_cancellare) {
        ultimo_abitante = abitante_precedente_di_abitante_da_cancellare;
        ultimo_abitante -> abitante_successivo = NULL;
        free(abitante_da_cancellare);
        abitante_da_cancellare = NULL;
      } else {
        abitante_precedente_di_abitante_da_cancellare = abitante_precedente_di_abitante_da_cancellare -> abitante_successivo;
      }
    } while(abitante_da_cancellare != NULL);

  } else if(primo_abitante != NULL) { //Altrimetri se la lista non fosse vuota, allora è un abitante che sta si trova in mezzo della lista
    Abitante_segrete* abitante_precedente_di_abitante_da_cancellare = primo_abitante;

    do {
      if(abitante_precedente_di_abitante_da_cancellare -> abitante_successivo == abitante_da_cancellare) {
        abitante_precedente_di_abitante_da_cancellare -> abitante_successivo = abitante_da_cancellare -> abitante_successivo;
        free(abitante_da_cancellare);
        abitante_da_cancellare = NULL;
      } else {
        abitante_precedente_di_abitante_da_cancellare = abitante_precedente_di_abitante_da_cancellare -> abitante_successivo;
      }
    } while(abitante_da_cancellare != NULL);
  }
  
}

static void stampa_giocatore(Giocatore* giocatore) {
  printf("Giocatore %s:\n", giocatore -> nome_giocatore);
  printf("\tClasse: %s\n", verifica_classe_giocatore(giocatore));
  printf("\tDadi attacco: %hu / Dadi difesa: %hu / Punti vita: %hu\n", giocatore -> dadi_attacco, giocatore-> dadi_difesa, giocatore -> p_vita);
  printf("\tMente: %hu / Potere speciale: %hu\n", giocatore -> mente, giocatore -> potere_speciale);
}

static void stampa_zona(Giocatore* giocatore) {
  Zona_segrete* zona_da_stampare = pFirst;
  for(int i = 0; i < numero_zone_create; i++) {
    if(zona_da_stampare == (giocatore -> posizione)) {
      printf("Il giocatore %s si trova nella %d° zona: %s,", giocatore -> nome_giocatore, i, verifica_tipo_zona(zona_da_stampare));
      if(zona_da_stampare -> tipo_tesoro == nessun_tesoro)
        printf(" non c'è tesoro,");
      else
        printf(" c'è tesoro,");
      if(zona_da_stampare -> tipo_porta == nessuna_porta)
        printf(" non c'è porta da aprire\n");
      else 
        printf(" c'è porta da aprire/scassinare\n");

      if(primo_abitante != NULL) {
        printf("Abitante delle segrete in questa zona:");
        Abitante_segrete* abitante_da_stampare = primo_abitante;
        while(abitante_da_stampare != NULL) {
          if(abitante_da_stampare -> posizione == giocatore -> posizione)
            printf(" %s  ", abitante_da_stampare -> nome_abitante_segrete);
          abitante_da_stampare = abitante_da_stampare -> abitante_successivo;
        }
        printf("\n");
      }
      sleep(durata_intervallo);
      break;

    } else {

      zona_da_stampare = zona_da_stampare -> zona_successiva;

    }
  }
}

//preferisco chiamare questa funzione distruggi_porta()
//perché di fatto nessuna_porta e porta_normale non sembrano di avere qualche differenza
//nessuna delle due impedisce l'avanzamento del giocatore
static void apri_porta(Giocatore* giocatore, unsigned short* azione) {
  if(giocatore -> posizione -> tipo_porta == nessuna_porta) {
    printf("Non c'è porta da aprire in questa zona\n");
  } else if(*azione >= 3) {
    color('r');
    printf("Hai già compiuto 3 azioni in questo turno!\n");
    color('0');
  } else if(giocatore -> posizione -> tipo_porta == porta_normale) {
    giocatore -> posizione -> tipo_porta = nessuna_porta;
    color('g');
    printf("Hai aperto e distrutto la porta\n");
    *azione = *azione + 1;
  } else if(giocatore -> posizione -> tipo_porta == porta_da_scassinare) {
    printf("Ora inizi a scassinare la porta. Premere l'invio per lanciare il dado...\n");
    while (getchar() != '\n');

    time_t t; // Variabile per la generazione casuale
    srand((unsigned)time(&t));
    unsigned short dado = (rand() % 6) + 1;
    printf("Dado lanciato = %hu \n", dado);
    printf("Mente del giocatore = %hu\n", giocatore -> mente);

    if(dado <= giocatore -> mente) {
      giocatore -> posizione -> tipo_porta = nessuna_porta;
      color('g');
      printf("Porta è stato scassinato e distrutto con successo!\n");
    } else {
      color('p');
      unsigned short numero_casuale = rand() % 10;
      if(numero_casuale == 0) {
        giocatore -> posizione = pFirst;
        printf("Porta non è stato scassinato con successo, e sei stato mandato al punto di partenza\n");
      } else if(numero_casuale < 6) {
        printf("Porta non è stato scassinato con successo\n");
        toglie_punto_vita(giocatore);
      } else {
        printf("Porta non è stato scassinato con successo, e ha causato l'apparizione di un nuovo abitante delle segrete\n");
        genera_abitante_segrete(100, giocatore -> posizione);
      }
    }
    *azione = *azione + 1;
  }
  color('0');
}

static void toglie_punto_vita(Giocatore* giocatore) {
  color('p');
  giocatore -> p_vita --;
  if(giocatore -> p_vita == 0) {
    num_giocatori_morti++;
    printf("Giocatore %s è morto!\n", giocatore -> nome_giocatore);
  } else {
    printf("Giocatore %s è stato tolto un punto di vita, punti di vita rimanenti: %hu\n", giocatore -> nome_giocatore, giocatore -> p_vita);
  }
  color('0');
  sleep(durata_intervallo);
}

static void genera_tesoro(Zona_segrete* zona) {
  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));
  if(zona -> tipo_tesoro == nessun_tesoro)
    zona -> tipo_tesoro = rand() % 4;
}

static void prendi_tesoro(Giocatore* giocatore, unsigned short* azione) {
  if(giocatore -> posizione -> tipo_tesoro == nessun_tesoro) {
    printf("Non c'è nessun tesoro da prendere in questa zona!\n");
  } else if(*azione >= 3) {
    color('r');
    printf("Hai già compiuto 3 azioni in questo turno!\n");
    color('0');
  } else if(giocatore -> posizione -> tipo_tesoro == veleno) {
    color('p');
    printf("Hai ottenuto un veleno!\n");
    sleep(durata_intervallo);
    for(int i = 0; i < 2; i++) {
      if(giocatore -> p_vita > 0)
        toglie_punto_vita(giocatore);
    }
    *azione = *azione + 1;
  } else if(giocatore -> posizione -> tipo_tesoro == guarigione) {
    giocatore -> p_vita++;
    color('g');
    printf("Hai ottenuto una guarigione! (Punti vita = %hu)\n", giocatore -> p_vita);
    *azione = *azione + 1;
  } else if(giocatore -> posizione -> tipo_tesoro == doppia_guarigione) {
    giocatore -> p_vita += 2;
    color('g');
    printf("Hai ottenuto una doppia guarigione! (Punti vita = %hu)\n", giocatore -> p_vita);
    *azione = *azione + 1;
  }
  giocatore -> posizione -> tipo_tesoro = nessun_tesoro;
  color('0');
}

static void scappa(Giocatore* giocatore, unsigned short* movimento) {
  color('r');
  if(giocatore -> posizione -> zona_precedente == NULL) {
    printf("Stai già nel punto di partenza, quindi non c'è zona precedente dove puoi scappare!\n");
    color('0');
    return;
  }

  if(!presenza_abitante_nella_zona_di(giocatore)) {
    printf("Non è neccessario scappare perché non ci sono gli abitanti in questa zona\n");
    color('0');
    return;
  }

  if(movimento > 0) {
    printf("Hai già spostato una volta in questo turno!\n");
    color('0');
    return;
  }
  color('0');

  printf("Ora inizi a lanciare i dadi per scappare. Premere l'invio per lanciare...\n");
  while (getchar() != '\n');
  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));
  unsigned short dado = (rand() % 6) +1;
  sleep(durata_intervallo);
  printf("Dado lanciato = %hu\n", dado);
  sleep(durata_intervallo);
  printf("Mente del giocatore = %hu\n", giocatore -> mente);
  sleep(durata_intervallo);

  if(dado <= giocatore -> mente) {
    giocatore -> posizione = giocatore -> posizione -> zona_precedente;
    color('g');
    printf("Sei riuscito a scappare ");
    color('y');
    printf("alla zona precedente!\n");
    color('0');
    *movimento = *movimento + 1;
  } else {
    color('p');
    printf("Non sei riuscito a scappare! Ora devi lanciare i tuoi dadi di difesa per difendere! Premere l'invio per lanciare i dadi...");
    color('0');
    sleep(durata_intervallo);
    while (getchar() != '\n');
    unsigned short parato = 0;
    for(int i = 0; i < (giocatore -> dadi_difesa / 2); i++) {
      if(dado_con_pittogrammi() == 1)
        parato++;
    }
    if(parato > 0) {
      color('g');
      printf("Hai evitato di subire il danno\n");
      color('0');
    } else {
      toglie_punto_vita(giocatore);
    }
  }
}

static void combatti(Giocatore* giocatore, Abitante_segrete* abitante_da_combattere) {
  color('b');
  
  if(abitante_da_combattere == NULL) { //Se chi ha invocato questa funzione non ha specificato quale abitente vuole combattere
    //allora combatterà con abitante generato più presto in questa zona
    abitante_da_combattere = primo_abitante;
    while(abitante_da_combattere != NULL) {
      if(abitante_da_combattere -> posizione == giocatore -> posizione) {
        printf("\nOra stai combattendo con %s\n", abitante_da_combattere -> nome_abitante_segrete);
        break;
      }
      abitante_da_combattere = abitante_da_combattere -> abitante_successivo;
    }

    if(abitante_da_combattere == NULL) {
      color('r');
      printf("Non ci sono abitanti da combattere!\n");
      color('0');
      return;
    }
    sleep(durata_intervallo);
  }
  
  //memorizzare punti vita iniziale di due combattenti
  unsigned short p_vita_giocatore = giocatore -> p_vita;
  unsigned short p_vita_abitante = abitante_da_combattere -> p_vita;

  do {
    time_t t; // Variabile per la generazione casuale
    srand((unsigned)time(&t));
    unsigned short dado_giocatore = (rand() % 6) +1;
    unsigned short dado_abitante = (rand() % 6) +1;
    printf("Dado giocatore = %hu VS ", dado_giocatore);
    sleep(durata_intervallo);
    printf("Dado abitante = %hu\n", dado_abitante);
    sleep(durata_intervallo);

    if(dado_giocatore >= dado_abitante) {
      printf("Si inizia prima %s\n", giocatore -> nome_giocatore);
      sleep(durata_intervallo);
      attacco(giocatore, abitante_da_combattere, true);
      if(abitante_da_combattere -> p_vita > 0)
        attacco(giocatore, abitante_da_combattere, false);
    } else {
      printf("Si inizia prima %s\n", abitante_da_combattere -> nome_abitante_segrete);
      sleep(durata_intervallo);
      attacco(giocatore, abitante_da_combattere, false);
      if(giocatore -> p_vita > 0)
        attacco(giocatore, abitante_da_combattere, true);
    }
  } while(giocatore -> p_vita > 0 && abitante_da_combattere -> p_vita > 0); //fino a che uno dei due duellanti muore

  if(giocatore -> p_vita > 0) { //se ha vinto il giocatore
    giocatore -> p_vita = p_vita_giocatore;
    printf("Ora i punti di vita di %s viene riprisinato a %hu\n", giocatore -> nome_giocatore, giocatore -> p_vita);
  } else {
    abitante_da_combattere -> p_vita = p_vita_abitante;
    printf("Ora i punti di vita di %s viene riprisinato a %hu\n", abitante_da_combattere -> nome_abitante_segrete, abitante_da_combattere -> p_vita);
  }
  sleep(durata_intervallo);
  printf("Combattimento terminato\n");
  color('0');
  sleep(durata_intervallo);
}

static unsigned short dado_con_pittogrammi() {
  time_t t; // Variabile per la generazione casuale
  srand((unsigned)time(&t));
  unsigned short numero_casuale = rand() % 6;
  if(numero_casuale < 3) {
    printf("dado lanciato = teschio\n");
    sleep(durata_intervallo);
    return 0;
  }
  if(numero_casuale < 5) {
    printf("dado lanciato = scudo bianco\n");
    sleep(durata_intervallo);
    return 1;
  }
  printf("dado lanciato = scudo nero\n");
  sleep(durata_intervallo);
  return 2;
}

static void attacco(Giocatore* giocatore, Abitante_segrete* abitante, bool attacco_di_giocatore) {
  unsigned short colpo = 0;
  unsigned short parato = 0;

  if(attacco_di_giocatore) {

    printf("Premere l'invio per lanciare i dadi di attacco...");
    while (getchar() != '\n');
    for(int i = 0; i < (giocatore -> dadi_attacco); i++) {
      if(dado_con_pittogrammi() == 0)
        colpo++;
    }
    
    printf("Ora %s inizia a lanciare i suoi dadi di difesa...\n", abitante -> nome_abitante_segrete);
    sleep(durata_intervallo);
    for(int i = 0; i < (abitante -> dadi_difesa); i++) {
      if(dado_con_pittogrammi() == 2)
        parato++;
    }

    if(colpo <= parato) {
      color('p');
      printf("%s ha parato tutti i colpi di %s!\n", abitante -> nome_abitante_segrete, giocatore -> nome_giocatore);
    } else {
      for(int i = 0; i < (colpo - parato); i++) {
        abitante -> p_vita--;
        color('g');
        printf("%s ha fatto un danno a %s (Punti vita: %hu)\n", giocatore -> nome_giocatore, abitante -> nome_abitante_segrete, abitante -> p_vita);
        if(abitante -> p_vita == 0) {
          sleep(durata_intervallo);
          color('g');
          printf("%s ha combattuto %s!\n", giocatore -> nome_giocatore, abitante -> nome_abitante_segrete);
          deallocazione_abitante_segrete(abitante);
          break;
        }
      }
    }
    
  } else {
    printf("Ora %s inizia a lanciare i suoi dadi di attacco..\n", abitante -> nome_abitante_segrete);
    sleep(durata_intervallo);
    for(int i = 0; i < (abitante -> dadi_attacco); i++) {
      if(dado_con_pittogrammi() == 0)
        colpo++;
    }

    printf("Premere l'invio per lanciare i dadi di difesa...");
    while (getchar() != '\n');
    for(int i = 0; i < (giocatore -> dadi_difesa); i++) {
      if(dado_con_pittogrammi() == 1)
        parato++;
    }

    if(colpo <= parato) {
      color('g');
      printf("%s ha parato tutti i colpi di %s!\n", giocatore -> nome_giocatore, abitante -> nome_abitante_segrete);
      
    } else {
      for(int i = 0; i < (colpo - parato); i++) {
        color('p');
        printf("%s ha fatto un danno a %s\n", abitante -> nome_abitante_segrete, giocatore -> nome_giocatore);
        toglie_punto_vita(giocatore);
        if(giocatore -> p_vita == 0) {
          sleep(durata_intervallo);
          color('p');
          printf("%s ha combattuto %s!\n", abitante -> nome_abitante_segrete, giocatore -> nome_giocatore);
          break;
        }
      }
    }
    
  }
  color('b');
  sleep(durata_intervallo);
}

static void gioca_potere_speciale(Giocatore* giocatore, unsigned short* azione) {
  if(giocatore -> potere_speciale == 0) {
    color('r');
    printf("Gentile %s, non hai il potere speciale per uccidere l'abitante delle segrete!\n" , giocatore -> nome_giocatore);
    color('0');
    return;
  }

  if(*azione >= 3) {
    color('r');
    printf("Hai già compiuto 3 azioni in questo turno!\n");
    color('0');
    return;
  } 

  Abitante_segrete* abitante_da_uccidere = primo_abitante;
  do {
    if(abitante_da_uccidere -> posizione == giocatore -> posizione) {
      giocatore -> potere_speciale--;
      printf("Hai ucciso %s con potere speciale! (il numero di potere speciale rimanenti: %hu)\n", abitante_da_uccidere -> nome_abitante_segrete, giocatore -> potere_speciale);
      deallocazione_abitante_segrete(abitante_da_uccidere);
      *azione = *azione + 1;
      break;
    }
    abitante_da_uccidere = abitante_da_uccidere -> abitante_successivo;
  } while(abitante_da_uccidere != NULL);

  if(abitante_da_uccidere == NULL) {
    color('r');
    printf("Non ci sono abitanti in questa zona!\n");
    color('0');
  }
}

static void investighi_zona (Giocatore* giocatore) {
  switch(giocatore -> posizione -> tipo_zona) {
    case corridoio:
      printf("Caratteristica di corridoio: non comparirà mai abitante delle segrete in caso di entrata di un giocatore\n(Escluso il caso in cui il corridoio è ultima stanza oppure il giocatore fallisce di scassinare la porta)\n");
      break;
    case scala:
      printf("Caratteristica di scala: con 16%% di probabilità comparirà un abitante segrete\n(Probabilità di zone normale: 33%%; probabilità di ultima zona: 100%%)\n");
      break;
    case sala_banchetto:
      printf("Caratteristica di sala banchetto: con 66%% di probabilità comparirà un abitante segrete\n(Probabilità di zone normale: 33%%; probabilità di ultima zona: 100%%)\n");
      break;
    case magazzino:
      printf("Evento speciale: C'è un libro che è conservato da un abitante delle segrete di magazzino\n(se riesci ad ucciderlo, ottenerai il libro che aumenta un punto di mente)\n");
      printf("Vuoi combatterlo?\n");
      evento_speciale(giocatore);
      break;
    case giardino:
      printf("Evento speciale: C'è un piante di guarigione in questo giardino\n(ti guadagna tre punti di vita)\n");
      printf("Vuoi cercarlo?\n");
      evento_speciale(giocatore);
      break;
    case posto_guardia:
      printf("Evento speciale: C'è un scudo bianco che è conservato da un abitante delle segrete di posto guardia\n(se riesci ad ucciderlo, ottenerai un dado di difesa)\n");
      printf("Vuoi combatterlo?\n");
      evento_speciale(giocatore);
      break;
    case prigione:
      printf("Caratteristica di prigione: con 100%% di probabilità comparirà un abitante segrete\n(Probabilità di zone normale: 33%%; probabilità di ultima zona: 100%%)\n");
      break;
    case cucina:
      printf("Evento speciale: Hai trovato tanti cibi nella tavola della cucina\n(ti guadagna due punti di vita)\n");
      printf("Vuoi mangiarlo?\n");
      evento_speciale(giocatore);
      break;
    case armeria:
      printf("Evento speciale: C'è una spada di teschio che è conservato da un abitante delle segrete di armeria\n(se riesci ad ucciderlo, ottenerai un dado di difesa)\n");
      printf("Vuoi combatterlo?\n");
      evento_speciale(giocatore);
      break;
    case tempio:
      printf("Evento speciale: C'è una statua di dea nel centro di tempio\n(Toccandorlo ti telesposta nella zona di partenza, ma ti guadagna 5 punti di vita, e ottenerai una volta in più di utilizzare il potere speciale, e non causerà l'apparizione di un nuovo abitante o tessoro.)\n");
      printf("Vuoi toccarlo?\n");
      evento_speciale(giocatore);
      break;
  }
}

static void evento_speciale(Giocatore* giocatore) {
  unsigned short scelta = 2;

  do {
    printf("0. No\n1. Sì\n");
    printf(">> ");
    scanf("%hu", &scelta);
    svuotaBuffer();

    if(scelta == 0) { //Se giocatore inserisce 0, vuole dire che non vuole procedere
      return;
    } else if(scelta != 1) { //Se giocatore ha inserito un numero diverso da 0 e 1, allora comando non riconosciuto
      color('r');
      printf("Il comando non riconosciuto!\n");
      color('0');
    }
  } while(scelta > 1);

  Tipo_zona tipo_zona = giocatore -> posizione -> tipo_zona;
  color('g');
  switch(tipo_zona) {
    case magazzino:
      combatti(giocatore, genera_abitante_segrete(100, giocatore -> posizione));
      if(giocatore -> p_vita > 0) {
        giocatore -> posizione -> evento_attivato = false;
        giocatore -> mente ++;
        color('g');
        printf("Hai ottenuto il libro \"L'arte della guerra\"! e ti ha guadagnato un punto di mente! (Punti mente attuale: %hu)\n", giocatore -> mente);
      }
      break;
    case giardino:
      giocatore -> p_vita += 3;
      giocatore -> posizione -> evento_attivato = false;
      printf("Hai ottenuto Dittamo! e ti ha guadagnato tre punti di vita! (Punti vita attuale: %hu)\n", giocatore -> p_vita);
      break;
    case posto_guardia:
      combatti(giocatore, genera_abitante_segrete(100, giocatore -> posizione));
      if(giocatore -> p_vita > 0) { //il giocatore ha vita > 0, vuol dire che ha vinto
        giocatore -> posizione -> evento_attivato = false;
        giocatore -> dadi_difesa++;
        color('g');
        printf("Hai ottenuto un scudo bianco! e ti ha guadagnato un dado di difesa in più! (Dadi difesa attuale: %hu)\n", giocatore -> dadi_difesa);
      }
      break;
    case cucina:
      giocatore -> p_vita += 2;
      giocatore -> posizione -> evento_attivato = false;
      printf("Hai consumato tutti i cibi, e ti ha guadagnato due punti di vita! (Punti vita attuale: %hu)\n", giocatore -> p_vita);
      break;
    case armeria:
      combatti(giocatore, genera_abitante_segrete(100, giocatore -> posizione));
      if(giocatore -> p_vita > 0) {
        giocatore -> posizione -> evento_attivato = false;
        giocatore -> dadi_attacco++;
        color('g');
        printf("Hai ottenuto la spada di techio! e ti ha guadagnato un dado di attacco in più! (Dadi attacco attuale: %hu)\n", giocatore -> dadi_attacco);
      }
      break;
    case tempio:
      giocatore -> posizione = pFirst;
      giocatore -> p_vita += 5;
      giocatore -> potere_speciale++;
      giocatore -> posizione -> evento_attivato = false;
      color('y');
      printf("Ora ti sei spostato nella zona di partenza\n");
      color('g');
      printf("Ti è guadagnato 5 punti di vita! (Punti vita attuale: %hu)\n", giocatore -> p_vita);
      printf("Ti è guadagnato una volta in più di utilizzare il potere speciale! (Potere speciale attuale: %hu)\n", giocatore -> potere_speciale);
      break;
    default:
  }
  color('0');
}