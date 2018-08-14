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
		printf("\n----NEW INPUT---\n");

		/* Dealloca il nastro */
		destroyTape(globTape, 0);
		printf("Tape destroyed\n");
		
		/* Parsa il nastro */
		uint32_t tape = parseTape();
		if (tape == 0) break;
		// tapeDump();

		/* Pulisci e riinizializza mtlist */
		initMtlist();

		/* Processa l'input */
		enum mt_status status = processInput(mt->curCell);
		printf("FINISHED with acceptance = %d\n",  (status == ACCEPT) ? 1 : 0);
		// while ((flush = getchar()) != '\n' && flush != EOF) { }
		
	}
}

// tr
// 0 a a R 0
// 0 b b R 0
// 0 a c R 1
// 0 b c R 2
// 1 a c L 3
// 2 b c L 3
// 3 c c L 3
// 3 a c R 4
// 3 b c R 5
// 4 c c R 4
// 4 a c L 3
// 5 c c R 5
// 5 b c L 3
// 3 _ _ R 6
// 6 c c R 6
// 6 _ _ S 7
// acc
// 7
// max
// 800
// run
// aa
// aababbabaa
// aababbabaaaababbabaa
// aababbabaaaababbabaab
// aababbabaaaababbabaabbaababbabaaaababbabaa
// aababbabbaaababbabaabbaababbabaaaababbabaa