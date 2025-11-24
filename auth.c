#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auth.h"

#define ARCHIVO_USUARIOS "usuarios.dat"
#define TEMP_ARCHIVO "temp_usuarios.dat"

void eliminarSaltoLinea(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') str[len - 1] = '\0';
}

bool esEmailValido(const char *email) {
    const char *arroba = strchr(email, '@');
    const char *punto = strrchr(email, '.');
    return (arroba && punto && punto > arroba && strlen(email) > 5);
}

bool usuarioExiste(const char *username) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) return false;
    char linea[1024]; char fileUser[100];
    while (fgets(linea, sizeof(linea), fp)) {
        char *token = strtok(linea, ",");
        if (token) {
            strcpy(fileUser, token);
            if (strcmp(fileUser, username) == 0) { fclose(fp); return true; }
        }
    }
    fclose(fp);
    return false;
}

// Registro inicial (Address y Card se guardan vacíos o por defecto)
bool registrarUsuario(const char *username, const char *password, const char *phone, const char *location, const char *email, char *mensajeError) {
    if (strlen(username) < 3) { strcpy(mensajeError, "Usuario muy corto."); return false; }
    if (strlen(password) < 4) { strcpy(mensajeError, "Contraseña min 4 caracteres."); return false; }
    if (!esEmailValido(email)) { strcpy(mensajeError, "Email inválido."); return false; }
    if (usuarioExiste(username)) { strcpy(mensajeError, "Usuario ya existe."); return false; }

    FILE *fp = fopen(ARCHIVO_USUARIOS, "a");
    if (!fp) { strcpy(mensajeError, "Error BD."); return false; }

    // Guardamos campos extra vacíos por ahora: Address="None", Card="None"
    // Formato: user,pass,phone,location,email,address,card_info
    fprintf(fp, "%s,%s,%s,%s,%s,None,None\n", username, password, phone, location, email);
    fclose(fp);
    return true;
}

bool validarLogin(const char *username, const char *password) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) return false;
    char linea[1024]; char fileUser[100]; char *filePass;
    while (fgets(linea, sizeof(linea), fp)) {
        eliminarSaltoLinea(linea);
        char copia[1024]; strcpy(copia, linea); // Trabajar en copia
        char *token = strtok(copia, ",");
        if (token) {
            strcpy(fileUser, token);
            token = strtok(NULL, ",");
            if (token) {
                filePass = token;
                if (strcmp(fileUser, username) == 0 && strcmp(filePass, password) == 0) { fclose(fp); return true; }
            }
        }
    }
    fclose(fp);
    return false;
}


bool mostrarDatosUsuario(const char *usernameBuscado) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) {
        printf("Error al abrir archivo.\n");
        return false;
    }

    char linea[512];
    char copia[512];

    while (fgets(linea, sizeof(linea), fp)) {

        strcpy(copia, linea);  // Copia porque strtok modifica
        char *token = strtok(copia, ",");

        if (!token) continue;

        if (strcmp(token, usernameBuscado) == 0) {
            // Coincidió: imprimir todos los campos
            printf("=== Datos del usuario ===\n");

            char *password      = strtok(NULL, ",");
            char *telefono      = strtok(NULL, ",");
            char *estado        = strtok(NULL, ",");
            char *correo        = strtok(NULL, ",");
            char *extra1        = strtok(NULL, ",");
            char *extra2        = strtok(NULL, ",");

            printf("Usuario:   %s\n", token);
            printf("Password:  %s\n", password);
            printf("Teléfono:  %s\n", telefono);
            printf("Estado:    %s\n", estado);
            printf("Correo:    %s\n", correo);
            printf("Extra 1:   %s\n", extra1);
            printf("Extra 2:   %s\n", extra2);

            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    printf("Usuario '%s' no encontrado.\n", usernameBuscado);
    return false;
}


// Recuperar TODOS los datos
bool obtenerDatosUsuarioCompleto(const char *username, char *phone, char *loc, char *email, char *addr, char *card) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) return false;
    char linea[1024];
    while (fgets(linea, sizeof(linea), fp)) {
        eliminarSaltoLinea(linea);
        char copia[1024]; strcpy(copia, linea);
        char *token = strtok(copia, ","); // user
        if (token && strcmp(token, username) == 0) {
            // Saltamos pass
            strtok(NULL, ","); 
            // Phone
            char *t = strtok(NULL, ","); if(t) strcpy(phone, t); else strcpy(phone, "");
            // Location
            t = strtok(NULL, ","); if(t) strcpy(loc, t); else strcpy(loc, "");
            // Email
            t = strtok(NULL, ","); if(t) strcpy(email, t); else strcpy(email, "");
            // Address
            t = strtok(NULL, ","); if(t) strcpy(addr, t); else strcpy(addr, "");
            // Card
            t = strtok(NULL, ","); if(t) strcpy(card, t); else strcpy(card, "");
            
            fclose(fp); return true;
        }
    }
    fclose(fp); return false;
}

// Actualizar Dirección y Tarjeta (Usado tras el Checkout)
bool actualizarDatosUsuario(const char *username, const char *new_address, const char *new_card_info) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    FILE *temp = fopen(TEMP_ARCHIVO, "w");
    if (!fp || !temp) return false;

    char linea[1024];
    bool encontrado = false;

    while (fgets(linea, sizeof(linea), fp)) {
        eliminarSaltoLinea(linea);
        char original[1024]; strcpy(original, linea);
        
        char *token = strtok(linea, ","); // user
        if (token && strcmp(token, username) == 0) {
            char *pass = strtok(NULL, ",");
            char *phone = strtok(NULL, ",");
            char *loc = strtok(NULL, ",");
            char *email = strtok(NULL, ",");
            // Reconstruimos la línea con los nuevos datos
            // Nota: Asumimos que los tokens anteriores existen. En producción esto debería ser más robusto.
            if (pass && phone && loc && email) {
                 fprintf(temp, "%s,%s,%s,%s,%s,%s,%s\n", 
                    username, pass, phone, loc, email, 
                    new_address ? new_address : "None", 
                    new_card_info ? new_card_info : "None");
                 encontrado = true;
            } else {
                 fprintf(temp, "%s\n", original); // Si falla algo, mantenemos original
            }
        } else {
            fprintf(temp, "%s\n", original);
        }
    }
    fclose(fp); fclose(temp);
    if (encontrado) {
        remove(ARCHIVO_USUARIOS);
        rename(TEMP_ARCHIVO, ARCHIVO_USUARIOS);
        return true;
    }
    remove(TEMP_ARCHIVO); // Limpiar si falló
    return false;
}

bool eliminarUsuario(const char *username, const char *password) {
    if (!validarLogin(username, password)) return false;
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    FILE *temp = fopen(TEMP_ARCHIVO, "w");
    if (!fp || !temp) return false;
    char linea[1024];
    while (fgets(linea, sizeof(linea), fp)) {
        char copia[1024]; strcpy(copia, linea);
        char *token = strtok(copia, ",");
        if (token && strcmp(token, username) != 0) fprintf(temp, "%s", linea);
    }
    fclose(fp); fclose(temp);
    remove(ARCHIVO_USUARIOS);
    rename(TEMP_ARCHIVO, ARCHIVO_USUARIOS);
    return true;
}