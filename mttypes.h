#ifndef MTTYPES
#define MTTYPES

#ifdef NOTRACE
	#define TRACE(msg, ...) ((void)0)
#else
	#define TRACE(msg, ...) printf(msg, ##__VA_ARGS__)
#endif

#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define BLANK '_'
#define N_TRAN 256

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

/*******************************************************************/

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

/**************************************************************************/

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

#endif // MTTYPES