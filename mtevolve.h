#include <stdio.h>
#include "mttypes.h"

/**
 * Global Variables
 */

MTListItem* mtlist = NULL;
uint32_t nMt = 0;
uint32_t glob_id = 0;

/**
 * Functions
 */

enum bool_t branchAndEvolve(MT* originalMt, TranListItem* tranList);
enum mt_status evolve(MT* mt, Transition* tran);
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov);

/* Inizializza l'hashmap degli stati */
void initStateHashmap() {
	//printf("STARTED\n");

	for(int i = 0; i < N_STATES; i++){
		states[i] = NULL;
	}
}

/* Pulisce mtlist e crea la prima MT*/
void initMtlist() {
	/* Dealloca tutte le MT parallele */
	MTListItem* mtItem = mtlist;
	MTListItem* next = NULL;
	while (mtItem != NULL) {
		next = mtItem->next;

		destroyMt(mtItem);
		mtItem = next;
	}
	//printf("Destroyed all MTs\n");

	/* Crea la prima MT */
	mtlist = newMT(1, maxMovs, globTape, states[0]);
	// printf("First MT\n");

	/* Setta stato iniziale e numero di movimenti */
	nMt = 1;
	glob_id = 2; // ID del nastro = 0, ID prima MT = 1
}

/* Rimuove una MT dalla mtlist */
void removeFromList(MTListItem* stopped, MTListItem* prev) {
	if(stopped != NULL) 
	{
		if(stopped == mtlist) {
			//printf("Replacing (first): MT_%d ",  mtlist->mt.ID);
			/// //printf("---Replacing first MT-----\n\told first: %d\n\t", mtlist->mt.ID);
			mtlist = stopped->next;
			//printf("with MT_%d\n", (mtlist == NULL) ? -1 : mtlist->mt.ID);
			destroyMt(stopped);
			stopped = mtlist;
		}
		else {
			assert(prev != NULL);
			printf("Replacing: MT_%d ", stopped->mt.ID);

			prev->next = stopped->next;
			printf("with MT_%d\n", (prev->next == NULL)? -1 : prev->next->mt.ID);

			destroyMt(stopped);
			stopped = prev->next;
		}
	}
}

/**
 * Process tape until acceptance or finish
 */
enum mt_status processInput(Tape_cell* tape) {
	enum mt_status status = ONGOING;
	MTListItem* mtItem = mtlist;
	MTListItem* prevItem = NULL;
	MTListItem* nextItem = NULL;

	/* Pulisci e riinizializza mtlist */
	initMtlist();
	mtlist->mt.curCell = globTape;

	/* Evolvi tutte le macchine finchè almeno una non accetta o non sono finite */
	while (status != ACCEPT && nMt > 0 && mtlist != NULL) //TODO. perche' non basta nMt
	{
		/* Per ogni MT */
		printf("\nNuovo giro di MT:\n");
		MTListItem* a = mtlist;
		while(a != NULL) {
			printf("MT_%d  ", a->mt.ID);
			a = a->next;
		}
		printf("\n");


		mtItem = mtlist;
		prevItem = NULL;

		while (mtItem != NULL && mtlist != NULL) 
		{
			MT* mt = &(mtItem->mt);
			nextItem = mtItem->next;

			printf("\nProcessing: %d\n", mt->ID);
			assert(mt != NULL);
			assert(mt->curCell != NULL);
			assert(mt->curState != NULL);

			/// //printf("Processing: %d, state: %d input %c\n", mt->ID, mt->curState->id, mt->curCell->content);

			/* Trova la lista di transizioni corrispondenti all'input della testina */
			char c = mt->curCell->content;
			TranListItem* curTran = mt->curState->tranList[(uint8_t)c];

			/* Se non ce n'è neanche una, questa macchina non accetta */
			if(curTran == NULL) {
				//printf("Processing MT_%d: no transitions\n", mt->ID);
				/* Se è l'ultima ritorna 0 */
				if(nMt == 1) return NOT_ACCEPT;
				/// //printf("mtItem %d, prevItem %d\n", mtItem, prevItem);
				removeFromList(mtItem, prevItem);
			}
			else {
				/* Crea una MT per ogni ulteriore transizione */
				enum bool_t accept = branchAndEvolve(mt, curTran->next);

				/* Se almeno una accetta, ritorna true */
				if(accept == TRUE)
					return ACCEPT;

				/* Evolvi la prima */
				status = evolve(mt, &(curTran->tran));

				/* Se accetta esci, se non accetta distruggila */
				switch(status) {
					case ACCEPT: 
						return ACCEPT;
						break;
					case NOT_ACCEPT:
					case UNDEFINED:
						if(nMt == 1) return status;
						removeFromList(mtItem, prevItem);
						break;
				}
				/// //printf("Finished transitions\n");
			}
			prevItem = mtItem;
			mtItem = nextItem;
			/// //printf("Next MT: mtItem %d\n", (mtItem == NULL) ? -1 : mtItem->mt.ID);
		}
		/// //printf("Finished MTs\n");
	}
}

/* 
 * Crea nuove MT a partire dall'originale leggendo la lista di transizioni,
 * le fa evolvere e aggiorna la mtlist (se non è terminata).
 */
enum bool_t branchAndEvolve(MT* originalMt, TranListItem* tranList) {
	TranListItem* tranItem = tranList;

	/* Nessuna transizione da fare */
	if(tranList == NULL || originalMt->nMovs == 0) {
		return FALSE;
	}

	/* Cicla sulla lista di transizioni */
	while(tranItem != NULL) 
	{
		assert(tranItem->tran.nextState != NULL);

		if(tranItem->tran.nextState->accept == TRUE) {
			return TRUE;
		}
		else {
			/* Crea nuova MT */
			MTListItem* mtItem = newMT(glob_id, originalMt->nMovs, originalMt->curCell,
										originalMt->curState);

			/* Evolvi nuova MT */
			enum mt_status retStatus = evolve(&(mtItem->mt), &(tranItem->tran));

			/* Se è terminata distruggi senza aggiungere a mtlist */
			if(retStatus == ACCEPT) {
				destroyMt(mtItem);
				return TRUE;
			}
			else if(retStatus == ONGOING) {
				/* Aggiorna primo elemento di mtlist */
				mtItem->next = mtlist;
				mtlist = mtItem;
				nMt++;
				glob_id++;	
			}
			else {
				destroyMt(mtItem);
			}
		}

		tranItem = tranItem->next;
	}

	return FALSE;
}

/**
 * Evolve MT: scrivi carattere, muovi la testina, cambia stato e decrementa il numero di mosse
 */
enum mt_status evolve(MT* mt, Transition* tran) {
	assert(mt != NULL);
	
	/* Nessuna transizione */
	if(tran == NULL) {
		return NOT_ACCEPT;
	}

	/* Mosse finite */
	if (mt->nMovs == 0) {
		// //printf("UNDEFINED\n");
		return UNDEFINED;
	}

	/* La transizione accetta */
	if(tran->nextState->accept == TRUE) {
		return ACCEPT;
	} 
	/* Evolvi davvero la MT */
	else {
		//printf("Evolving MT_%d: state %d, reading %c -> state %d\n", mt->ID, mt->curState->id, mt->curCell->content, tran->nextState->id);

		/* Cambia stato */
		mt->curState = tran->nextState;
		mt->curCell->content = tran->output;
		mt->curCell = moveCell(mt->ID, mt->curCell, tran->mov);
		assert(mt->curCell != NULL);
		// //printf("moving cell, mt %d new cell %c curState%d\n", mt->ID, mt->curCell->content, mt->curState->id);

		mt->nMovs--;
		return ONGOING;
	}
}

/**
 * Ritorna la prossima cella (creando la cella se ce n'è bisogno)
 */
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov) {
	Tape_cell* nextCell = curCell;
	Tape_cell* nextNextCell = NULL;

	/* Prossima cella */
	if (mov == L) {
		nextCell = curCell->prev;
		if (nextCell != NULL) nextNextCell = nextCell->prev;
	} else if (mov == R) {
		nextCell = curCell->next;
		if (nextCell != NULL) nextNextCell = nextCell->next;
	}

	/* Se la prossima cella ha un ID diverso o non esiste, crea nuova */
	if(nextCell == NULL || nextCell->owner != id) {
		if(mov == L) {
			nextCell = newCell (id, nextNextCell, curCell, (nextCell == NULL) ? BLANK : nextCell->content);
		}
		else if(mov == R) {
			nextCell = newCell (id, curCell, nextNextCell, (nextCell == NULL) ? BLANK : nextCell->content);
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
		printf("\tid %d\n", mt->ID);

		State* state = mt->curState;
		printf("\tSTATES {\n");

		for (int i = 0; i < N_TRAN; i++) 
		{

			if (states[i] == NULL)
				break;

			state = states[i];
			printf("\n\t\tQ%d", state->id);
			if(state->accept)
				printf(" (FINAL!)\n");
			else
				printf("\n");

			printf("\t\tTRANSITIONS {\n");

			for (int i = 0; i < N_TRAN; i++) 
			{
				if(state->tranList[i] != NULL) {
					TranListItem* item = state->tranList[i];

					do {
						printf( "\t\t\t%c-> Q%d (%c), mov %d \n", 
									i, item->tran.nextState->id,
									item->tran.output, item->tran.mov);
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

void tapeDump(Tape_cell* cell) {

	printf("----------- TAPE ---------------\n");
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