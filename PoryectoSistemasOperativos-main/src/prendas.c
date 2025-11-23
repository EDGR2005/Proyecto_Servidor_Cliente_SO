#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prendas.h"

Prenda* crearPrenda(const char *id,
                    const char *marca,
                    const char *nombre,
                    float precio,
                    const char *moneda,
                    const char *urlImagen,
                    const char *genero,
                    const char *categoria,
                    const char *coleccion) {

    Prenda *p = (Prenda*) malloc(sizeof(Prenda));
    if (!p) return NULL;

    // Copiar campos
    strcpy(p->id, id);
    strcpy(p->marca, marca);
    strcpy(p->nombre, nombre);
    p->precio = precio;
    strcpy(p->moneda, moneda);
    strcpy(p->urlImagen, urlImagen);
    strcpy(p->genero, genero);
    strcpy(p->categoria, categoria);
    strcpy(p->coleccion, coleccion);

    // Inicializar enlaces de la lista
    p->siguiente = NULL;
    p->anterior = NULL;

    return p;
}
