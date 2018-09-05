#include <stdio.h>
#include "mttypes.h"

/**********************
 *  Global Variables  *
 **********************/
MTListItem* mtlist = NULL;
uint32_t nMt = 0;
uint32_t glob_id = 0;

/**********************
 *  Function Headers  *
 **********************/
void initMtList();
void cleanMtList();

// MTListItem* removeFromList(MTListItem* stopped, enum mt_status status);
enum mt_status processTape(Tape_cell* tape);
enum mt_status branch(MT* originalMt, TranListItem* tranList);
enum mt_status evolve(MT* mt, Transition* tran);
Tape_cell* moveCell(uint32_t id, Tape_cell* curCell, enum test_mov mov);

/*********************
 *  Function Bodies  *
 *********************/

/* Crea mtlist con la prima MT*/
void initMtList() {
	/* Pulisce mtlist */
	if(mtlist != NULL) {
		cleanMtList();
	}

	/* Crea la prima MT */
	mtlist = newMT(1, maxMovs, globTape, states[0]);
	// TRACE("First MT\n");

	/* Setta stato iniziale e numero di movimenti */
	nMt = 1;
	glob_id = 2; // ID del nastro = 0, ID prima MT = 1
}

/* Pulisce mtlist */
void cleanMtList() {
	MTListItem* mtItem = mtlist;
	MTListItem* next = NULL;

	/* Dealloca tutte le MT parallele */
	while (mtItem != NULL) {
		next = mtItem->next;

		destroyMt(mtItem);
		mtItem = next;
	}

	mtlist = NULL;
}

/**
 * Process tape until acceptance or finish
 */
enum mt_status processTape(Tape_cell* tape) {
	enum mt_status status = ONGOING;
	MTListItem* mtItem = mtlist;
	MTListItem* prevItem = NULL;
	MTListItem* nextItem = NULL;

	/* Pulisci e riinizializza mtlist */
	initMtList();
	mtlist->mt.curCell = globTape;

	/* Evolvi tutte le macchine finchè non sono finite */
	while (nMt > 0) // TODO: perche' non basta nMt?
	{
		#ifndef WITHTRACE
			/* Printa tutte le mt */
			TRACE("\nNuovo giro di MT (tot:%d)\n",  nMt);
		#endif

		mtItem = mtlist;
		prevItem = NULL;

		/* Scorri tutte le MT */
		while (mtItem != NULL) 
		{
			MT* mt = &(mtItem->mt);

			TRACE("\nProcessing: %d\n", mt->ID);
			assert(mt != NULL);
			assert(mt->curCell != NULL);
			assert(mt->curState != NULL);

			/* Se e' ferma saltala */
			if(mt->result != ONGOING){
				mtItem = mtItem->next;
				TRACE("[MT_%d] Already stopped with status %d\n", mt->ID, mt->result);
				continue;
			}

			// TRACE("Processing: %d, state: %d input %c\n", mt->ID, mt->curState->id, mt->curCell->content);

			/* Leggi dal nastro */
			char c = mt->curCell->content;

			/* Trova la lista di transizioni */
			TranListItem* curTran = mt->curState->tranList[(uint8_t)c];

			/* Crea una MT per ogni transizione */
			status = branch(mt, curTran);

			/* Se accetta esci, se non accetta distruggila */
			if(status == ACCEPT) {
				return ACCEPT;
			}

			mtItem = mtItem->next;
		}
		// TRACE("Finished MTs\n");
	}

	cleanMtList();
	return status;
}

/* 
 * Crea nuove MT a partire dall'originale leggendo la lista di transizioni,
 * le fa evolvere e aggiorna la mtlist (se non è terminata).
 *
 * Ritorna: ACCEPT appena una accetta.
 *          ONGOING se almeno uno è ongoing.
 *          UNDEFINED se almeno una ha finito le mosse e nessuna è ongoing.
 *          NOT_ACCEPT se tutte non accettano.
 */
enum mt_status branch(MT* originalMt, TranListItem* tranList) 
{
	/* Nessuna transizione: la macchina si ferma */
	if(tranList == NULL){
		originalMt->result = NOT_ACCEPT;
		nMt--;
		return NOT_ACCEPT;
	}
	/* Una sola transizione: evolvi */
	else if(tranList->next == NULL) {
		enum mt_status evolveStatus = evolve(originalMt, &(tranList->tran));
		if(evolveStatus != ONGOING) {
			nMt--;
		}
		return evolveStatus;
	}

	/* Transizioni multiple: stoppa quella corrente */
	originalMt->result = NOT_ACCEPT;
	nMt--;

	/* Cicla su tutte le transizioni parallele */
	TranListItem* tranItem = tranList;
	enum mt_status retStatus = NOT_ACCEPT;

	while(tranItem != NULL) 
	{
		assert(tranItem->tran.nextState != NULL);

		/* Se il prossimo stato accetta, ritorna subito */
		if(tranItem->tran.nextState->accept == TRUE) {
			originalMt->result = ACCEPT;
			return ACCEPT;
		}
		else {
			/* Crea nuova MT */
			MTListItem* newMt = newMT(glob_id, originalMt->nMovs, originalMt->curCell,
										originalMt->curState);

			/* Evolvi nuova MT */
			enum mt_status evolveStatus = evolve(&(newMt->mt), &(tranItem->tran));

			/* Se non è morta, aggiungila a MTList */
			switch(evolveStatus) {
				case ACCEPT:
					/* Ritorna subito */
					destroyMt(newMt);
					return ACCEPT;
					break;
				case ONGOING:
					/* Aggiungila in testa a mtlist */
					newMt->next = mtlist;
					mtlist = newMt;
					/* Aggiorna variabili globali */
					nMt++;
					glob_id++;
					retStatus = ONGOING;
					break;
				case UNDEFINED:
					/* Almeno una è indefinita: eliminala */
					if(retStatus == NOT_ACCEPT) {
						retStatus = UNDEFINED;
					}
					destroyMt(newMt);
					break;
				case NOT_ACCEPT:
					/* Eliminala subito */
					destroyMt(newMt);
					break;
			}
		}

		tranItem = tranItem->next;
	}

	return retStatus;
}

/**
 * Evolve MT: scrivi carattere, muovi la testina, cambia stato e decrementa il numero di mosse
 */
enum mt_status evolve(MT* mt, Transition* tran) {
	TRACE("Evolving MT_%d", mt->ID);
	assert(mt != NULL);
	
	/* Nessuna transizione */
	if(tran == NULL) {
		mt->result = NOT_ACCEPT;
	}
	/* Mosse finite */
	else if (mt->nMovs == 0) {
		mt->result = UNDEFINED;
	}
	/* La transizione accetta */
	else if(tran->nextState->accept == TRUE) {
		mt->result = ACCEPT;
	} 
	/* Evolvi davvero la MT */
	else {
		TRACE("Evolving MT_%d: state %d, reading %c writing %c -> state %d\n", mt->ID, mt->curState->id, mt->curCell->content, tran->output, tran->nextState->id);

		/* Cambia stato */
		mt->curState = tran->nextState;
		mt->curCell->content = tran->output;
		mt->curCell = moveCell(mt->ID, mt->curCell, tran->mov);
		assert(mt->curCell != NULL);
		// TRACE("moving cell, mt %d new cell %c curState%d\n", mt->ID, mt->curCell->content, mt->curState->id);

		mt->nMovs--;
		mt->result = ONGOING;
	}

	return mt->result;
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
			curCell->prev = nextCell;
		}
		else if(mov == R) {
			nextCell = newCell (id, curCell, nextNextCell, (nextCell == NULL) ? BLANK : nextCell->content);
			curCell->next = nextCell;
		}
	} 

	return nextCell;
}