void imposta_gioco();
void gioca();
void termina_gioco();
typedef enum Classe_giocatore {barbaro, nano, elfo, mago} Classe_giocatore;
typedef enum Tipo_zona {corridoio, scala, sala_banchetto, magazzino, giardino, posto_guardia, prigione, cucina, armeria, tempio} Tipo_zona;
typedef enum Tipo_tesoro {nessun_tesoro, veleno, guarigione, doppia_guarigione} Tipo_tesoro;
typedef enum Tipo_porta {nessuna_porta, porta_normale, porta_da_scassinare} Tipo_porta;
typedef enum bool {true = 1, false = 0} bool;

typedef struct Giocatore {
    char nome_giocatore[10];
    enum Classe_giocatore classe_giocatore;
    struct Zona_segrete* posizione;
    unsigned char dadi_attacco;
    unsigned char dadi_difesa;
    unsigned char p_vita;
    unsigned char mente;
    unsigned char potere_speciale;
} Giocatore;

typedef struct Zona_segrete {
    struct Zona_segrete* zona_successiva;
    struct Zona_segrete* zona_precedente;
    enum Tipo_zona tipo_zona;
    enum Tipo_tesoro tipo_tesoro;
    enum Tipo_porta tipo_porta;
    enum bool evento_attivato; //Il campo in aggiunto per poter gestire il sistema di evento speciale per ogni zona
} Zona_segrete;

typedef struct Abitante_segrete {
    char nome_abitante_segrete[20];
    unsigned char dadi_attacco;
    unsigned char dadi_difesa;
    unsigned char p_vita;
    struct Zona_segrete* posizione;
    struct Abitante_segrete* abitante_successivo;
} Abitante_segrete;