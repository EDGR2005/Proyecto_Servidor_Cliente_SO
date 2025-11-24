#include <stdio.h>
#include <stdlib.h>
#include "listaRopa.h"

ListaRopa* crearListaRopa() {
    ListaRopa *lista = (ListaRopa*)malloc(sizeof(ListaRopa));
    lista->raiz = NULL;
    lista->ultimo = NULL;
    lista->tamano = 0;
    return lista;
}

void agregarAlFinal(ListaRopa *lista, Prenda *p) {
    if (lista->raiz == NULL) {
        lista->raiz = p;
        lista->ultimo = p;
    } else {
        lista->ultimo->siguiente = p;
        p->anterior = lista->ultimo;
        lista->ultimo = p;
    }
    lista->tamano++;
}

void eliminarFinal(ListaRopa *lista) {
    if (lista->ultimo == NULL)
        return;

    Prenda *elim = lista->ultimo;

    if (lista->raiz == lista->ultimo) {
        lista->raiz = NULL;
        lista->ultimo = NULL;
    } else {
        lista->ultimo = elim->anterior;
        lista->ultimo->siguiente = NULL;
    }

    free(elim);
    lista->tamano--;
}

void imprimirListaRopa(ListaRopa *lista) {
    if (lista == NULL || lista->raiz == NULL) {
        printf("Lista vacía\n");
        return;
    }

    printf("--- 👕 INICIO DE LISTADO DE PRENDAS 👚 ---\n");
    
    // Asumimos que la estructura 'Prenda' tiene todos los campos usados en 'cargarDesdeCSV'
    // id, marca, nombre, precio, moneda, urlImagen, genero, categoria, coleccion
    
    Prenda *p = lista->raiz;
    while (p != NULL) {
        printf("--------------------------------------------------\n");
        printf("ID:        %s\n", p->id);
        printf("Marca:     %s\n", p->marca);
        printf("Nombre:    %s\n", p->nombre);
        printf("Precio:    %.2f %s\n", p->precio, p->moneda); // Precio y Moneda
        printf("URL Img:   %s\n", p->urlImagen);
        printf("Género:    %s\n", p->genero);
        printf("Categoría: %s\n", p->categoria);
        printf("Colección: %s\n", p->coleccion);
        
        p = p->siguiente;
    }
    
    printf("--- FIN DE LISTADO DE PRENDAS ---\n");
}
