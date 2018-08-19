#include <stdio.h>
#include "mtparser.h"
#include "mtevolve.h"

/**
 * Main
 */
int main() {
	enum mt_status status = ONGOING;

	/* Parsa stati e transizioni */
	initStateHashmap();
	parseTranAndStates();
	// initMtlist();
	// mtDump(&(mtlist->mt));

	/* Cicla sugli input */
	while(1) {

		/* Parsa il nastro */
		uint32_t tape = parseTape();
		if (tape == 0) break;

		printf("\n----NEW INPUT---\n");
		// tapeDump(globTape);

		/* Processa l'input */
		status = processInput(globTape);
		printf("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);
		// while ((flush = getchar()) != '\n' && flush != EOF) { }
		
		/* Dealloca il nastro */
		destroyTape(globTape, 0);
		TRACE("Tape destroyed\n");
	}
}