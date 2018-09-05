#include <stdio.h>

#ifdef WITHTRACE
	#define TRACE(msg, ...) printf(msg, ##__VA_ARGS__)
#else
	#define TRACE(msg, ...) ((void)0)
#endif

#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define BLANK '_'
#define N_TRAN 256

#include <string.h>
#include <unistd.h>

#define QUIT_CHAR '\n'
#define N_STATES 11
#define N_STATES_INCREASE 20

/************************
 *  Types & Structures  *
 ************************/

enum bool_t {FALSE, TRUE};
enum test_mov {S, L, R};
enum mt_status {ONGOING, ACCEPT, NOT_ACCEPT, UNDEFINED};

/**
 * Stato e Transizioni.
 */
typedef struct State
{
	struct TranListItem* tranList[N_TRAN]; // Array di liste
	enum bool_t accept;
	uint32_t id;
} State;

typedef struct Transition 
{
	struct State* nextState;
	char output;
	enum test_mov mov;
} Transition;

typedef struct TranListItem 
{
	struct TranListItem* next;
	struct Transition tran;
} TranListItem;

/**
 * Nastro: double linked list di celle, ognuna ha un proprietario
 */
typedef struct Tape_cell 
{
	uint32_t owner;
	char content;
	struct Tape_cell* prev;
	struct Tape_cell* next;
} Tape_cell;

/**
 * MT
 */
typedef struct MT
{
	uint32_t ID;
	uint32_t nMovs;
	struct Tape_cell* curCell; // testina del nastro
	struct State* curState;
	enum mt_status result;
} MT;


typedef struct MTListItem
{
	struct MTListItem* next;
	MT mt;
} MTListItem;

/********************************
 *  Constructors & Destructors  *
 ********************************/

/**
 * Costruttore per gli stati
 */
State* newState(int id) 
{
	State* state = malloc(sizeof(State));
	int i = 0;

	for (i = 0; i < N_TRAN; i++) {
		state->tranList[i] = NULL;
	}

	state->id = id;
	state->accept = FALSE;
	return state;
}

/* Costruttore per le transizioni di uno stato */
void addTran(State* state, const char input, const char output,
					const enum test_mov mov, struct State* nextState) 
{
	/* Create transition */
	TranListItem* newListItem = malloc(sizeof(TranListItem));

	/* Populate transition */
	newListItem->tran.output 	= output;
	newListItem->tran.mov 		= mov;
	newListItem->tran.nextState = nextState;

	/* Add transition at the head of the tranList */
	newListItem->next = state->tranList[(uint8_t)input];
	state->tranList[(uint8_t)input] = newListItem;
}

/**
 * Costruttore per le celle
 */
Tape_cell* newCell(const uint32_t owner, struct Tape_cell* prev, 
						struct Tape_cell* next, const char content) 
{
	Tape_cell* cell = malloc(sizeof(Tape_cell));

	cell->owner = owner;
	cell->prev= prev;
	cell->next = next;
	cell->content = content;

	return cell;
}

/**
 * Costruttore della MT
 */
MTListItem* newMT(uint32_t ID, uint32_t nMovs, Tape_cell* curCell, State* curState) 
{
	/* Crea MT */
	MTListItem* mtItem = malloc(sizeof(MTListItem));

	/* Default settings */
	mtItem->next = NULL;
	mtItem->mt.ID = ID;
	mtItem->mt.nMovs = nMovs;
	mtItem->mt.curCell = newCell(ID, curCell->prev, curCell->next, curCell->content);
	mtItem->mt.curState = curState;
	mtItem->mt.result = ONGOING;

	assert(curCell != NULL);
	assert(curState != NULL);

	TRACE("Created MT_%d\n", ID);

	return mtItem;
}

/**
 * Distruttore delle celle del nastro
 */
void destroyTape(Tape_cell* cell, uint32_t id)
{
	Tape_cell* curCell = cell->next;
	uint8_t i = 0;

	if(cell == NULL)
		return;

	/* Distruggi tutte quelle dopo */
	while(curCell != NULL && curCell->owner == id) {
		// TRACE("Destroying cell '%c'\n", curCell->content);
		i++;
		Tape_cell* nextCell = curCell->next;
		free(curCell);
		curCell = nextCell;
	}

	/* Distruggi tutte quelle prima */
	curCell = cell;
	while(curCell != NULL && curCell->owner == id) {
		// TRACE("Destroying cell '%c'\n", curCell->content);
		i++;
		Tape_cell* nextCell = curCell->prev;
		free(curCell);
		curCell = nextCell;
	}
	// TRACE("Destroyed tape with id %d: %d cells\n", id,  i);
}

/**
 * Distruttore di MT
 */
void destroyMt(MTListItem* mtItem) {
	if(mtItem != NULL) {
		TRACE("\nDestroying MT_%d...\n", mtItem->mt.ID);
		destroyTape(mtItem->mt.curCell, mtItem->mt.ID);
		free(mtItem);
		// TRACE("Destroyed\n");
	}
}

/*************************************************************************************************************/
/*************************************************************************************************************/

/**********************
 *  Global Variables  *
 **********************/
State** states = NULL;
int maxState = 0;
Tape_cell* globTape = NULL;
int maxMovs = 0;

/**********************
 *  Function Headers  *
 **********************/
void parseMT();
void parseTransitions();
void parseAcc();

uint32_t parseTape();

void initStateHashmap();
State* searchState(int id);
void addState(State* state);

/*********************
 *  Function Bodies  *
 *********************/

/**
 * Create the MT states and transitions
 */
void parseMT() 
{
	char s[3];
	char flush;

	initStateHashmap();

	/* Transition FLAG */
	scanf("%s", s);
	if(strcmp(s,"tr") != 0) { return; }
	parseTransitions();
 
	/* Accept FLAG */
	scanf("%s", s);
	if(strcmp(s,"acc") != 0) { return; }
	parseAcc();

	/* Max FLAG */
	scanf("%s", s);
	if(strcmp(s,"max") != 0) { return; } 

	/* Maximum moves */
	TRACE("Max flag found\n");
	scanf(" %d", &maxMovs);

	/* Run flag*/
	scanf(" %s", s);
	if(strcmp(s,"run") != 0) { return; } 

	TRACE("Run flag found\n");
	while ((flush = getchar()) != '\n' && flush != EOF) { }
}

/* Crea stati e transizioni */
void parseTransitions() 
{
	char in, out, mov_char;
	int state1_id, state2_id;
	char flush;
	enum test_mov mov;

	TRACE("Transition flag found\n");

	/* Flush input buffer */
	while ((flush = getchar()) != '\n' && flush != EOF) { }

	/* Scan transitions */
	while(scanf("%d", &state1_id) > 0) {
		/* Read the whole transition line */
		scanf(" %c %c %c %d", &in, &out, &mov_char, &state2_id);
		while ((flush = getchar()) != '\n' && flush != EOF) { }
		// TRACE(" %d %c %c %c %d", state1_id, in, out, mov_char, state2_id);

		/* Search initial state (create new if it doesn't exist) */
		State* state1 = searchState(state1_id);
		if(state1 == NULL) {
			state1 = newState(state1_id);
			addState(state1);
		}

		/* Search final state (create new if it doesn't exist) */
		State* state2 = searchState(state2_id);
		if(state2 == NULL) {
			state2 = newState(state2_id);
			addState(state2);
		}

		/* Decode transition movement */
		if(mov_char == 'L') {
			mov = L;
		} else if(mov_char == 'R') {
			mov = R;
		} else if(mov_char == 'S') {
			mov = S;
		} else {
			return;
		}

		/* Create transition */
		addTran(state1, in, out, mov, state2);
		TRACE("adding %d, %c, %c, %d, %d\n", state1->id, in, out, mov, state2->id);
	}
}

/* Assegna lo stato di accettazione */
void parseAcc() 
{
	uint32_t id;
	char flush;
	TRACE("Acc flag found\n");

	while ((flush = getchar()) != '\n' && flush != EOF) { }

	while(scanf("%d", &id) > 0) 
	{
		State* state = searchState(id);
		assert(state!= NULL);

		state->accept = TRUE;

		char flush;
		while ((flush = getchar()) != '\n' && flush != EOF) { }
	}
}

/*
 * Read from standard input and save in globTape.
 */
uint32_t parseTape() {
	char c = BLANK;
	uint32_t nChar = 0;
	Tape_cell* curCell = globTape;

	/* Read chars */
	// while ((c = getchar()) != '\n' && c != EOF) { }
	c = getchar();

	/* Se è il primo carattere crea una cella senza prev e next */
	if(c != QUIT_CHAR && c != EOF) {
		globTape = newCell(0, NULL, NULL, c);
		curCell = globTape;
	}

	/* Se non è il primo crea la prossima cella */
	c = getchar();
	while(c != QUIT_CHAR && c != EOF) 
	{
		assert(curCell != NULL);
		
		Tape_cell* prevCell = curCell;
		curCell = newCell(0, prevCell, NULL, c);
		prevCell->next = curCell;

		/* Conta i caratteri scritti */
		nChar++;
		c = getchar();
	}

	return nChar;
}

/**
 *  Inizializza l'hashmap degli stati 
 */
void initStateHashmap() {
	states = (State**) malloc(N_STATES * sizeof(State*));
	maxState = N_STATES;

	for(int i = 0; i < N_STATES; i++){
		states[i] = NULL;
	}
}

/**
 *  Inizializza l'hashmap degli stati 
 */
void reinitStateHashmap(int n) {
	if(n <= maxState)
		return;

	states = (State**) realloc(states, n * sizeof(State*));

	for(int i = maxState; i < n; i++){
		states[i] = NULL;
	}

	maxState = n;
}

/* Search state in hashmap */
State* searchState(int id) {
	if(id >= maxState){
		return NULL;
	}

	return states[id];
}

/* Add state to hashmap */
void addState(State* state) {
	assert(state != NULL);

	while(state->id >= maxState){
		reinitStateHashmap(maxState + N_STATES_INCREASE);
	}

	TRACE("Adding state %d\n", state->id);
	states[state->id] = state;
}

/*******************************************************************************************/
/*******************************************************************************************/


/**********************
 *  Global Variables  *
 **********************/
MTListItem* mtlist = NULL;
uint32_t nMt = 0;
uint32_t glob_id = 0;

/**********************
 *  Function Headers  *
 **********************/
void initMtList();
void cleanMtList();

// MTListItem* removeFromList(MTListItem* stopped, enum mt_status status);
enum mt_status processTape(Tape_cell* tape);
enum mt_status branch(MT* originalMt, TranListItem* tranList);
enum mt_status evolve(MT* mt, Transition* tran);
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov);

/*********************
 *  Function Bodies  *
 *********************/

/* Crea mtlist con la prima MT*/
void initMtList() {
	/* Pulisce mtlist */
	if(mtlist != NULL) {
		cleanMtList();
	}

	/* Crea la prima MT */
	mtlist = newMT(1, maxMovs, globTape, states[0]);
	// TRACE("First MT\n");

	/* Setta stato iniziale e numero di movimenti */
	nMt = 1;
	glob_id = 2; // ID del nastro = 0, ID prima MT = 1
}

/* Pulisce mtlist */
void cleanMtList() {
	MTListItem* mtItem = mtlist;
	MTListItem* next = NULL;

	/* Dealloca tutte le MT parallele */
	while (mtItem != NULL) {
		next = mtItem->next;

		destroyMt(mtItem);
		mtItem = next;
	}

	mtlist = NULL;
}

/**
 * Process tape until acceptance or finish
 */
enum mt_status processTape(Tape_cell* tape) {
	enum mt_status status = ONGOING;
	MTListItem* mtItem = mtlist;

	/* Pulisci e riinizializza mtlist */
	initMtList();
	mtlist->mt.curCell = globTape;

	/* Evolvi tutte le macchine finchè non sono finite */
	while (nMt > 0) // TODO: perche' non basta nMt?
	{
		#ifndef WITHTRACE
			/* Printa tutte le mt */
			TRACE("\nNuovo giro di MT (tot:%d)\n",  nMt);
		#endif

		mtItem = mtlist;

		/* Scorri tutte le MT */
		while (mtItem != NULL) 
		{
			MT* mt = &(mtItem->mt);

			TRACE("\nProcessing: %d\n", mt->ID);
			assert(mt != NULL);
			assert(mt->curCell != NULL);
			assert(mt->curState != NULL);

			/* Se e' ferma saltala */
			if(mt->result != ONGOING){
				mtItem = mtItem->next;
				TRACE("[MT_%d] Already stopped with status %d\n", mt->ID, mt->result);
				continue;
			}

			// TRACE("Processing: %d, state: %d input %c\n", mt->ID, mt->curState->id, mt->curCell->content);

			/* Leggi dal nastro */
			char c = mt->curCell->content;

			/* Trova la lista di transizioni */
			TranListItem* curTran = mt->curState->tranList[(uint8_t)c];

			/* Crea una MT per ogni transizione */
			status = branch(mt, curTran);

			/* Se accetta esci, se non accetta distruggila */
			if(status == ACCEPT) {
				return ACCEPT;
			}

			mtItem = mtItem->next;
		}
		// TRACE("Finished MTs\n");
	}

	cleanMtList();
	return status;
}

/* 
 * Crea nuove MT a partire dall'originale leggendo la lista di transizioni,
 * le fa evolvere e aggiorna la mtlist (se non è terminata).
 *
 * Ritorna: ACCEPT appena una accetta.
 *          ONGOING se almeno uno è ongoing.
 *          UNDEFINED se almeno una ha finito le mosse e nessuna è ongoing.
 *          NOT_ACCEPT se tutte non accettano.
 */
enum mt_status branch(MT* originalMt, TranListItem* tranList) 
{
	/* Nessuna transizione: la macchina si ferma */
	if(tranList == NULL){
		originalMt->result = NOT_ACCEPT;
		nMt--;
		return NOT_ACCEPT;
	}
	/* Una sola transizione: evolvi */
	else if(tranList->next == NULL) {
		enum mt_status evolveStatus = evolve(originalMt, &(tranList->tran));
		if(evolveStatus != ONGOING) {
			nMt--;
		}
		return evolveStatus;
	}

	/* Transizioni multiple: stoppa quella corrente */
	originalMt->result = NOT_ACCEPT;
	nMt--;

	/* Cicla su tutte le transizioni parallele */
	TranListItem* tranItem = tranList;
	enum mt_status retStatus = NOT_ACCEPT;

	while(tranItem != NULL) 
	{
		assert(tranItem->tran.nextState != NULL);

		/* Se il prossimo stato accetta, ritorna subito */
		if(tranItem->tran.nextState->accept == TRUE) {
			originalMt->result = ACCEPT;
			return ACCEPT;
		}
		else {
			/* Crea nuova MT */
			MTListItem* newMt = newMT(glob_id, originalMt->nMovs, originalMt->curCell,
										originalMt->curState);

			/* Evolvi nuova MT */
			enum mt_status evolveStatus = evolve(&(newMt->mt), &(tranItem->tran));

			/* Se non è morta, aggiungila a MTList */
			switch(evolveStatus) {
				case ACCEPT:
					/* Ritorna subito */
					destroyMt(newMt);
					return ACCEPT;
					break;
				case ONGOING:
					/* Aggiungila in testa a mtlist */
					newMt->next = mtlist;
					mtlist = newMt;
					/* Aggiorna variabili globali */
					nMt++;
					glob_id++;
					retStatus = ONGOING;
					break;
				case UNDEFINED:
					/* Almeno una è indefinita: eliminala */
					if(retStatus == NOT_ACCEPT) {
						retStatus = UNDEFINED;
					}
					destroyMt(newMt);
					break;
				case NOT_ACCEPT:
					/* Eliminala subito */
					destroyMt(newMt);
					break;
			}
		}

		tranItem = tranItem->next;
	}

	return retStatus;
}

/**
 * Evolve MT: scrivi carattere, muovi la testina, cambia stato e decrementa il numero di mosse
 */
enum mt_status evolve(MT* mt, Transition* tran) {
	TRACE("Evolving MT_%d", mt->ID);
	assert(mt != NULL);
	
	/* Nessuna transizione */
	if(tran == NULL) {
		mt->result = NOT_ACCEPT;
	}
	/* Mosse finite */
	else if (mt->nMovs == 0) {
		mt->result = UNDEFINED;
	}
	/* La transizione accetta */
	else if(tran->nextState->accept == TRUE) {
		mt->result = ACCEPT;
	} 
	/* Evolvi davvero la MT */
	else {
		TRACE("Evolving MT_%d: state %d, reading %c writing %c -> state %d\n", mt->ID, mt->curState->id, mt->curCell->content, tran->output, tran->nextState->id);

		/* Cambia stato */
		mt->curState = tran->nextState;
		mt->curCell->content = tran->output;
		mt->curCell = moveCell(mt->ID, mt->curCell, tran->mov);
		assert(mt->curCell != NULL);
		// TRACE("moving cell, mt %d new cell %c curState%d\n", mt->ID, mt->curCell->content, mt->curState->id);

		mt->nMovs--;
		mt->result = ONGOING;
	}

	return mt->result;
}

/**
 * Ritorna la prossima cella (creando la cella se ce n'è bisogno)
 */
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov) {
	Tape_cell* nextCell = curCell;
	Tape_cell* nextNextCell = NULL;

	/* Prossima cella */
	if (mov == L) {
		nextCell = curCell->prev;
		if (nextCell != NULL) nextNextCell = nextCell->prev;
	} else if (mov == R) {
		nextCell = curCell->next;
		if (nextCell != NULL) nextNextCell = nextCell->next;
	}

	/* Se la prossima cella ha un ID diverso o non esiste, crea nuova */
	if(nextCell == NULL || nextCell->owner != id) {
		if(mov == L) {
			nextCell = newCell (id, nextNextCell, curCell, (nextCell == NULL) ? BLANK : nextCell->content);
			curCell->prev = nextCell;
		}
		else if(mov == R) {
			nextCell = newCell (id, curCell, nextNextCell, (nextCell == NULL) ? BLANK : nextCell->content);
			curCell->next = nextCell;
		}
	} 

	return nextCell;
}


/*******************************************************************************************/
/*******************************************************************************************/

/**
 * Main
 */
int main() {
	enum mt_status status = ONGOING;

	/* Parsa stati e transizioni */
	parseMT();

	/* Cicla sugli input */
	while(1) {

		/* Parsa il nastro */
		uint32_t tape = parseTape();
		if (tape == 0) break;

		TRACE("\n----NEW INPUT---\n");
		// tapeDump(globTape);

		/* Processa l'input */
		status = processTape(globTape);

		switch(status){
			case ACCEPT:
				printf("1\n");
				break;
			case UNDEFINED:
				printf("U\n");
				break;
			default:
				printf("0\n");
				break;
		}
		
		/* Dealloca il nastro */
		destroyTape(globTape, 0);

		TRACE("Tape destroyed\n");
	}

	cleanMtList();
	// TODO: destroyMT(); ?
}