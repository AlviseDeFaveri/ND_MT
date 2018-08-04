# READ ME

### Multithread

Ogni cella del nastro ha un proprietario: prima di scrivere sulla cella, controlli che sia tua. Se non lo è, la
aggiungi al tuo nastro.

Il nastro è una (double ?) linked list. La MT viene inizializzata con un nastro: ogni volta che la cella L/R è
NULL o di un altro, crei una nuova cella.

Ogni volta che c'è più di una transizione, crei una nuova MT che ha come puntatore al nastro la cella attuale del nastro.

Transizione: f: input --> output, movimento.
Può essere realizzata come una f che cerca su una lista/hashmap il valore di input

## TODO

* free memory
* hashmap degli stati
* hashmap delle MT
X array di liste di transizioni
* max movs leggerli dal file