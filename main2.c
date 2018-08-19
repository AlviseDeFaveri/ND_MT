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
	mtDump();

	/* Cicla sugli input */
	while(1) {
		TRACE("\n----NEW INPUT---\n");

		/* Dealloca il nastro */
		destroyTape(globTape, 0);
		TRACE("Tape destroyed\n");
		
		/* Parsa il nastro */
		uint32_t tape = parseTape();
		if (tape == 0) break;
		// tapeDump();

		/* Pulisci e riinizializza mtlist */
		initMtlist();

		/* Processa l'input */
		enum mt_status status = processInput(mt->curCell);
		TRACE("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);
		// while ((flush = getchar()) != '\n' && flush != EOF) { }
		
	}
}

// aababbabaa 0
// aababbabaaaababbabaa U
// aababbabaaaababbabaab 0
// aababbabaaaababbabaabbaababbabaaaababbabaa
// aababbabbaaababbabaabbaababbabaaaababbabaa