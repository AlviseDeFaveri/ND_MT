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
	newListItem->next = state->tranList[input];
	state->tranList[input] = newListItem;
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
#include <string.h>
#include <unistd.h>

#define QUIT_CHAR '\n'
#define N_STATES 20
#define N_STATES_INCREASE 20

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