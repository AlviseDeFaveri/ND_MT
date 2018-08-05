#include "mttypes.h"
#include <string.h>

#define QUIT_CHAR '\n'

State* searchState(MT* mt, int id) {
	// printf("Searching %d\n", id);
	return states[id];

	// TODO hashmap search
}

void addState(MT* mt, State* state) {
	if(mt == NULL || state == NULL)
		printf("POrCODIO\n");

	if(states[state->id] == NULL) {
		printf("adding state %d\n", state->id);
		states[state->id] = state;
	} else {
		printf("Not adding state\n");
		// TODO linear probing
	}
}

/**
 * Create the MT states and transitions
 */
MT* parseMT() {
	char s[3], in, out, mov_char;
	int state1_id, state2_id;
	enum test_mov mov;

	MT* mt = newMT();
	mt->curCell = NULL;

	/* Transitions FLAG */
	scanf("%s", s);
	if(strcmp(s,"tr") != 0) {
		return NULL;
	} else {
		printf("Transition flag found\n");
	}
	char flush;
	
	while ((flush = getchar()) != '\n' && flush != EOF) { }
	/* Scan transitions */
	while(scanf("%d", &state1_id) > 0) {

		/* Read the whole transition line */
		//scanf("%d", &state1_id);
		char flush;
		int flag;

		scanf(" %c %c %c %d", &in, &out, &mov_char, &state2_id);
		// printf(" %d %c %c %c %d", state1_id, in, out, mov_char, state2_id);
		while ((flush = getchar()) != '\n' && flush != EOF) { }

		/* Search initial state (create new if it doesn't exist) */
		State* state1 = searchState(mt, state1_id);
		if(state1 == NULL) {
			state1 = newState(state1_id);
			addState(mt, state1);
		}

		/* Search final state (create new if it doesn't exist) */
		State* state2 = searchState(mt, state2_id);
		if(state2 == NULL) {
			state2 = newState(state2_id);
			addState(mt, state2);
		}

		/* Decode transition movement */
		if(mov_char == 'L') {
			mov = L;
		} else if(mov_char == 'R') {
			mov = R;
		} else if(mov_char == 'S') {
			mov = S;
		} else {
			return NULL;
		}

		/* Create transition */
		addTran(state1, in, out, mov, state2);
		printf("adding %d, %c, %c, %d, %d\n", state1->id, in, out, mov, state2->id);
	}
 
	/* Accept FLAG */
	scanf("%s", s);
	if(strcmp(s,"acc") != 0) {
		return NULL;
	} 

	/* Acceptance states */
	printf("Acc flag found\n");
	while(scanf("%d", &state1_id) > 0) {
		if(states[state1_id] != NULL) {
			states[state1_id]->status = ACCEPT;
		}
		while ((flush = getchar()) != '\n' && flush != EOF) { }
	}

	/* Max FLAG */
	scanf("%s", s);
	if(strcmp(s,"max") != 0) {
		return NULL;
	} 

	/* Maximum moves */
	printf("Max flag found\n");
	scanf(" %d", &(mt->nMovs));

	/* Run flag*/
	scanf(" %s", s);
	if(strcmp(s,"run") != 0) {
		return NULL;
	} 
	else {
		printf("Run flag found\n");
	}

	mt->curState = states[0];
	maxMovs = mt->nMovs;

	printf("Initial state set to %d\n", states[0]);
	while ((flush = getchar()) != '\n' && flush != EOF) { }

	return mt;
}

/*
 * Read from standard input and save in the MT's memory.
 */
uint32_t parseTape(MT* mt) {
	char c = BLANK;
	char flush;
	uint32_t nChar = 0;
	Tape_cell* initCell;

	/* Read chars */
	// while ((c = getchar()) != '\n' && c != EOF) { }
	c = getchar();

	while(c != QUIT_CHAR && c != EOF) {
		/* Se è il primo carattere crea una cella senza prev e next */
		if(mt->curCell == NULL) {
			mt->curCell = newCell(mt->ID, NULL, NULL, BLANK);
			initCell = mt->curCell;
		} 
		else { /* Se non è il primo crea la prossima cella e avanza di 1 */
			mt->curCell->next = newCell(mt->ID, mt->curCell, NULL, BLANK);
			mt->curCell = mt->curCell->next;
		}

		/* Conta i caratteri scritti */
		mt->curCell->content = c;
		nChar++;

		c = getchar();
	}

	/* Rewind the tape */
	mt->curCell = initCell;

	return nChar;
}