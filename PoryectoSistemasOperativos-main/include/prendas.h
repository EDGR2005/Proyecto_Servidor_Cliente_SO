#ifndef PRENDAS_H
#define PRENDAS_H

typedef struct Prenda {
    char id[100];
    char marca[50];
    char nombre[200];
    float precio;
    char moneda[10];
    char urlImagen[500];
    char genero[20];
    char categoria[100];
    char coleccion[50];
    
    struct Prenda *siguiente;
    struct Prenda *anterior;
} Prenda;

// Constructor
Prenda* crearPrenda(const char *id, const char *marca, const char *nombre, float precio, const char *moneda,
                    const char *urlImagen, const char *genero, const char * categoria, const char *coleccion);

#endif
