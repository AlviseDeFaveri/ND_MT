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
	MT* firstMt = mt;
	mtDump(mt);

	MTListItem* mtl = mtlist;
	while(mtl!= NULL) {
		printf("MT null? %d\n", mtl->mt == NULL);
		printf("MT %d\n", mtl->mt->ID);
		mtl = mtl->next;
	}

	/* Cicla sugli input */
	while(1) {
		printf("\n----NEW INPUT---\n");

		/* Parsa il nastro */
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
			// printf("Processing %d\n", mt->ID);

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
 * Main
 */
// int main() {
// 	enum mt_status status = ONGOING;

// 	setup();

// 	/* Parse the MT states, max and acc */
// 	MT* mt = parseMT();
// 	maxMovs = mt->nMovs;
// 	mtDump(mt);

// 	/* Cicla sugli input */
// 	while(1) {
// 		printf("\n----NEW INPUT---\n");

// 		/* Parsa il nastro */
// 		mt->curCell = NULL;
// 		uint32_t tape = parseTape(mt);
// 		//tapeDump(mt);

// 		if (tape == 0) break;

// 		/* Setta stato iniziale e numero di movimenti */
// 		mt->curState = states[0];
// 		nMt = 1;
// 		glob_id = 1;
// 		mt->nMovs = maxMovs;

// 		/* Fai evolvere ogni MT finchè non finisce in uno stato di stop */
// 		status = ONGOING;
// 		while (1) 
// 		{
// 			/* Cicla su tutte le MT */
// 			MTListItem* item = mtlist;
// 			while (item != NULL)
// 			{
// 				/* Evolvi la MT */
// 				status = evolve (item->mt);

// 				/* Se è in uno stato finale distruggi la MT */
// 				if (status == NOT_ACCEPT || status == UNDEFINED) {
// 					// printf("Exit status %d\n", status);
// 					if(nMt >= 1) {
// 						// printf("A) Distruggo MT %d, TOT= %d\n", item->mt->ID, nMt);
// 						item = destroyMt(item->mt->ID);
// 						// printf("Done\n");//printf("B) Distrutta MT %d, TOT= %d\n", item->mt->ID, nMt);
// 					} else {
// 						// printf("Last MT %d, TOT= %d\n", item->mt->ID, nMt);
// 						break;
// 					}
// 				}

// 				// printf("item==NULL?%d\n", item->mt == NULL);
// 				if(item != NULL)
// 					item = item->next;
// 			}

// 			/*  */
// 			// printf("exited status=%d n =%d\n", status, nMt);
// 			if(status == ACCEPT || (status != ONGOING && nMt <= 1))
// 				break;
// 		}

// 		printf("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);
// 		//while ((flush = getchar()) != '\n' && flush != EOF) { }

// 		/* Dealloca il nastro */
// 		destroyTape(mtlist->mt->curCell);

// 		/* Dealloca tutte le MT parallele */
// 		if (nMt > 1){
// 			for (int i = 1; i < nMt; i++) {
// 				destroyMt(i);
// 			}
// 		}

// 		printf("nMovs = %d\n", mt->nMovs);
// 		printf("id della finale=%d\n", mtlist->mt->ID);
// 	}

// 	return 0;
// }

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

// enum mt_status evolve(MT* mt, Transition* tran) {

// 	/* Leggi dal nastro un carattere e prendi la liste delle transizioni corrispondenti */
// 	printf("Evolving MT_%d: reading %c ", mt->ID, mt->curCell->content);
// 	TranList* item = mt->curState->transitions[mt->curCell->content];
	
// 	if (item != NULL) {

// 		/* Checka se ha ancora mosse da fare */
// 		if(mt->nMovs <= 0) {
// 			printf("UNDEFINED %d\n", mt->ID);
// 			return UNDEFINED;
// 		}

// 		/* Cicla su tutte le possibili transizioni */
// 		uint8_t first = 1;
// 		MT* tempMt = copyMt(mt);
// 		do {
// 			Transition* tran = &(item->tran);

// 			/* Check se lo stato prossimo è NOT ACCEPT(errore) o ACCEPT */
// 			if (tran->nextState->status != ONGOING) {
// 			 	return tran->nextState->status;
// 			}

// 			if (first == 1)
// 			{
// 				/* Scrivi sul nastro e muoviti */
// 				mt->curCell->content = tran->output;
// 				moveCell(mt, tran->mov);

// 				/* Cambia lo stato corrente */
// 				printf("nextState: %d\n", tran->nextState->id);
// 				mt->curState = tran->nextState;
// 				mt->nMovs--;

// 				first = 0;
// 			} 
// 			else {
// 				MT* temp = copyMtAndIncrease(tempMt);
// 				printf("created new mt", tran->nextState->id);
// 				// mtDump(temp);
// 				// tapeDump(temp);
// 			}

// 			item = item->next;
// 		} while (item != NULL);
// 		free(tempMt);
// 	} 
// 	else {
// 		return NOT_ACCEPT;
// 	}

// 	return mt->curState->status;
// }

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
		if(mov == L) {
			nextCell = newCell (id, NULL, curCell, (nextCell == NULL) ? BLANK : nextCell->content);
		} else if (mov == R) {
			nextCell = newCell (id, curCell, NULL, (nextCell == NULL) ? BLANK : nextCell->content);
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