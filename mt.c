#include <stdio.h>
#include "mtparser.h"

void moveCell(MT* mt, enum test_mov mov);
enum mt_status evolve(MT* mt);
void mtDump(MT* mt);
void tapeDump(MT* mt);

/**
 * Main
 */
int main() {

	enum mt_status status = ONGOING;
	printf("STARTED\n");

	for(int i = 0; i < N_STATES; i++){
		states[i] = NULL;
	}

	MT* mt = parseMT();
	printf("\n-------- MT -----------\n");
	mtDump(mt);
	printf("-----------------------\n");

	uint32_t tape; // = parseTape(mt);
	// printf("\n----NEW INPUT---\n");
	// tapeDump(mt);

	while(1) {

		mt->curCell = NULL;
		tape = parseTape(mt);

		if (tape == 0) break;

		mt->curState = states[0];
		mt->nMovs = 10; // TODO read max movs

		printf("\n----NEW INPUT---\n");
		//tapeDump(mt);
		char flush;

		/* Fai evolvere ogni MT finchè non finisce in uno stato di stop */
		status = ONGOING;
		while (1) {

			/* Cicla su tutte le MT */
			MTListItem* item = mtlist;
			int i = 0;

			while (item != NULL) {
				/* Evolvi la MT */
				status = evolve(item->mt);

				/* Se è in uno stato finale distruggi la MT */
				if (status == NOT_ACCEPT || status == UNDEFINED) {
					printf("Exit status %d\n", status);
					if(nMt > 1) {
						destroyMt(i);
						printf("Distruggo MT %d, TOT= %d\n", i, nMt);
					} else {
						printf("Last MT %d, TOT= %d\n", i, nMt);
						break;
					}
				}

				item = item->next;
				i++;
			}
			printf("exited status=%d n =%d\n", status, nMt);
			if(status == ACCEPT || status != ONGOING && nMt <= 1)
				break;
		}

		printf("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);
		//while ((flush = getchar()) != '\n' && flush != EOF) { }

		/* Dealloca il nastro */
		destroyTape(mtlist->mt->curCell);

		/* Dealloca tutte le MT parallele */
		if (nMt > 1){
			for (int i = 1; i < nMt; i++) {
				destroyMt(i);
			}
		}

		nMt = 1;
	}

	return 0;
}

/**
 * Evolve MT: cerca transizione, scrivi carattere, muovi la testina, cambia stato e decrementa il numero di mosse
 */
enum mt_status evolve(MT* mt) {

	/* Leggi dal nastro un carattere e prendi la liste delle transizioni corrispondenti */
	printf("Evolving MT_%d: reading %c\n", mt->ID, mt->curCell->content);
	TranList* item = mt->curState->transitions[mt->curCell->content];
	
	if (item != NULL) {
		uint8_t first = 1;

		/* Checka se ha ancora mosse da fare */
		if(mt->nMovs <= 0) {
			printf("UNDEFINED\n");
			return UNDEFINED;
		}

		/* Cicla su tutte le possibili transizioni */
		do {
			Transition* tran = &(item->tran);

			/* Check se lo stato prossimo è NOT ACCEPT(errore) o ACCEPT */
			if (tran->nextState->status != ONGOING) {
				return tran->nextState->status;
			}

			MT* tempMt = copyMt(mt);

			if (first == 1)
			{
				/* Scrivi sul nastro e muoviti */
				mt->curCell->content = tran->output;
				moveCell(mt, tran->mov);

				/* Cambia lo stato corrente */
				mt->curState = tran->nextState;
				mt->nMovs--;

				first = 0;
			} 
			else {
				MT* temp = copyMtAndIncrease(tempMt);
				// mtDump(temp);
				// tapeDump(temp);
			}

			free(tempMt);
			item = item->next;
		} while (item != NULL);
	} 
	else {
		return NOT_ACCEPT;
	}

	return mt->curState->status;
}

/**
 * Muove la testina (creando la cella se ce n'è bisogno)
 */
void moveCell(MT* mt, enum test_mov mov) {
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

/**
 * Stampa la MT
 */
void mtDump(MT* mt) {
	
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
}

void tapeDump(MT* mt) {
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