#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auth.h"

#define ARCHIVO_USUARIOS "usuarios.dat"

// Función auxiliar para limpiar saltos de línea
void eliminarSaltoLinea(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

// Verifica si el usuario ya existe en el archivo
bool usuarioExiste(const char *username) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) return false; // Si no existe el archivo, no hay usuarios

    char linea[150];
    char fileUser[50], filePass[50];

    while (fgets(linea, sizeof(linea), fp)) {
        // Formato esperado: usuario,password
        char *token = strtok(linea, ",");
        if (token) {
            strcpy(fileUser, token);
            if (strcmp(fileUser, username) == 0) {
                fclose(fp);
                return true;
            }
        }
    }

    fclose(fp);
    return false;
}

// Registra un nuevo usuario
bool registrarUsuario(const char *username, const char *password, char *mensajeError) {
    // 1. Validaciones básicas
    if (strlen(username) < 4) {
        strcpy(mensajeError, "El usuario debe tener al menos 4 caracteres.");
        return false;
    }
    if (strlen(password) < 4) {
        strcpy(mensajeError, "La contraseña debe tener al menos 4 caracteres.");
        return false;
    }
    if (strchr(username, ',') || strchr(password, ',')) {
        strcpy(mensajeError, "El carácter ',' no está permitido.");
        return false;
    }

    // 2. Verificar duplicados
    if (usuarioExiste(username)) {
        strcpy(mensajeError, "El nombre de usuario ya está en uso.");
        return false;
    }

    // 3. Guardar en archivo
    FILE *fp = fopen(ARCHIVO_USUARIOS, "a"); // 'a' para append (agregar al final)
    if (!fp) {
        strcpy(mensajeError, "Error crítico al acceder a la base de datos.");
        return false;
    }

    fprintf(fp, "%s,%s\n", username, password);
    fclose(fp);
    return true;
}

// Valida las credenciales para el login
bool validarLogin(const char *username, const char *password) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) return false;

    char linea[150];
    char fileUser[50];
    char *filePass; // Puntero al token de la contraseña

    while (fgets(linea, sizeof(linea), fp)) {
        eliminarSaltoLinea(linea);

        char *token = strtok(linea, ","); // Primer token: Usuario
        if (token) {
            strcpy(fileUser, token);
            
            token = strtok(NULL, ","); // Segundo token: Password
            if (token) {
                filePass = token;
                
                // Comparar ambos
                if (strcmp(fileUser, username) == 0 && strcmp(filePass, password) == 0) {
                    fclose(fp);
                    return true;
                }
            }
        }
    }

    fclose(fp);
    return false;
}