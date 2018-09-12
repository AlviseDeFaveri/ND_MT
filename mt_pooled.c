#include <stdio.h>

#ifdef WITHTRACE
	#define TRACE(msg, ...) printf(msg, ##__VA_ARGS__)
	#define assert(x) if(!(x)) {printf("PORCODIO\n"); exit(1);}
#else
	#define TRACE(msg, ...) ((void)0)
	#define assert(x) 
#endif

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define BLANK '_'

#define INIT_TAPE_DIM 1000000
#define N_STATES 13
#define TAPE_LEN_INC_MAX 200000
#define TAPE_LEN_INIT 10
#define TAPE_LEN_SCALE 5
#define MTLIST_INIT_DIM 10

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
	struct TranListItem* tranList; // Lista di transizioni
	bool_t accept;
	int id;
	struct State* next;  // Bucket
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
	uint32_t cur;
} Tape;

typedef struct __attribute__((packed)) MT
{
	Tape tape;
	struct State* curState;
	int input;
} MT;


/**********************
 *  Global Variables  *
 **********************/
State** states;
State* firstState = NULL;

char initTape[INIT_TAPE_DIM];
int maxMovs = 0U;

bool_t statesOk = 0;
bool_t accOk = 0;

MT* mtlist = NULL;
int mtlistDim = MTLIST_INIT_DIM;
int curInput = 1;

uint32_t nMt = 0U;
bool_t dioc1 = FALSE, dioc2 = FALSE;


/********************************
 *  Constructors & Destructors  *
 ********************************/
State* newState(const int id) ;
State* searchState(const int id);
State* addToStateList(const int id);
void cleanStateList();

int newMT(const int i, Tape* tape, State* curState);
void destroyMt(const int i) ;

void initMtList(char* tapeCells);
void addToMtList(const int i);
void removeFromMtList(const int i);
void cleanMtList();

void addTran(State* state, const char input, const char output,
					const test_mov mov, State* nextState);

/**
 * Costruttore per gli stati
 */
State* newState(const int id) 
{
	State* state = (State*) malloc(sizeof(State));

	state->tranList = NULL;
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
State* addToStateList(const int state_id) {
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

void cleanStateList() {
	// State* stateItem = states;
	// State* next = NULL;

	// /* Dealloca tutte le MT parallele */
	// while (stateItem != NULL) {
	// 	next = stateItem->next;

	// 	TranListItem* tran = stateItem->tranList;
	// 	TranListItem* nextTran = NULL;

	// 	while(tran != NULL) {
	// 		nextTran = tran->next;
	// 		free(tran);
	// 		tran = nextTran;
	// 	}

	// 	free(stateItem);
	// 	stateItem = next;
	// }
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
	newListItem->next = state->tranList;
	state->tranList = newListItem;
}

/**
 * Costruttore della MT
 */
int newMT(const int refIndex, Tape* refTape, State* curState) 
{
	MT* mt = NULL;
	int retIndex = -1;

	int len = strlen(refTape->cells);
	assert(len > 0);
	assert(curInput > 0);

	/* Cercane una libera */
	for(int i = 0; i < mtlistDim; ++i) {
		if (mtlist[i].input != curInput) {
			mt = &(mtlist[i]);
			retIndex = i;

			free(mt->tape.cells);
			TRACE("Found free MT_%d\n", i);
			break;
		}
	}

	/* Se non c'è allarga la mtlist e metti tutti gli input a 0 */
	if(retIndex == -1) {
		TRACE("Free MT not found\n");
		mtlist = (MT*) realloc(mtlist, sizeof(MT) * (mtlistDim*2 ));
		assert(mtlist != NULL);

		for(int i = mtlistDim; i < mtlistDim*2; ++i) {
			TRACE("%d\n", i);
			mtlist[i].input = 0;
		}

		mt = (MT*) &(mtlist[mtlistDim]);
		retIndex = mtlistDim;
		mtlistDim *= 2;

		if(refIndex >= 0) {
			refTape = &(mtlist[refIndex]);
		}
	}


	mt->tape.cells = (char*) malloc(len+ 1);

	assert(mt != NULL);
	mt->curState = curState;
	mt->input = curInput;
	mt->tape.cur = refTape->cur;

	/* Copia il nastro */
	char* a = strcpy(mt->tape.cells, refTape->cells);
	assert(a != NULL)

	#ifdef WITHTRACE
	//malloc_stats();
	#endif

	TRACE("\nAdded MT_%d...\n", retIndex);

	return retIndex;
}

/**
 * Distruttore di MT
 */
void destroyMt(const int mtIndex) {
	assert(mtIndex < mtlistDim);
	mtlist[mtIndex].input = 0;
}

/* Crea mtlist con la prima MT*/
void initMtList(char* tapeCells) {

	/* Crea il nastro */
	assert(firstState != NULL);
	assert(tapeCells != NULL);
	Tape tape;
	tape.cells = tapeCells;
	tape.cur = 1;

	/* Crea la prima MT */
	TRACE("Creating first MT...\n");
	mtlist[0].input = -1;
	newMT(-1, &tape, firstState);
	assert(mtlist[0].input == curInput);
	TRACE("Created first MT\n");

	nMt = 1;
}

void addToMtList(const int i) {
	assert(i >= 0 && i < mtlistDim);
	++nMt;
}

void removeFromMtList(const int i) {
	TRACE("removing i=%d\n", i);
	assert(i >= 0 && i < mtlistDim);
	destroyMt(i);
	if(nMt > 0)
		--nMt;
}

/* Pulisce mtlist */
void cleanMtList() {
	curInput++;
}

/*************************************************************************************************************
 *                                        P A R S E R                                                        *
 *************************************************************************************************************/

/**********************
 *  Function Headers  *
 **********************/
void parseMT();
void parseTransitions();
void parseAcc();

/*********************
 *  Function Bodies  *
 *********************/

/**
 * Create the MT states and transitions
 */
void parseMT() 
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
	scanf("%d", &maxMovs);
	assert(maxMovs > 0);
	//assert(maxMovs <= 1000000);

	// if(maxMovs > 2000000) maxMovs = 200000;

	/* Run flag*/
	scanf(" %s", s);
	assert(strcmp(s,"run") == 0);

	TRACE("Run flag found\n");
	while ((flush = getchar()) != '\n' && flush != EOF) { }
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
			state1 = addToStateList(state1_id);
		}

		/* Search final state (create new if it doesn't exist) */
		State* state2 = searchState(state2_id);
		if(state2 == NULL) {
			state2 = addToStateList(state2_id);
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
mt_status branch(const int i, const char c);
mt_status evolve(const int i, Transition* tran);
void move(const int i, const test_mov mov);

/*********************
 *  Function Bodies  *
 *********************/

/**
 * Process tape until acceptance or finish
 */
mt_status process() {
	mt_status status = ONGOING;
	int nMovs = maxMovs;

	/* Evolvi tutte le macchine finchè non sono finite */
	while (nMt > 0 && nMovs > 0)
	{
		#ifndef WITHTRACE
			/* Printa tutte le mt */
			TRACE("\nNuovo giro di MT (tot:%d)\n",  nMt);
		#endif


		int initNMT = nMt;
		int count = 0;
		/* Scorri tutte le MT */
		for (int i = 0; i < mtlistDim; i++) 
		{
			if(mtlist[i].input != curInput)
				continue;
			TRACE("\nProcessing MT_%d\n", i);
			count++;
			if(count >= initNMT + 1) {
				break;
			}

			// TRACE("Processing: %d, state: %d input %c\n", mt->ID, mt->curState->id, mt->curCell->content);

			/* Leggi dal nastro */
			char c = mtlist[i].tape.cells[mtlist[i].tape.cur];
			assert(c != '\0' && c != '\n' && c != ' ');

			/* Crea una MT per ogni transizione */
			status = branch(i, c);

			/* Se accetta esci */
			switch(status) {
				case ACCEPT:
					return ACCEPT;
					break;
				case NOT_ACCEPT:
					removeFromMtList(i);
					if(nMt == 0) return NOT_ACCEPT;
					break;
				default:
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
mt_status branch(const int mtIndex, const char c) 
{
	TranListItem* tranItem = mtlist[mtIndex].curState->tranList;
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
				TRACE("FOUND FIRST\n");
				firstTran = tranItem;
				TRACE("TAPE:%s\n", mtlist[mtIndex].tape.cells);
			} 
			/* Evolvi le successive	 */
			else {
				/* Crea nuova MT */
				TRACE("FOUND NEXT\n");
				int i = newMT(mtIndex, &(mtlist[mtIndex].tape), mtlist[mtIndex].curState);

				/* Evolvi nuova MT */
				mt_status evolveStatus = evolve(i, &(tranItem->tran));

				/* Se non è morta, aggiungila a MTList */
				switch(evolveStatus) {
					case ACCEPT:
						/* Ritorna subito */
						TRACE("ACCEPT\n");
						destroyMt(i);
						TRACE("ACCEPT\n");
						return ACCEPT;
						break;
					case ONGOING:
						/* Aggiungila in testa a mtlist */
						TRACE("ONGOING\n");
						addToMtList(i);
						TRACE("ONGOING\n");
						// retStatus = ONGOING;
						break;
					default:
						/* Eliminala subito */
						TRACE("DESTROY\n");
						destroyMt(i);
						TRACE("DESTROY\n");
						break;
				}

			}

			++found;
		}

		tranItem = tranItem->next;
	}

	/* Evolvi la prima */
	if(found > 0) {
		TRACE("EVOLVING FIRST %d...\n", mtIndex);
		TRACE("TAPE:%s\n", mtlist[mtIndex].tape.cells);
		mt_status evolveStatus = evolve(mtIndex, &(firstTran->tran));
		TRACE("EVOLVED FIRST\n");

		switch(evolveStatus) {
			case ACCEPT:
				/* Ritorna subito */
				return ACCEPT;
				break;
			case NOT_ACCEPT:
			case UNDEFINED:
				/* Segnala da scartare */
				mtlist[mtIndex].curState = NULL;
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
mt_status evolve(const int i, Transition* tran) {
	assert(i < mtlistDim && i >= 0);
	assert(tran != NULL);
	
	/* La transizione accetta */
	if(tran->nextState->accept == TRUE) {
		return ACCEPT;
	} 
	/* Evolvi davvero la MT */
	else {
		TRACE("Evolving --- MT_%d... in%d curin%d\n", i, mtlist[i].input, curInput);
		TRACE("%s\n", mtlist[i].tape.cells);
		TRACE("Input %d...\n", mtlist[i].tape.cells[mtlist[i].tape.cur]);
		TRACE("CurState %d...\n", mtlist[i].curState->id);
		TRACE("Output %d...\n", tran->output);
		TRACE("NextState %d...\n",  tran->nextState->id);
		assert(mtlist[i].curState != NULL);
		assert(mtlist[i].tape.cells[mtlist[i].tape.cur] != '\n');

		// TRACE( 	"Evolving MT_%d: state %d, reading %c writing %c -> state %d\n", i,
		// 		mtlist[i].curState->id, 'c'/*mtlist[i].tape.cells[mtlist[i].tape.cur]*/,
		// 		tran->output, tran->nextState->id );

		/* Cambia stato */
		mtlist[i].curState = tran->nextState;
		/* Cambia il nastro */
		mtlist[i].tape.cells[mtlist[i].tape.cur] = tran->output;
		TRACE("Moving...\n");
		move(i, tran->mov);
		TRACE("Evolved!\n");

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
void move(const int i, const test_mov mov) {
	/* Prossima cella */
	if (mov == MOV_R) 
	{
		/* Incrementa */
		mtlist[i].tape.cur++;
		uint32_t len = strlen(mtlist[i].tape.cells);
		assert(len > 0);
		/* Realloc tape */
		if(mtlist[i].tape.cur >= len) {
			mtlist[i].tape.cells = (char*) realloc(mtlist[i].tape.cells, len + TAPE_LEN_INC + 1);

			assert(mtlist[i].tape.cells[len] == '\0');
			assert(mtlist[i].tape.cells!=NULL);

			// char* a = strcat(mt->tape.cells, TAPE_INC_CONTENT);
			for (int j = 0; j < TAPE_LEN_INC; ++j)
		    {
		        mtlist[i].tape.cells[len + j] = BLANK;
		    }

			// assert(a != NULL);
			mtlist[i].tape.cells[len + TAPE_LEN_INC] = '\0';
			assert(mtlist[i].tape.cells!=NULL);

			TRACE("Realloc dopo (R) %s \n", mtlist[i].tape.cells);
			if(TAPE_LEN_INC < TAPE_LEN_INC_MAX && dioc1 == FALSE){
				TAPE_LEN_INC *= TAPE_LEN_SCALE;
				dioc1 = TRUE;
			}
		}
	} 
	else if (mov == MOV_L) {
		/* Realloc tape */
		if(mtlist[i].tape.cur == 0) {
			int len = strlen(mtlist[i].tape.cells);
			assert(len > 0);

			mtlist[i].tape.cells = (char*) realloc(mtlist[i].tape.cells, len + TAPE_LEN_INC + 1);
			memmove( mtlist[i].tape.cells + TAPE_LEN_INC, mtlist[i].tape.cells, len + 1);

			assert(mtlist[i].tape.cells[len + TAPE_LEN_INC] == '\0');
			assert(mtlist[i].tape.cells!=NULL);

		    for (int j = 0; j < TAPE_LEN_INC; ++j)
		    {
		        mtlist[i].tape.cells[j] = BLANK;
		    }

			TRACE("Realloc dopo (L) %s \n", mtlist[i].tape.cells);

			mtlist[i].tape.cur = TAPE_LEN_INC;

			if(TAPE_LEN_INC < TAPE_LEN_INC_MAX && dioc2 == FALSE){
				TAPE_LEN_INC *= TAPE_LEN_SCALE;
				dioc2 = TRUE;
			}
		}

		/* Decrementa */
		mtlist[i].tape.cur--;
	}
}


/*******************************************************************************************
 *                              M A I N                                                    *
 *******************************************************************************************/

/**
 * Main
 */
int main() {
	/* Inizializzazione*/
	states = calloc(N_STATES, sizeof(State*));
	assert(states[0] == NULL);

	mtlistDim = MTLIST_INIT_DIM;
	curInput = 1;
	mtlist = calloc(mtlistDim, sizeof(MT));
	assert(mtlist != NULL);
	// for(int i = 0; i < mtlistDim; ++i){
	// 	mtlist[i].input = 0;
	// }

	/* Parsa stati e transizioni */
	parseMT();

	/* Cicla sugli input */
	mt_status status = ONGOING;
	while(1) {

		TAPE_LEN_INC = TAPE_LEN_INIT;

		/* Parsa il nastro */
		int tape = scanf(" %s", initTape + 1);
		if (tape <= 0) break;
		//assert(strlen(initTape) > 0);

		initTape[0] = BLANK;
		char* a = strcat(initTape, "_");

		/* Pulisci e riinizializza mtlist */
		TRACE("Inizializzo mtlist...\n");
		initMtList(initTape);
		TRACE("Inizializzata mtlist");

		/* Processa l'input */
		status = process();

		//if(maxMovs> 1000000 && nMt < 100)
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
 
	// cleanMtList();
	// cleanStateList();
}