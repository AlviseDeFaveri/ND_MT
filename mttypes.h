#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define BLANK '_'
#define N_TRAN 256
#define N_STATES 256

enum test_mov {UNINIT, S, L, R};
enum mt_status {ONGOING, ACCEPT, NOT_ACCEPT};
uint32_t glob_ID = 0;

/**
 * Transizione
 */
typedef struct Transition 
{
	char output;
	enum test_mov mov;
	struct State* nextState;
} Transition;

Transition* newTran(const char output, const enum test_mov mov, struct State* nextState) {
	Transition* tran = malloc(sizeof(Transition));
	tran->output = output;
	tran->mov = mov;
	tran->nextState = nextState;
	return tran;
}

/**
 * Stato
 */
typedef struct State
{
	struct Transition transitions[N_TRAN];
	enum mt_status status;
	int id;
} State;

State* newState(int id) {
	State* state = malloc(sizeof(State));

	int i = 0;
	for(i = 0; i < N_TRAN; i++){
		state->transitions[i].mov = UNINIT;
	}
	state->id = id;
	state->status = NOT_ACCEPT; // cambia se ne aggiungi qualcuna
	return state;
}

void addTran(State* state, const char input, const char output,
				 const enum test_mov mov, struct State* nextState) {
	assert(mov != UNINIT);

	Transition* tran = &(state->transitions[input]);
	tran->output = output;
	tran->mov = mov;
	tran->nextState = nextState;

	if(state->status == NOT_ACCEPT) {
		state->status = ONGOING;
	}
}

/**
 * Nastro
 */
typedef struct Tape_cell
{
	uint32_t owner;
	struct Tape_cell* prev;
	struct Tape_cell* next;
	char content;
} Tape_cell;

Tape_cell* newCell(const uint32_t owner, struct Tape_cell* prev, 
						struct Tape_cell* next) {
	Tape_cell* cell = malloc(sizeof(Tape_cell));
	cell->owner = owner;
	cell->prev= prev;
	cell->next = next;
	cell->content = BLANK;
	return cell;
}

/**
 * MT
 */
typedef struct MT
{
	uint32_t ID;
	uint32_t nMovs;
	struct Tape_cell* curCell;
	struct State* curState;
	State* states[N_STATES];
} MT;

MT* newMT() {
	MT* mt = malloc(sizeof(MT));
	mt->ID = glob_ID;
	glob_ID++;
	mt->nMovs = 1;
	mt->curCell = NULL;
	mt->curState = NULL;

	for(int i = 0; i < N_STATES; i++){
		mt->states[i] = NULL;
	}

	return mt;
}

MT* copyMt(MT* oldMt) {
	if(oldMt != NULL) {
		MT* mt = malloc(sizeof(MT));

		mt->ID = glob_ID;
		glob_ID++;

		mt->nMovs = oldMt->nMovs;
		mt->curCell = oldMt->curCell;
		mt->curState = oldMt->curState;

		return mt;
	}

	return NULL;
}

// TODO distruttori!!!