#ifndef AUTH_H
#define AUTH_H

#include <gtk/gtk.h>
#include <stdbool.h>

// Estructura para representar un usuario
typedef struct {
    char username[50];
    char password[50];
} User;

// Funciones principales
bool registrarUsuario(const char *username, const char *password, char *mensajeError);
bool validarLogin(const char *username, const char *password);
bool usuarioExiste(const char *username);

#endif