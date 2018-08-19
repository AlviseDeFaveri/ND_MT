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
	//TRACE("STARTED\n");

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
	//TRACE("Destroyed all MTs\n");

	/* Crea la prima MT */
	mtlist = newMT(1, maxMovs, globTape, states[0]);
	// TRACE("First MT\n");

	/* Setta stato iniziale e numero di movimenti */
	nMt = 1;
	glob_id = 2; // ID del nastro = 0, ID prima MT = 1
}

/* Rimuove una MT dalla mtlist */
MTListItem* removeFromList(MTListItem* stopped, enum mt_status status) {
	if(stopped != NULL) 
	{
		nMt--;
		stopped->mt.result = status;
		// if(stopped == mtlist) {
		// 	//TRACE("Replacing (first): MT_%d ",  mtlist->mt.ID);
		// 	/// //TRACE("---Replacing first MT-----\n\told first: %d\n\t", mtlist->mt.ID);
		// 	mtlist = stopped->next;
		// 	//TRACE("with MT_%d\n", (mtlist == NULL) ? -1 : mtlist->mt.ID);
		// 	destroyMt(stopped);
		// 	return mtlist;
		// }
		// else {
		// 	assert(prev != NULL);
		// 	TRACE("Replacing: MT_%d ", stopped->mt.ID);

		// 	prev->next = stopped->next;
		// 	TRACE("with MT_%d\n", (prev->next == NULL) ? -1 : prev->next->mt.ID);

		// 	destroyMt(stopped);

		// 	TRACE("Prev= MT_%d, Prev->Next = %d\n", prev->mt.ID, 
	 // 				(prev->next == NULL) ? -1 : prev->next->mt.ID);
		// 	return prev;
		// }
	}
}

/**
 * Process tape until acceptance or finish
 */
enum mt_status processInput(Tape_cell* tape) {
	enum mt_status status = ONGOING;
	MTListItem* mtItem = mtlist;
	MTListItem* nextItem = NULL;

	/* Pulisci e riinizializza mtlist */
	initMtlist();
	//mtlist->mt.curCell = globTape;

	/* Evolvi tutte le macchine finchè almeno una non accetta o non sono finite */
	while (status != ACCEPT && nMt > 0) 
	{
		#ifndef NOTRACE
			TRACE("\nNuovo giro di MT (tot:%d): ",  nMt);

			MTListItem* a = mtlist;
			while(a != NULL) 
			{
				TRACE("MT_%d  ", a->mt.ID);
				a = a->next;
			}

			TRACE("\n");
		#endif

		mtItem = mtlist;

		/* Per ogni MT */
		while (mtItem != NULL && mtlist != NULL) 
		{
			MT* mt = &(mtItem->mt);
			nextItem = mtItem->next;

			TRACE("\nProcessing: %d\n", mt->ID);
			assert(mt != NULL);
			assert(mt->curCell != NULL);
			assert(mt->curState != NULL);

			/* Se e' ferma, saltala */
			if(mt->result != ONGOING){
				mtItem = nextItem;
				continue;
			}

			/// TRACE("Processing: %d, state: %d input %c\n", mt->ID, mt->curState->id, mt->curCell->content);

			/* Leggi e trova la lista di transizioni */
			char c = mt->curCell->content;
			TranListItem* curTran = mt->curState->tranList[(uint8_t)c];

			/* Se non ce n'è neanche una, questa macchina non accetta */
			if(curTran == NULL) 
			{
				///TRACE("Processing MT_%d: no transitions\n", mt->ID);
				/* Se è l'ultima ritorna 0 */
				if(nMt <= 1) return NOT_ACCEPT;
				///TRACE("mtItem %d, prevItem %d\n", mtItem, prevItem);
				removeFromList(mtItem, NOT_ACCEPT);
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
				switch(status) 
				{
					case ACCEPT: 
						return ACCEPT;
						break;
					case NOT_ACCEPT:
					case UNDEFINED:
						if(nMt <= 1) return status;
						removeFromList(mtItem, status);
						break;
				}
				/// //TRACE("Finished transitions\n");
			}
			// prevItem = mtItem;
			mtItem = nextItem;
			// TRACE("Next MT: mtItem %d\n", (mtItem == NULL) ? -1 : mtItem->mt.ID);
		}
		/// //TRACE("Finished MTs\n");
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
		TRACE("---------------------------------------UNDEFINED\n");
		return UNDEFINED;
	}

	/* La transizione accetta */
	if(tran->nextState->accept == TRUE) {
		return ACCEPT;
	} 
	/* Evolvi davvero la MT */
	else {
		//TRACE("Evolving MT_%d: state %d, reading %c -> state %d\n", mt->ID, mt->curState->id, mt->curCell->content, tran->nextState->id);

		/* Cambia stato */
		mt->curState = tran->nextState;
		mt->curCell->content = tran->output;
		Tape_cell* nextCell = moveCell(mt->ID, mt->curCell, tran->mov);
		assert(mt->curCell->owner == mt->ID);
		assert(mt->curCell != NULL);

		if(tran->mov == L)
			mt->curCell->prev = nextCell;
		else if(tran->mov == R)
			mt->curCell->next = nextCell;

		mt->curCell = nextCell;
		// //TRACE("moving cell, mt %d new cell %c curState%d\n", mt->ID, mt->curCell->content, mt->curState->id);

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
			nextCell = newCell (id, nextNextCell, curCell, 
				(nextCell == NULL) ? BLANK : nextCell->content);
		}
		else if(mov == R) {
			nextCell = newCell (id, curCell, nextNextCell, 
				(nextCell == NULL) ? BLANK : nextCell->content);
		}
	} 

	return nextCell;
}

/**
 * Stampa la MT
 */
void mtDump(MT* mt) {

	TRACE("----------- MT ---------------\n");
	
	if (mt == NULL) {
		TRACE("MT is NULL!\n");
	} 
	else {
		TRACE("MT {\n");
		TRACE("\tid %d\n", mt->ID);

		State* state = mt->curState;
		TRACE("\tSTATES {\n");

		for (int i = 0; i < N_TRAN; i++) 
		{

			if (states[i] == NULL)
				break;

			state = states[i];
			TRACE("\n\t\tQ%d", state->id);
			if(state->accept)
				TRACE(" (FINAL!)\n");
			else
				TRACE("\n");

			TRACE("\t\tTRANSITIONS {\n");

			for (int i = 0; i < N_TRAN; i++) 
			{
				if(state->tranList[i] != NULL) {
					TranListItem* item = state->tranList[i];

					do {
						TRACE( "\t\t\t%c-> Q%d (%c), mov %d \n", 
									i, item->tran.nextState->id,
									item->tran.output, item->tran.mov);
						item = item->next;
					} while(item != NULL);
				}
			}

			TRACE("\t\t}\n");
		}
		TRACE("\t}\n");
		TRACE("}\n");
	}

	TRACE("----------------------------\n\n");
}

void tapeDump(Tape_cell* firstCell, const int dir) {

	Tape_cell* curCell = firstCell;

	if (firstCell == NULL)
	{
		return;
	}

	printf("\n ");
	while(curCell != NULL) {
		printf("___ ");
		if(dir == 0)
			break;
		else if(curCell != NULL)
			curCell = (dir > 0) ? curCell->next : curCell->prev;
		else 
			printf("LA MADONNA E' TROIA\n");
	}

	curCell = firstCell;

	printf("\n|");
	while(curCell != NULL) {
		printf(" %d |", curCell->owner);

		if(dir == 0)
			break;
		else
			curCell = (dir > 0) ? curCell->next : curCell->prev;
	}

	curCell = firstCell;

	printf("\n|");
	while(curCell != NULL) {
		printf(" %c |", curCell->content);

		if(dir == 0)
			break;
		else
			curCell = (dir > 0) ? curCell->next : curCell->prev;
	}

	curCell = firstCell;

	printf("\n|");
	while(curCell != NULL) {
		printf("___|", curCell->content);

		if(dir == 0)
			break;
		else
			curCell = (dir > 0) ? curCell->next : curCell->prev;
	}

	printf("\n\n");

	// TRACE("\tCells {\n");

	// while(cell != NULL) {
	// 	TRACE("\t\tCell {\n");
	// 	TRACE("\t\t\taddress: %x\n", cell);
	// 	TRACE("\t\t\towner: %d\n", cell->owner);
	// 	TRACE("\t\t\tcontent: %c\n", cell->content);
	// 	TRACE("\t\t\tprev: %x\n", cell->prev);
	// 	TRACE("\t\t\tnext: %x\n", cell->next);
	// 	TRACE("\t\t}\n");
	// 	cell = cell->next;
	// }

	// TRACE("\t}\n");
}

void list() {
	MTListItem* curMt = mtlist;

	TRACE("------------ MTList ---------------\n\n");
	while(curMt != NULL) {
		MT* mt = &(curMt->mt);
		TRACE("MT %d: STATUS%d, STATE Q%d, TAPE \'%c\', nMOVS=%d\n", mt->result,
				mt->ID, mt->curState->id, mt->curCell->content, mt->nMovs);
		curMt = curMt->next;
	}
	TRACE("\n-----------------------------------\n");
}

void mtape(int id) {
	MTListItem* curMt = mtlist;

	while(curMt != NULL && curMt->mt.ID != id) {
		curMt = curMt->next;
	}

	if (curMt == NULL)
	{
		TRACE("ID not found\n");
	} else {
		MT* mt = &(curMt->mt);
		Tape_cell* curCell = curMt->mt.curCell;

		if(curCell == NULL) {
			printf("MT%d has no curcell\n", curMt->mt.ID);
			return;
		}

		printf("BEFORE:\n");
		tapeDump(curCell->prev, -1);
		printf("\nCURRENT:\n");
		tapeDump(curCell, 0);
		printf("\nAFTER:\n");
		tapeDump(curCell->next, 1);

		TRACE("MT %d: STATUS%d, STATE Q%d, TAPE \'%c\', nMOVS=%d\n\n", mt->result,
				mt->ID, mt->curState->id, mt->curCell->content, mt->nMovs);

	}
}

// MT 2: STATE Q1, TAPE 'c', nMOVS=5
//  ___ ___ ___ ___ 
// | 1 | 1 | 2 | 0 |
// | a | b | c | b |
// |___|___|___|___|
//          ^^^