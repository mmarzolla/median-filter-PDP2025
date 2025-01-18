/****************************************************************************
 *
 * hist.h -- Istogram interface
 *
 * Copyright (C) 2024 Moreno Marzolla
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#ifndef HIST_H
#define HIST_H

#include <stdint.h>
#include "common.h"

typedef struct Hist Hist;

/* Restituisce un nuovo istogramma inizialmente vuoto */
Hist *hist_create( void );

/* Svuota l'istogramma. */
void hist_clear(Hist *H);

/* Distrugge l'istogramma e il suo contenuto, liberando tutta la
   memoria occupata dalla struttura `H`. Dopo aver chiamato questa
   funzione il puntatore `H` punta ad un blocco di memoria non più
   valido, quindi non deve essere dereferenziato. */
void hist_destroy(Hist *H);

/* Aggiunge c>=0 occorrenze del valore `k` in `H`. */
void hist_insert(Hist *H, data_t k, int c);

/* Restituisce il numero di occorrenze del valore `k`
   nell'istogramma. */
int hist_get(const Hist *H, data_t k);

/* Cancella `c` occorrenze del valore `k` da `H`. Devono essere
   presenti almeno `c` occorrenze di `k`. */
void hist_delete(Hist *H, data_t k, int c);

/* Restituisce `true` (un valore diverso da zero) se l'istogramma è
   vuoto, 0 altrimenti */
int hist_is_empty(const Hist *H);

/* Stampa a video il contenuto dell'istogramma `H`. */
void hist_print(const Hist *H);

/* Stampa a video il contenuto dell'albero `H` in modo più "leggibile". */
void hist_pretty_print(const Hist *H);

/* Add the content of h2 to h1 */
void hist_add(Hist *H1, const Hist *H2);

/* Remove the content of h2 from h1; note that all elements of h2 must
   be in h1 with at least the same number of occurrences. */
void hist_sub(Hist *H1, const Hist *H2);

/* Restituisce il valore mediano presente in `H`; l'istogramma non
   deve essere vuoto. */
data_t hist_median(const Hist *H);

#endif
