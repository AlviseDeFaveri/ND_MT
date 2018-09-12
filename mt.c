#include <stdio.h>

#ifdef WITHTRACE
	#define TRACE(msg, ...) printf(msg, ##__VA_ARGS__)
#else
	#define TRACE(msg, ...) ((void)0)
#endif

//#define assert(x) if(!(x)) {printf("PORCODIO\n"); exit(1);}
#define assert(x)

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define BLANK '_'

#define INIT_TAPE_DIM 1000000
#define N_STATES 10
#define N_TRAN 10
#define TAPE_LEN_INC_MAX 10000
#define TAPE_LEN_INIT 100
#define TAPE_LEN_SCALE 1.01f
#define MAX_CLEAN 1
int TAPE_LEN_INC = 0U;

/************************
 *  Types & Structures  *
 ************************/

#define FALSE 0
#define TRUE 1
typedef uint8_t bool_t;

#define MOV_S 'S'
#define MOV_L 'L'
#define MOV_R 'R'
typedef char test_mov;

#define ONGOING 0
#define ACCEPT 1
#define NOT_ACCEPT 2
#define UNDEFINED 3
typedef uint8_t mt_status;

typedef struct __attribute__((packed)) State
{
	struct TranListItem* tranList[N_TRAN]; // Lista di transizioni
	bool_t accept;
	int id;
	struct State* next;
} State;

typedef struct __attribute__((packed)) Transition 
{
	struct State* nextState;
	char input;
	char output;
	test_mov mov;
} Transition;

typedef struct __attribute__((packed)) TranListItem 
{
	struct TranListItem* next;
	struct Transition tran;
} TranListItem;

typedef struct __attribute__((packed)) Tape
{
	char* cells;
	// uint16_t len;
	uint32_t cur;
} Tape;

typedef struct __attribute__((packed)) MT
{
	Tape tape;
	struct State* curState;
} MT;


typedef struct __attribute__((packed)) MTListItem
{
	struct MTListItem* next;
	MT mt;
} MTListItem;


/**********************
 *  Global Variables  *
 **********************/
State** states;

bool_t statesOk = 0;
bool_t accOk = 0;

char initTape[INIT_TAPE_DIM];

uint64_t maxMovs = 0U;
MTListItem* mtlist = NULL;
State* firstState = NULL;
uint32_t nMt = 0U;
bool_t dioc1 = FALSE, dioc2 = FALSE;


/********************************
 *  Constructors & Destructors  *
 ********************************/
State* searchState(const int id);
State* addState(const int id);

/**
 * Costruttore per gli stati
 */
State* newState(const int id) 
{
	State* state = (State*) malloc(sizeof(State));

	memset(state->tranList, 0, N_TRAN);
	state->id = id;
	state->accept = FALSE;
	state->next = NULL;
	return state;
}

/* Search state in hashmap */
State* searchState(const int id) {

	assert(id >= 0);
	State* state = states[id%N_STATES];

	while(state != NULL) {
		if(state->id == id) return state;
		state = state->next;
	}

	return NULL;
}

/* Add state to hashmap */
State* addState(const int state_id) {
	State* state = newState(state_id);

	state->next = states[state_id%N_STATES];
	states[state_id%N_STATES] = state;

	TRACE("Adding state %d\n", state->id);

	if(state_id == 0) {
		firstState = state;
	}

	//assert(state_id < N_STATES && stateCount < N_STATES);

	return state;
}

/* Costruttore per le transizioni di uno stato */
void addTran(State* state, const char input, const char output,
					const test_mov mov, State* nextState) 
{
	// assert(state != NULL);
	// assert(nextState != NULL);
	/* Create transition */
	TranListItem* newListItem = (TranListItem*) malloc(sizeof(TranListItem));

	/* Populate transition */
	newListItem->tran.input 	= input;
	newListItem->tran.output 	= output;
	newListItem->tran.mov 		= mov;
	newListItem->tran.nextState = nextState;

	/* Add transition at the head of the tranList */
	newListItem->next = state->tranList[input%N_TRAN];
	state->tranList[input%N_TRAN] = newListItem;
}

/**
 * Costruttore della MT
 */
MTListItem* newMT(Tape* tape, State* curState) 
{
	/* Crea MT */
	MTListItem* mtItem = (MTListItem*) malloc(sizeof(MTListItem));

	/* Default settings */
	mtItem->next = NULL;
	mtItem->mt.curState = curState;

	/* Copy tape */
	int len = strlen(tape->cells) ;
	assert(len > 0);

	mtItem->mt.tape.cells = (char*) malloc(len+ 1);
	char* a = strcpy(mtItem->mt.tape.cells, tape->cells);
	assert(a != NULL)
	mtItem->mt.tape.cur = tape->cur;

	#ifdef WITHTRACE
	malloc_stats();
	#endif

	TRACE("\nAdded MT %x...\n", mtItem);

	return mtItem;
}

/**
 * Distruttore di MT
 */
void destroyMt(MTListItem* mtItem) {
	if(mtItem != NULL) {
		TRACE("\nDestroying MT %x...\n", mtItem);
		free(mtItem->mt.tape.cells);
		free(mtItem);

		mtItem = NULL;
		// TRACE("Destroyed\n");
	}
}

/*************************************************************************************************************
 *                                        P A R S E R                                                        *
 *************************************************************************************************************/

/**********************
 *  Function Headers  *
 **********************/
bool_t parseMT();
void parseTransitions();
void parseAcc();

uint32_t parseTape();

/*********************
 *  Function Bodies  *
 *********************/

/**
 * Create the MT states and transitions
 */
bool_t parseMT() 
{
	char s[4];
	char flush;

	/* Transition FLAG */
	scanf("%s", s);
	assert(strcmp(s,"tr") == 0);
	parseTransitions();

	/* Accept FLAG */
	scanf("%s", s);
	assert(strcmp(s,"acc") == 0);
	parseAcc();

	/* Max FLAG */
	scanf("%s", s);
	assert(strcmp(s,"max") == 0);

	/* Maximum moves */
	TRACE("Max flag found\n");
	scanf("%lu", &maxMovs);

	/* Run flag*/
	scanf("%s", s);
	assert(strcmp(s,"run") == 0);

	TRACE("Run flag found\n");
	while ((flush = getchar()) != '\n' && flush != EOF) { }
	
	return TRUE;
}

/* Crea stati e transizioni */
void parseTransitions() 
{
	char in, out, mov_char;
	int state1_id, state2_id;
	char flush;

	TRACE("Transition flag found\n");

	/* Flush input buffer */
	while ((flush = getchar()) != '\n' && flush != EOF) { }

	/* Scan transitions */
	while ( scanf("%d", &state1_id) > 0 ) {
		/* Read the whole transition line */
		scanf(" %c %c %c %d", &in, &out, &mov_char, &state2_id);
		while ((flush = getchar()) != '\n' && flush != EOF) { }
		// TRACE(" %d %c %c %c %d", state1_id, in, out, mov_char, state2_id);

		/* Search initial state (create new if it doesn't exist) */
		State* state1 = searchState(state1_id);
		if(state1 == NULL) {
			state1 = addState(state1_id);
		}

		/* Search final state (create new if it doesn't exist) */
		State* state2 = searchState(state2_id);
		if(state2 == NULL) {
			state2 = addState(state2_id);
		}

		/* Create transition */
		addTran(state1, in, out, mov_char, state2);
		TRACE("adding %d, %c, %c, %c, %d\n", state1->id, in, out, mov_char, state2->id);
		statesOk = TRUE;
	}

	assert(statesOk);
}

/* Assegna lo stato di accettazione */
void parseAcc() 
{
	int id;
	char flush;
	TRACE("Acc flag found\n");

	while ((flush = getchar()) != '\n' && flush != EOF) { }

	while(scanf("%d", &id) > 0) 
	{
		State* state = searchState(id);

		assert(state!= NULL)
		state->accept = TRUE;

		char flush;
		while ((flush = getchar()) != '\n' && flush != EOF) { }
		accOk = TRUE;
	}

	assert(accOk);
}

/*******************************************************************************************
 *                          E V O L V E R                                                  *
 *******************************************************************************************/

/**********************
 *  Function Headers  *
 **********************/
// MTListItem* removeFromList(MTListItem* stopped, enum mt_status status);
mt_status process();
mt_status branch(MTListItem* originalMt, const char c);
mt_status evolve(MT* mt, Transition* tran);
void move(MT* mt, const test_mov mov);

/*********************
 *  Function Bodies  *
 *********************/

/* Pulisce mtlist */
void cleanMtList() {
	MTListItem* mtItem = mtlist;
	MTListItem* next = NULL;

	/* Dealloca tutte le MT parallele */
	int i = 0;
	while (mtItem != NULL) {
		next = mtItem->next;

		destroyMt(mtItem);
		mtItem = next;
		i++;
		if(i == MAX_CLEAN)
			break;
	}

	mtlist = NULL;
}

/* Crea mtlist con la prima MT*/
void initMtList(char* tapeCells) {
	/* Pulisce mtlist */
	if(mtlist != NULL) {
		//cleanMtList();
	}

	/* Crea la prima MT */
	assert(firstState != NULL);
	assert(tapeCells != NULL);

	Tape tape;
	tape.cells = tapeCells;
	tape.cur = 1;

	// if(mtlist != NULL) {
		mtlist = newMT(&tape, firstState);
	// } else {
	// 	mtlist = reinitMt(mtlist, &tape, firstState);
	// }

	nMt = 1;
}

void addToMtList(MTListItem* mtItem) {
	mtItem->next = mtlist;
	mtlist = mtItem;
	++nMt;
}

MTListItem* removeFromList(MTListItem* mtItem, MTListItem* prev) {
	assert(nMt > 0);
	--nMt;
	if(mtItem == mtlist) {
		mtlist = mtlist->next;
		destroyMt(mtItem);
		return mtlist;
	}
	else {
		// assert(prev != NULL);
		MTListItem* next = mtItem->next;
		prev->next = next;
		destroyMt(mtItem);
		return next;
	}
}

void cleanStateList() {
	State* stateItem = states;
	State* next = NULL;

	/* Dealloca tutte le MT parallele */
	while (stateItem != NULL) {
		next = stateItem->next;

		TranListItem* tran = stateItem->tranList;
		TranListItem* nextTran = NULL;

		while(tran != NULL) {
			nextTran = tran->next;
			free(tran);
			tran = nextTran;
		}

		free(stateItem);
		stateItem = next;
	}
}

/**
 * Process tape until acceptance or finish
 */
mt_status process() {
	mt_status status = ONGOING;
	MTListItem* mtItem = mtlist;
	MTListItem* prev = NULL;
	uint64_t nMovs = maxMovs;

	dioc1 = FALSE;
	dioc2 = FALSE;

	/* Evolvi tutte le macchine finchè non sono finite */
	while (nMt > 0 && nMovs > 0)
	{
		#ifndef WITHTRACE
			/* Printa tutte le mt */
			TRACE("\nNuovo giro di MT (tot:%d)\n",  nMt);
		#endif

		mtItem = mtlist;

		/* Scorri tutte le MT */
		while (mtItem != NULL) 
		{
			TRACE("\nProcessing MT %x\n", mtItem);

			/* Leggi dal nastro */
			char c = mtItem->mt.tape.cells[mtItem->mt.tape.cur];
			assert(c != '\0' && c != '\n' && c != ' ');

			/* Crea una MT per ogni transizione */
			status = branch(mtItem, c);

			/* Se accetta esci */
			switch(status) {
				case ACCEPT:
					return ACCEPT;
					break;
				case NOT_ACCEPT:
					mtItem = removeFromList(mtItem, prev);
					if(nMt == 0) return NOT_ACCEPT;
					break;
				case ONGOING:
					prev = mtItem;
					mtItem = mtItem->next;
					break;
			}
			
		}

		dioc1 = FALSE;
		dioc2 = FALSE;

		--nMovs;
		if(nMovs == 0 && nMt > 0) 
			status = UNDEFINED;

		TRACE("nMovs %d status %d\n", nMovs, status);

		// TRACE("Finished MTs\n");
	}

	//cleanMtList();
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
mt_status branch(MTListItem* originalMt, const char c) 
{
	TranListItem* tranItem = originalMt->mt.curState->tranList[c%N_TRAN];
	TranListItem* firstTran = NULL;
	uint8_t found = 0;
	mt_status retStatus = NOT_ACCEPT;

	while(tranItem != NULL) 
	{
		/* Transizione trovata */
		if(tranItem->tran.input == c) 
		{
			/* Salva la prima */
			if(found == 0) {
				firstTran = tranItem;
			} 
			/* Evolvi le successive	 */
			else {
				/* Crea nuova MT */
				MTListItem* newMt = newMT(&(originalMt->mt.tape), originalMt->mt.curState);

				/* Evolvi nuova MT */
				mt_status evolveStatus = evolve(&(newMt->mt), &(tranItem->tran));

				/* Se non è morta, aggiungila a MTList */
				switch(evolveStatus) {
					case ACCEPT:
						/* Ritorna subito */
						destroyMt(newMt);
						return ACCEPT;
						break;
					case ONGOING:
						/* Aggiungila in testa a mtlist */
						addToMtList(newMt);
						// retStatus = ONGOING;
						break;
					default:
						/* Eliminala subito */
						destroyMt(newMt);
						break;
				}

			}

			++found;
		}

		tranItem = tranItem->next;
	}

	/* Evolvi la prima */
	if(found > 0) {
		mt_status evolveStatus = evolve(&(originalMt->mt), &(firstTran->tran));

		switch(evolveStatus) {
			case ACCEPT:
				/* Ritorna subito */
				return ACCEPT;
				break;
			case NOT_ACCEPT:
			case UNDEFINED:
				/* Segnala da scartare */
				originalMt->mt.curState = NULL;
				retStatus = NOT_ACCEPT;
				break;
			case ONGOING:
				retStatus = ONGOING;
				break;
		}
	}

	return retStatus;
}

/**
 * Evolve MT: scrivi carattere, muovi la testina, cambia stato e decrementa il numero di mosse
 */
mt_status evolve(MT* mt, Transition* tran) {
	// assert(mt != NULL);
	// assert(tran != NULL);
	
	/* La transizione accetta */
	if(tran->nextState->accept == TRUE) {
		return ACCEPT;
	} 
	/* Evolvi davvero la MT */
	else {
		TRACE( 	"Evolving MT: state %d, reading %c writing %c -> state %d\n",
				mt->curState->id, mt->tape.cells[mt->tape.cur],
				tran->output, tran->nextState->id );

		/* Cambia stato */
		mt->curState = tran->nextState;
		/* Cambia il nastro */
		mt->tape.cells[mt->tape.cur] = tran->output;
		move(mt, tran->mov);

		// printf("Evolving MT 0x%x\n", mt);
		// printf("%s\n", mt->tape.cells);

		// for(int i = 0; i < mt->tape.cur; i++){
		// 	printf(" ");
		// }
		// printf("@\n");

		return ONGOING;
	}
	
}

/**
 * Muove la MT alla prossima cella (creando la cella se ce n'è bisogno)
 */
void move(MT* mt, const test_mov mov) {
	/* Prossima cella */
	if (mov == MOV_R) 
	{
		/* Incrementa */
		mt->tape.cur++;
		uint32_t len = strlen(mt->tape.cells);
		// assert(len > 0);
		/* Realloc tape */
		if(mt->tape.cur >= len) {
			mt->tape.cells = (char*) realloc(mt->tape.cells, len + TAPE_LEN_INC + 1);

			assert(mt->tape.cells[len] == '\0');
			assert(mt->tape.cells!=NULL);

			//char* a = strcat(mt->tape.cells, TAPE_INC_CONTENT);
			for (int i = 0; i < TAPE_LEN_INC; ++i)
		    {
		        mt->tape.cells[len + i] = BLANK;
		    }

			// assert(a != NULL);
			mt->tape.cells[len + TAPE_LEN_INC] = '\0';
			assert(mt->tape.cells!=NULL);

			if(TAPE_LEN_INC < TAPE_LEN_INC_MAX && dioc2 == FALSE){
				TAPE_LEN_INC *= TAPE_LEN_SCALE;
				dioc2 = TRUE;
			}

			//printf("Realloc dopo (R) %s \n", mt->tape.cells);
		}
	} 
	else if (mov == MOV_L) {
		/* Realloc tape */
		if(mt->tape.cur == 0) {
			int len = strlen(mt->tape.cells);
			// assert(len > 0);

			mt->tape.cells = (char*) realloc(mt->tape.cells, len + TAPE_LEN_INC + 1);
			memmove( mt->tape.cells + TAPE_LEN_INC, mt->tape.cells, len + 1);

			assert(mt->tape.cells[len + TAPE_LEN_INC] == '\0');
			assert(mt->tape.cells!=NULL);

		    for (int i = 0; i < TAPE_LEN_INC; ++i)
		    {
		        mt->tape.cells[i] = BLANK;
		    }

			TRACE("Realloc dopo (L) %s \n", mt->tape.cells);

			mt->tape.cur = TAPE_LEN_INC;

			if(TAPE_LEN_INC < TAPE_LEN_INC_MAX && dioc2 == FALSE){
				TAPE_LEN_INC *= TAPE_LEN_SCALE;
				dioc2 = TRUE;
			}
		}

		/* Decrementa */
		mt->tape.cur--;
	}
}


/*******************************************************************************************
 *                              M A I N                                                    *
 *******************************************************************************************/

/**
 * Main
 */
int main() {
	mt_status status = ONGOING;

	/* Parsa stati e transizioni */
	states = calloc(N_STATES, sizeof(State*));
	assert(states[0] == NULL);

	bool_t ok = parseMT();

	/* Cicla sugli input */
	while(1) {
		TAPE_LEN_INC = TAPE_LEN_INIT;

		/* Parsa il nastro */
		int tape = scanf(" %s", initTape + 1);
		//int tape = fgets(initTape, INIT_TAPE_DIM, stdin);
		if (tape <= 0) break;

		initTape[0] = BLANK;
		assert(strlen(initTape) > 0);

		char* a = strcat(initTape, "_");
		assert(a != NULL)

		if(ok == FALSE) {
			printf("0\n");
			continue;
		}

		/* Pulisci e riinizializza mtlist */
		initMtList(initTape);

		/* Processa l'input */
		status = process();

		cleanMtList();

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

	}
 
	//cleanMtList();
	// cleanStateList();
}