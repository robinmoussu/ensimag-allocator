/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"

// Renvoie 2 à la puissance x
#define POW_2(x) (1 << (x))

// Une zone mémoire (voir explications plus bas)
// La taille minimale alouable est celle d'un pointeur sur un autre bloc.
union bloc {
    union bloc *next_record; // Chaine de zone mémoire disponible lorsque la
                             // zone est non allouée
    void       *data;        // Les données lorsque la zone est allouée
};
#define MIN_SIZE_ALLOC sizeof(union bloc)

// L'espace allouable est situé dans le tableau memory_pool, d'une taille de
// ALLOC_MEM_SIZE octets;
//
// Le tableau free_bloc de BUDDY_MAX_INDEX elements pointe sur les zones
// mémoire libres de taille 2 puissance n (avec n <= BUDDY_MAX_INDEX).
//
// Les blocs allouables ont une taille T(n) = 2 puissance n, avec pour
// minimum MIN_SIZE_ALLOC.
//
// Les blocs allouables de taille T(n) sont regroupés dans une liste dont le
// premier élément est free_bloc[n].
//
// Si free_bloc[n] = 0, alors il n'y a pas de blocs de taille T(n) disponible.
// Si free_bloc[n] = A (avec A une adresse différente de 0), alors le bloc
// situé à l'adresse A est un bloc prêt à être alloué. De même, si le champ
// next_record du bloc situé à l'adresse A pointe vers B (avec B != 0), alors
// le bloc situé à l'adresse B est disponible, et ainsi de suite jusqu'au
// dernier élément de la chaine (dont le champ next_record == 0).
//
// Si un bloc est alloué, alors il ne sera plus présent ni dans le tableau
// free_bloc, ni dans aucune des sous-chaines. Dans ce cas, seul
// le champ data du bloc devient utile.
uint8_t    *memory_pool;
union bloc free_bloc[BUDDY_MAX_INDEX];

int mem_init()
{
    if (!memory_pool) {
        memory_pool = (void *) malloc( ALLOC_MEM_SIZE );
    }
    if (memory_pool == 0) {
        perror("Cannot initialise memory\n");
        return -1;
    }

    // À l'initialisation, seul un bloc de de taille maximal (faisant
    // 2 puissance BUDDY_MAX_INDEX octets) est disponible.
    for(int i = 0; i < BUDDY_MAX_INDEX - 1; i++) {
        free_bloc[i].next_record = 0;
    }
    free_bloc[BUDDY_MAX_INDEX - 1].next_record = (union bloc*) memory_pool;
    free_bloc[BUDDY_MAX_INDEX - 1].next_record->next_record = 0;
    return 0;
}

// Retourne l'index de la première cellule de taille T >= size tel que
// 2 puissance k ≤ T < 2 puissance (k+1) dans le tableau memory.free_bloc
int get_index(unsigned long size)
{
    int index;
    for (index = 0; POW_2(index) < size; index++) {
        ;}
    return index - 1;
}

// Retourne un bloc libre de taille T >= size, tel que
// 2 puissance k ≤ T < 2 puissance (k+1)
// Retourne 0 si il n'y a pas d'espace disponible.
void *mem_alloc(unsigned long size)
{
    int index_celulle;

    // On s'assure que la mémoire soit initialisé
    if (memory_pool == 0) {
        perror("Memory not intitialized\n");
        return 0;
    }

    // On s'assure que la taille ne soit pas nulle
    if (size == 0) {
        perror("Request of 0 byte allocation\n");
        return 0;
    }
    if (size < MIN_SIZE_ALLOC) {
        size = MIN_SIZE_ALLOC;
    }

    index_celulle = get_index(size);
    // On s'assure que la taille demandé soit valide
    if (index_celulle == BUDDY_MAX_INDEX) {
        return 0;
    }

    // Cas 1, un bloc de taille T existe
    if (free_bloc[index_celulle].next_record != 0) {
        union bloc selected_bloc = free_bloc[index_celulle];
        free_bloc[index_celulle].next_record = selected_bloc.next_record->next_record;

        return selected_bloc.data;
    }

    // Cas 2, il n'existe pas de bloc de taille, on cherche le premier bloc
    // disponible de taille T * (2 puissance k), et on le découpe en 2
    // récursivement jusqu'à avoir un bloc de taille T.
    else {
        int i; // l'indice de la cellule de taille T * (2 puissance k)
        for(i = index_celulle + 1; (i < BUDDY_MAX_INDEX)
                && (free_bloc[i].next_record == 0); i++) {
        }
        if (i >= BUDDY_MAX_INDEX) {
            // la taille demandé est plus grande que le plus grand bloc
            // disponible.
            perror("Not enough availlable space\n");
            return 0;
        }

        // On a trouvé un bloc plus grand que necessaire, il faut maintenant
        // le découper.
        union bloc big_bloc = free_bloc[i];

        // Tout d'abord on l'enlève de la chaine
        free_bloc[i].next_record = big_bloc.next_record->next_record;

        // Ensuite on le découpe en 2 récursivement. La taille des sous bloc
        // est de 2 puissance (i-1). On insère à chaque fois le deuxième sous
        // bloc dans la chaine, et on continue de découper le premier
        // sous-bloc.
        for(; i > index_celulle + 1; i--) {
            free_bloc[i - 1] = (union bloc) (
                big_bloc.data + POW_2(i - 1));
            free_bloc[i - 1].next_record->next_record = 0;
        }

        // On à maintenant un bloc de taille T, qu'on peut retourner
        return big_bloc.data;
    }
}

int mem_free(void *ptr, unsigned long size)
{
    /* ecrire votre code ici */
    // NB : cette ligne est là uniquement pour pouvoir utiliser les test en rapport avec l'allocation, elle est évidement à supprimer lorsque mem_free sera implémenté
    mem_init();
    return 0;
}


int mem_destroy()
{
    /* ecrire votre code ici */

    free(memory_pool);
    memory_pool = 0;
    return 0;
}
