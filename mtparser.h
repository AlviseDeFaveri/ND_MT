#include "mttypes.h"
#include <string.h>
#include <unistd.h>

#define QUIT_CHAR '\n'
#define N_STATES 256

/* GLOBAL VARIABLES */
State* states[N_STATES];
Tape_cell* globTape = NULL;
int maxMovs = 0;

/**
 * Search and add in state hashmap
 */
State* searchState(int id) {
	// TRACE("Searching %d\n", id);
	// TODO: hashmap search
	return states[id];
}

void addState(State* state) {
	if(state == NULL) {
		TRACE("POrCODIO\n");
		return;
	}

	if(states[state->id] == NULL) {
		TRACE("Adding state %d\n", state->id);
		states[state->id] = state;
	} else {
		TRACE("Not adding state\n");
		// TODO: linear probing
	}
}

/**
 * Create the MT states and transitions
 */
void parseTransitions();
void parseAcc();
void parseTranAndStates() 
{
	char s[3];
	char flush;

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