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

char* obtenerInfoPrenda(Prenda *p) {
    if (!p) return NULL;

    // Calculamos un tamaño suficiente para almacenar toda la info
    size_t tamaño = 1024; // Ajusta si es necesario
    char *info = (char*) malloc(tamaño);
    if (!info) return NULL;

    snprintf(info, tamaño,
             "ID: %s\nMarca: %s\nNombre: %s\nPrecio: %.2f %s \nGenero: %s\nCategoria: %s\nColeccion: %s",
             p->id,
             p->marca,
             p->nombre,
             p->precio,
             p->moneda,
             p->genero,
             p->categoria,
             p->coleccion);

    return info;
}
