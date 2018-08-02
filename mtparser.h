#include "mttypes.h"
#include <string.h>

#define QUIT_CHAR '\n'

State* searchState(MT* mt, int id) {
	printf("id %d is null? %d\n", id, mt->states[id]);
	return mt->states[id];

	// TODO hashmap search
}

void addState(MT* mt, State* state) {
	if(mt == NULL || state == NULL)
		printf("POrCODIO\n");

	if(mt->states[state->id] == NULL) {
		printf("adding state %d\n", state->id);
		mt->states[state->id] = state;
	} else {
		printf("Not adding state\n");
		// TODO linear probing
	}
}

/**
 * Create the MT states
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
		while ((flush = getchar()) != '\n' && flush != EOF) { }

		printf("id %d\n", state1_id);
		printf("in %c\n", in);
		printf("out %c\n", out);
		printf("mov %c\n", mov_char);
		printf("id %d\n", state2_id);

		/* Search initial state (create new if it doesn't exist) */
		State* state1 = searchState(mt, state1_id);
		if(state1 == NULL) {
			printf("State1 is null\n");
			state1 = newState(state1_id);
			printf("State1 is null after new? %d\n", state1);
			addState(mt, state1);
		}

		/* Search final state (create new if it doesn't exist) */
		State* state2 = searchState(mt, state2_id);
		if(state2 == NULL) {
			printf("State2 is null\n");
			state2 = newState(state2_id);
			printf("State2 is null after new? %d\n", state2);
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
		if(mt->states[state1_id] != NULL) {
			mt->states[state1_id]->status = ACCEPT;
		}
		while ((flush = getchar()) != '\n' && flush != EOF) { }
	}

	/* Run flag*/
	scanf("%s", s);
	if(strcmp(s,"run") != 0) {
		return NULL;
	} 
	else {
		printf("Run flag found\n");
	}

	/* XXX test only
	State* state1 = newState();
	State* state2 = newState();
	State* state3 = newState();
	State* state4 = newState();
	State* state5 = newState();
	State* state6 = newState();
	state6->status = ACCEPT;

	addTran(state1, 'a', 'a', R, state1);
	addTran(state1, 'b', 'b', R, state2);

	addTran(state2, 'b', 'b', R, state2);
	addTran(state2, BLANK, BLANK, L, state3);
	addTran(state3, 'b', 'b', L, state4);
	addTran(state4, 'a', 'a', L, state5);

	addTran(state5, BLANK, BLANK, L, state6);

	printf("State 1 = %x, %d\n", state1, state1->status);
	printf("State 2 = %x, %d\n", state2, state2->status);
	printf("State 3 = %x, %d\n", state3, state3->status);
	printf("State 4 = %x, %d\n", state4, state4->status);
	printf("State 5 = %x, %d\n", state5, state5->status);
	printf("State 6 = %x, %d\n", state6, state6->status);

	mt->curState = state1;
	*/

	mt->curState = mt->states[0];
	mt->nMovs = 10; // TODO read max movs

	printf("Initial state set to %d\n", mt->states[0]);
	while ((flush = getchar()) != '\n' && flush != EOF) { }

	return mt;
}

/**
 * Save the tape content in the MT's linked list reading from standard input
 */
uint32_t parseTape(MT* mt) {
	char c = BLANK;
	char flush;
	uint32_t nChar = 0;
	Tape_cell* initCell;

	/* Read chars */
	//while ((c = getchar()) != '\n' && c != EOF) { }
	c = getchar();

	while(c != QUIT_CHAR && c != EOF) {
		/* Se è il primo carattere crea una cella senza prev e next */
		if(mt->curCell == NULL) {
			mt->curCell = newCell(mt->ID, NULL, NULL);
			initCell = mt->curCell;
			printf("curCell NULL, new cell created instead\n");
		} else { /* Se non è il primo crea la prossima cella e avanza di 1 */
			mt->curCell->next = newCell(mt->ID, mt->curCell, NULL);
			mt->curCell = mt->curCell->next;
			printf("curCell not NULL, new cell created as next\n");
		}

		/* Conta i caratteri scritti */
		mt->curCell->content = c;
		// printf("Write %c actual %c\n", c, mt->curCell->content);
		nChar++;

		c = getchar();
	}

	/* Rewind the tape */
	mt->curCell = initCell;

	printf("First cell content = %c\n", mt->curCell->content);
	printf("curState = %x\n", mt->curState);

	return nChar;
}