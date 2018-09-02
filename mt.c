#include <stdio.h>
#include "mtparser.h"
#include "mtevolve.h"

/**
 * Main
 */
int main() {
	enum mt_status status = ONGOING;

	/* Parsa stati e transizioni */
	parseMT();

	/* Cicla sugli input */
	while(1) {

		/* Parsa il nastro */
		uint32_t tape = parseTape();
		if (tape == 0) break;

		TRACE("\n----NEW INPUT---\n");
		// tapeDump(globTape);

		/* Processa l'input */
		status = processTape(globTape);

		switch(status){
			case ACCEPT:
				printf("1\n");
				break;
			case UNDEFINED:
				printf("U\n");
				break;
			default:
				printf("0\n");
				break;
		}
		// while ((flush = getchar()) != '\n' && flush != EOF) { }
		
		/* Dealloca il nastro */
		destroyTape(globTape, 0);
		TRACE("Tape destroyed\n");
	}

	// TODO: destroyMT(); ?
}