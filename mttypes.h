#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define BLANK '_'
#define N_TRAN 256
#define N_STATES 256

enum test_mov {S, L, R};
enum mt_status {ONGOING, ACCEPT, NOT_ACCEPT, UNDEFINED};

/**
 * Transizione:
 */
typedef struct Transition 
{
	char output;
	enum test_mov mov;
	struct State* nextState;
} Transition;

/**
 * Transizione:
 */
typedef struct TranList 
{
	Transition tran;
	struct TranList* next;
} TranList;

/**
 * Stato: ha un'array di 256 liste di transizioni (ogni lista->1 char di input)
 */
typedef struct State
{
	struct TranList* transitions[N_TRAN]; // TODO array di liste
	enum mt_status status;
	int id;
} State;

/**
 * Nastro: double linked list di celle, ognuna ha un proprietario
 */
typedef struct Tape_cell 
{
	uint32_t owner;
	struct Tape_cell* prev;
	struct Tape_cell* next;
	char content;
} Tape_cell;

/**
 * MT: ha un array di stati (hashmap) e la linked list di celle
 */
typedef struct MT
{
	uint32_t ID;
	uint32_t nMovs;
	struct Tape_cell* curCell; // testina del nastro
	struct State* curState;
} MT;

/**
 * MT: ha un array di stati (hashmap) e la linked list di celle
 */
typedef struct MTListItem
{
	MT* mt;
	struct MTListItem* next;
} MTListItem;


/* GLOBAL VARIABLES */
uint32_t nMt = 0;
uint32_t glob_id = 0;
MTListItem* mtlist = NULL;
State* states[N_STATES];  // TODO Hashmap per la ricerca dello stato prossimo (in creazione)
int maxMovs = 0;

/**
 * Costruttore per gli stati (NON accettato di default)
 */
State* newState(int id) {
	State* state = malloc(sizeof(State));

	int i = 0;
	for(i = 0; i < N_TRAN; i++){
		state->transitions[i] = NULL;
	}
	state->id = id;
	state->status = NOT_ACCEPT; // cambia se ne aggiungi qualcuna
	return state;
}

/**
 * Costruttore per le transizioni (trasforma stati non accettanti in transitori)
 */
void addTran(State* state, const char input, const char output,
				 const enum test_mov mov, struct State* nextState) {

	TranList* listItem = malloc(sizeof(TranList)); // &(state->transitions[input]);
	Transition* tran = &(listItem->tran);

	tran->output = output;
	tran->mov = mov;
	tran->nextState = nextState;

	if(state->status == NOT_ACCEPT) {
		state->status = ONGOING;
	}

	/* Add tran to state */
	TranList* freeSpace = state->transitions[input];
	if(freeSpace == NULL) {
		state->transitions[input] = listItem;
		listItem->next = NULL;
	} else {
		while(freeSpace->next != NULL) {
			freeSpace = freeSpace->next;
		}
		freeSpace->next = listItem;
		listItem->next = NULL;
	}
}

/**
 * Costruttore delle celle del nastro
 */
Tape_cell* newCell(const uint32_t owner, struct Tape_cell* prev, 
						struct Tape_cell* next, const char content) {
	Tape_cell* cell = malloc(sizeof(Tape_cell));

	cell->owner = owner;
	cell->prev= prev;
	cell->next = next;
	cell->content = content;
	return cell;
}

void destroyTape(Tape_cell* cell) {
	Tape_cell* initcell = cell;
	Tape_cell* cur = cell->next;
	int i = 0;

	// printf("Avanti\n");
	while(cur != NULL && cur->next != NULL) {
		cur = cur->next;
		// printf("%c \n", cur->prev->content);
		free(cur->prev);
		i++;
	}

	if(cur != NULL) {
		// printf("%c \n", cur->content);
		free(cur);
	}

	// printf("Indietro\n");
	cur = initcell;
	while(cur->prev != NULL) {
		cur = cur->prev;
		// printf("%c \n", cur->next->content);
		free(cur->next);
		i++;
	}
	if(cur != NULL) {
		// printf("%c \n", cur->content);
		free(cur);
	}

	// printf("\nDestroyed %d cells\n", i);
}

/**
 * Costruttore della MT
 */
MT* newMT() {

	/* Crea MT */
	MT* mt = malloc(sizeof(MT));
	// printf("Created new MT, TOT=%d\n", nMt+1);

	/* Crea listItem */
	MTListItem* mtItem = malloc(sizeof(MTListItem));
	mtItem->mt = mt;
	mtItem->next = NULL;

	/* Mettila nel primo posto libero */
	MTListItem* item = mtlist;
	// printf("MTList[0] = NULL? %d\n", mtlist == NULL);
	if(mtlist == NULL){
		mtlist = mtItem;
	} 
	else {
		while(item->next != NULL) {
			item = item->next;
		}
		item->next = mtItem;
	}
	// printf("MTList[0] = NULL? %d\n", mtlist == NULL);

	/* Default settings */
	mt->ID = glob_id++;
	nMt++;
	// printf("Created new, TOT=%d\n", nMt);

	mt->nMovs = 1;
	mt->curCell = NULL;
	mt->curState = NULL;

	return mt;
}

/**
 * Crea nuova MT incrementando l'id, con stessa testina, stesso stato attuale
 * e stesso numero di mosse.
 */
MT* copyMtAndIncrease(MT* oldMt) {
	if(oldMt != NULL) {
		MT* mt = newMT();

		mt->nMovs = oldMt->nMovs;
		mt->curCell = oldMt->curCell;
		mt->curState = oldMt->curState;

		return mt;
	}

	return NULL;
}

MT* copyMt(MT* oldMt) {
	if(oldMt != NULL) {
		MT* mt = malloc(sizeof(MT));

		mt->ID = oldMt->ID;

		mt->nMovs = oldMt->nMovs;
		mt->curCell = oldMt->curCell;
		mt->curState = oldMt->curState;

		return mt;
	}

	return NULL;
}

MTListItem* destroyMt(int id) {
	MTListItem* prev = mtlist;
	MTListItem* cur = NULL;

	if(mtlist == NULL) {
		printf("No MTs\n");
		return NULL;
	}

	MTListItem* mtl = mtlist;
	printf("MTLIST (TOT=%d):\n", nMt);
	while(mtl!= NULL) {
		printf("MT %d\n", mtl->mt->ID);
		mtl = mtl->next;
	}

	printf("Destroying %d == %d, N=%d\n", id, mtlist->mt->ID, nMt);

	if(nMt <= 1){
		assert(id == mtlist->mt->ID);
		mtlist->next = NULL;
		return mtlist;
	}

	/* Se è il primo cambia mtlist */
	if(mtlist->mt->ID == id) {
		printf("trovata Prima MT\n");
		cur = mtlist;
		mtlist = prev->next;
		printf("%d, %x\n", mtlist->mt->ID, mtlist->next);
	} 
	else {

		/* Assegna il cur al secondo elemento */
		cur = prev->next;
		// printf("prev %d cur %d\n", prev->mt->ID, cur->mt->ID);

		/* Cerca l'item con l'id selezionato */
		while(cur != NULL && cur->mt->ID != id) {
			/* Fai avanzare prev e next */
			prev = cur;
			cur = cur->next;
			// printf("prev %d cur %d\n", prev->mt->ID, cur == NULL ? -1 : cur->mt->ID);
		}

		/* Assegna il next del prev al prossimo */
		// printf("Substitute %d <- %d\n", prev->next->mt->ID, cur->next == NULL ? -1 : cur->next->mt->ID);
		if(cur != NULL && cur->next != NULL) {
			prev->next = cur->next;
		} else {
			prev->next = NULL;
		}
	}

	/* Libera il puntatore */
	free(cur);
	nMt--;

	mtl = mtlist;
	printf("MTLIST (TOT=%d):\n", nMt);
	while(mtl!= NULL) {
		printf("MT %d\n", mtl->mt->ID);
		mtl = mtl->next;
	}

	return prev;
}