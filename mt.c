#include <stdio.h>
#include "mtparser.h"

Transition* findTran(Transition* tranList, char read);
void moveCell(MT* mt, enum test_mov mov);
enum mt_status evolve(MT* mt);
void dump(MT* mt);

/**
 * Main
 */
int main() {

	enum mt_status status = ONGOING;
	printf("STARTED\n");

	MT* mt = parseMT();
	printf("MT parsed\n");

	uint32_t tape = parseTape(mt);

	while(tape != 0) {

		//dump(mt);
		//return 0;

		/* Fai evolvere la MT finchè non finisce in uno stato di stop */
		status = ONGOING;
		while (status == ONGOING) {
			status = evolve(mt);
		}

		printf("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);

		mt->curCell = NULL;
		tape = parseTape(mt);
		mt->curState = mt->states[0];
		mt->nMovs = 10; // TODO read max movs

		printf("Tape parsed\n");
		char flush;
		while ((flush = getchar()) != '\n' && flush != EOF) { }
	}

	return 0;
}

/**
 * Evolve MT
 */
enum mt_status evolve(MT* mt) {
	printf("Evolving MT_%x: reading %c\n", mt->ID, mt->curCell->content);
	/* Leggi dal nastro un carattere e cercalo nella mappa delle transizioni dello stato corrente */
	Transition* tran = findTran(mt->curState->transitions,
											 mt->curCell->content);
	// TODO: epsilon-mosse

	if(tran != NULL && mt->nMovs > 0) {
		printf("Found transition %c, %x | %c, %d, %x\n", mt->curCell->content, 
			mt->curState, tran->output, tran->mov, tran->nextState);
		/* Check se lo stato prossimo è di stop */
		if(tran->nextState->status != ONGOING) {
			return tran->nextState->status;
		}

		/* Scrivi sul nastro e muoviti */
		mt->curCell->content = tran->output;
		moveCell(mt, tran->mov);

		/* Cambia lo stato corrente */
		mt->curState = tran->nextState;
		mt->nMovs--;
	} else {
		return NOT_ACCEPT;
	}

	return mt->curState->status;
}

/**
 * Find transition inside the transition array of the state (TODO check hashmap)
 */
Transition* findTran(Transition* tranList, char read) {
	if(tranList[read].mov != UNINIT)
		return &tranList[read];
	else
		return NULL;
}

/**
 * Muove la testina (creando la cella se ce n'è bisogno)
 */
void moveCell(MT* mt, enum test_mov mov) {
	/* Check that the movement is valid */
	assert(mov != UNINIT);

	/* Store a pointer to the target cell */
	Tape_cell* nextCell = mt->curCell;
	if(mov == L) {
		nextCell = mt->curCell->prev;
	} else if (mov == R) {
		nextCell = mt->curCell->next;
	}

	/* If the target cell pointer has another MT's id or it's null, create a new cell */
	if(nextCell == NULL || nextCell->owner != mt->ID) {
		if(mov == L) {
			nextCell = newCell(mt->ID, NULL, mt->curCell);
		} else if (mov == R) {
			nextCell = newCell(mt->ID, mt->curCell, NULL);
		}
	} 

	mt->curCell = nextCell;
}

void dump(MT* mt) {
	if(mt == NULL) {
		printf("MT is NULL!\n");
	} else {
		printf("MT {\n");
		printf("\tID %d\n", mt->ID);

		State* state = mt->curState;
		printf("\tStates {\n");

		while(state != NULL) {
			printf("\t\tstate: %x\n", state);
			printf("\t\tstatus: %d\n", state->status);
			printf("\t\tTRANSITIONS {\n");

			for(int i = 0; i < N_TRAN; i++) {
				if(state->transitions[i].mov != UNINIT) {
					printf( "\t\t\tout %c mov %d nextState %x\n", 
						state->transitions[i].output, state->transitions[i].mov,
						state->transitions[i].nextState );
				}
			}

			printf("\t\t}\n");
			state = NULL; // state = state->nextState;
		}
		printf("\t}\n");

		Tape_cell* cell = mt->curCell;
		printf("\tCells {\n");

		while(cell != NULL) {
			printf("\t\tCell {\n");
			printf("\t\t\taddress: %x\n", cell);
			printf("\t\t\towner: %d\n", cell->owner);
			printf("\t\t\tcontent: %c\n", cell->content);
			printf("\t\t\tprev: %x\n", cell->prev);
			printf("\t\t\tnext: %x\n", cell->next);
			printf("\t\t}\n");
			cell = cell->next;
		}

		printf("\t}\n");
	}
}