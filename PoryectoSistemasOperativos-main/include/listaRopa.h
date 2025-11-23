#ifndef LISTA_ROPA_H
#define LISTA_ROPA_H

#include "prendas.h"

typedef struct ListaRopa {
    Prenda *raiz;
    Prenda *ultimo;
    int tamano;
} ListaRopa;

ListaRopa* crearListaRopa();
void agregarAlFinal(ListaRopa *lista, Prenda *p);
void eliminarFinal(ListaRopa *lista);
void imprimirListaRopa(ListaRopa *lista);

#endif
