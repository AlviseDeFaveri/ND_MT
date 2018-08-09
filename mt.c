#include <stdio.h>
#include "mtparser.h"

enum mt_status processInput(Tape_cell* tape);
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov);
enum mt_status evolve(MT* mt, Transition* tran);
void mtDump(MT* mt);
void tapeDump(MT* mt);

void setup() {
	printf("STARTED\n");

	for(int i = 0; i < N_STATES; i++){
		states[i] = NULL;
	}
}

/**
 * Main
 */
int main() {
	enum mt_status status = ONGOING;

	setup();

	/* Parse the MT states, max and acc */
	MT* mt = parseMT();
	mtDump(mt);

	MTListItem* mtl = mtlist;
	while(mtl!= NULL) {
		printf("MT null? %d\n", mtl->mt == NULL);
		printf("MT %d\n", mtl->mt->ID);
		mtl = mtl->next;
	}

	MT* backupMt = copyMt(mt);

	/* Cicla sugli input */
	while(1) {
		printf("\n----NEW INPUT---\n");

		/* Parsa il nastro */
		//mt = backupMt;
		//mtlist->mt = mt;
		mt->curCell = NULL;
		uint32_t tape = parseTape(mt);
		// tapeDump(mt);
		if (tape == 0) break;

		/* Setta stato iniziale e numero di movimenti */
		mt->curState = states[0];
		nMt = 1;
		glob_id = 1;
		mt->nMovs = maxMovs;
		mt = mtlist->mt;

		MTListItem* mtl = mtlist;
		while(mtl!= NULL) {
			printf("MT null? %d\n", mtl->mt == NULL);
			printf("MT %d\n", mtl->mt->ID);
			mtl = mtl->next;
		}

		/* Processa l'input */
		enum mt_status status = processInput(mt->curCell);
		printf("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);
		// while ((flush = getchar()) != '\n' && flush != EOF) { }

		/* Dealloca il nastro */
		destroyTape(mtlist->mt->curCell);
		printf("Tape destroyed\n");

		/* Dealloca tutte le MT parallele */
		MTListItem* mtItem = mtlist;
		MTListItem* next = NULL;
		while (mtItem != NULL) {
			printf("MT destroying %d\n", mtItem->mt->ID);
			next = mtItem->next;
			destroyMt(mtItem->mt->ID);
			mtItem = next;
		}

		printf("Detroyed all\n");
		
	}
}

/**
 * Process tape until acceptance or finish
 */
enum mt_status processInput(Tape_cell* tape) {
	enum mt_status status = ONGOING;
	MTListItem* mtItem = mtlist;

	/* Evolvi tutte le macchine finchè almeno una non accetta */
	while (status != ACCEPT && nMt > 0 ) 
	{
		mtItem = mtlist;
		/* Per ogni MT */
		while (mtItem != NULL && mtlist != NULL) 
		{
			MT* mt = mtItem->mt;
			printf("Processing: %d, state: %d input %c\n", mt->ID, mt->curState->id, mt->curCell->content);

			/* Trova la lista di transizioni corrispondenti all'input della testina */
			char c = mt->curCell->content;
			TranList* curTran = mt->curState->transitions[(uint8_t)c];

			/* Se non ce n'è neanche una, questa macchina non accetta */
			if(curTran == NULL) {
				printf("No transition\n");
				destroyMt(mt->ID);
				/* Se è l'ultima ritorna 0 */
				if(nMt == 1) return NOT_ACCEPT;
			}
			else {
				/* Altrimenti, crea una nuova mt ogni transizione di questa mt */
				uint8_t first = 1;
				// printf("-----------------------------------\n");
				// MTListItem* mtl = mtlist;
				// while(mtl!= NULL) {
				// 	printf("MT null? %d\n", mtl->mt == NULL);
				// 	printf("MT %d\n", mtl->mt->ID);
				// 	mtl = mtl->next;
				// }
				while (curTran != NULL) 
				{
					// printf("Tran found: ");
					/*printf( "out %c mov %d nextState %d\n", 
									curTran->tran.output, curTran->tran.mov,
									curTran->tran.nextState->id );*/

					if(first) {
						status = evolve(mt, &(curTran->tran));
						// printf("Evolving first %d\n", mt->ID);
						first = 0;
					}
					else {
						MT* temp = copyMtAndIncrease(mt);
						printf("Evolving new %d\n", temp->ID);
						status = evolve(temp, &(curTran->tran));
					}

					printf("Returned %d\n", status);
					/* Se una qualsiasi accetta ritorna 1 */
					if(status == ACCEPT) {
						return ACCEPT;
					}

					/* Se si è piantata distruggila */
					if(status != ONGOING) {
						destroyMt(mt->ID);
						/* Se era l'ultima ritorna 0 o U */
						if(nMt == 1) return status;
					}
 					// printf("created new mt", tran->nextState->id);
 					curTran = curTran->next;
				}
				// printf("Finished transitions\n");
			}
			mtItem = mtItem->next;
			// printf("Next MT: mtItem null? %d\n", mtItem == NULL);
		}
		printf("Finished MTs\n");
	}
}

/**
 * Evolve MT: scrivi carattere, muovi la testina, cambia stato e decrementa il numero di mosse
 */
enum mt_status evolve(MT* mt, Transition* tran) {
	/* If there are no more moves return U */
	if (mt->nMovs == 0) {
		// printf("UNDEFINED\n");
		return UNDEFINED;
	}

	printf("Evolving MT_%d: reading %c ", mt->ID, mt->curCell->content);

	/* Change state */
	mt->curState = tran->nextState;

	/* If next state is ACCEPT or NOT ACCEPT return immediately (will be destoyed) */
	if (mt->curState->status != ONGOING) {
		// printf("returning immediately, mt %d status %d\n", mt->ID, mt->curState->status);
		return mt->curState->status;
	} else {
		mt->curCell->content = tran->output;
		mt->curCell = moveCell(mt->ID, mt->curCell, tran->mov);
		// printf("moving cell, mt %d new cell %c curState%d\n", mt->ID, mt->curCell->content, mt->curState->id);
	}

	mt->nMovs--;
	return mt->curState->status;
}

/**
 * Ritorna la prossima cella (creando la cella se ce n'è bisogno)
 */
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov) {
	/* Store a pointer to the target cell */
	Tape_cell* nextCell = curCell;

	if(mov == L) {
		nextCell = curCell->prev;
	} else if (mov == R) {
		nextCell = curCell->next;
	}

	/* If the target cell pointer has another MT's id or it's null, create a new cell */
	if(nextCell == NULL || nextCell->owner != id) {
		Tape_cell* linkedCell = NULL;
		if(mov == L) {
			if(nextCell != NULL) linkedCell = nextCell->prev;
				nextCell = newCell (id, linkedCell, curCell, (nextCell == NULL) ? BLANK : nextCell->content);
		} else if (mov == R) {
			if(nextCell != NULL) linkedCell = nextCell->next;
				nextCell = newCell (id, curCell, linkedCell, (nextCell == NULL) ? BLANK : nextCell->content);
		}
	} 

	return nextCell;
}

/**
 * Stampa la MT
 */
void mtDump(MT* mt) {

	printf("----------- MT ---------------\n");
	
	if (mt == NULL) {
		printf("MT is NULL!\n");
	} 
	else {
		printf("MT {\n");
		printf("\tis %d\n", mt->ID);

		State* state = mt->curState;
		printf("\tSTATES {\n");

		for (int i = 0; i < N_TRAN; i++) 
		{

			if (states[i] == NULL)
				break;

			state = states[i];
			printf("\t\tid: %d\n", state->id);
			printf("\t\tstatus: %d\n", state->status);
			printf("\t\tTRANSITIONS {\n");

			for (int i = 0; i < N_TRAN; i++) 
			{
				if(state->transitions[i] != NULL) {
					TranList* item = state->transitions[i];

					do {
						printf( "\t\t\tout %c mov %d nextState %d\n", 
									item->tran.output, item->tran.mov,
									item->tran.nextState->id );
						item = item->next;
					} while(item != NULL);
				}
			}

			printf("\t\t}\n");
		}
		printf("\t}\n");
		printf("}\n");
	}

	printf("----------------------------\n\n");
}

void tapeDump(MT* mt) {

	printf("----------- TAPE ---------------\n");
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
	printf("--------------------------------\n\n");
}