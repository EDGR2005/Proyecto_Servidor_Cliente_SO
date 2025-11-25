#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define ARCHIVO_USUARIOS "usuarios.dat"

// Colores para logs
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// --- UTILS LOGGING ---
void log_activity(const char *tipo, const char *mensaje, const char *color) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local);

    printf("[%s] %s[%s]%s %s\n", time_str, color, tipo, ANSI_COLOR_RESET, mensaje);
    // También se podría guardar en un archivo server.log aquí
}

// --- LÓGICA DE ARCHIVOS ---

void eliminarSaltoLinea(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') str[len - 1] = '\0';
}

bool usuarioExiste(const char *username) {
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) return false;
    char linea[1024];
    while (fgets(linea, sizeof(linea), fp)) {
        char copia[1024]; strcpy(copia, linea);
        char *token = strtok(copia, ",");
        if (token && strcmp(token, username) == 0) { fclose(fp); return true; }
    }
    fclose(fp);
    return false;
}

// --- PROCESADORES DE COMANDOS ---

void procesar_login(char *username, char *password, char *respuesta, struct sockaddr_in *client_addr) {
    char log_msg[200];
    sprintf(log_msg, "Intento de Login: Usuario '%s' desde %s", username, inet_ntoa(client_addr->sin_addr));
    log_activity("LOGIN", log_msg, ANSI_COLOR_YELLOW);

    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    if (!fp) {
        sprintf(respuesta, "FAIL:Database Error");
        log_activity("ERROR", "No se pudo abrir la base de datos de usuarios", ANSI_COLOR_RED);
        return;
    }

    char linea[1024];
    bool encontrado = false;

    while (fgets(linea, sizeof(linea), fp)) {
        eliminarSaltoLinea(linea);
        char copia[1024]; strcpy(copia, linea);
        
        char *u = strtok(copia, ",");
        char *p = strtok(NULL, ",");
        
        if (u && p && strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            // Recuperar datos extra
            char *phone = strtok(NULL, ",");
            char *loc = strtok(NULL, ",");
            char *email = strtok(NULL, ",");
            char *addr = strtok(NULL, ",");
            char *card = strtok(NULL, ",");
            
            sprintf(respuesta, "OK|%s|%s|%s|%s|%s", 
                    phone?phone:"", loc?loc:"", email?email:"", addr?addr:"", card?card:"");
            encontrado = true;
            
            sprintf(log_msg, "Login EXITOSO: Usuario '%s'", username);
            log_activity("SUCCESS", log_msg, ANSI_COLOR_GREEN);
            break;
        }
    }
    fclose(fp);
    
    if (!encontrado) {
        sprintf(respuesta, "FAIL:Credenciales Incorrectas");
        sprintf(log_msg, "Login FALLIDO: Usuario '%s' - Credenciales incorrectas", username);
        log_activity("FAIL", log_msg, ANSI_COLOR_RED);
    }
}

void procesar_registro(char *datos, char *respuesta) {
    // datos: user,pass,phone,loc,email
    char copia[1024]; strcpy(copia, datos);
    char *u = strtok(copia, ",");
    
    char log_msg[200];
    sprintf(log_msg, "Solicitud de Registro: Nuevo usuario '%s'", u);
    log_activity("REGISTER", log_msg, ANSI_COLOR_CYAN);
    
    if (usuarioExiste(u)) {
        sprintf(respuesta, "FAIL:Usuario ya existe");
        log_activity("FAIL", "Registro rechazado: Usuario duplicado", ANSI_COLOR_RED);
        return;
    }

    FILE *fp = fopen(ARCHIVO_USUARIOS, "a");
    if (!fp) {
        sprintf(respuesta, "FAIL:Error escritura");
        log_activity("ERROR", "Error crítico al escribir en disco", ANSI_COLOR_RED);
        return;
    }
    // Guardamos con campos vacíos al final
    fprintf(fp, "%s,None,None\n", datos); 
    fclose(fp);
    
    sprintf(respuesta, "OK:Usuario Creado");
    sprintf(log_msg, "Registro COMPLETADO: Usuario '%s' añadido a la base de datos", u);
    log_activity("SUCCESS", log_msg, ANSI_COLOR_GREEN);
}

void procesar_update(char *datos, char *respuesta) {
    // datos: user|address|card (Nota: usamos | como separador en el protocolo para evitar conflictos con comas en direcciones)
    // Pero ojo, el cliente manda UPDATE_PAY user|address|card. 
    // Aquí el 'datos' es el payload.
    
    char *user = strtok(datos, "|");
    char *new_addr = strtok(NULL, "|");
    char *new_card = strtok(NULL, "|");
    
    if (!user) {
        sprintf(respuesta, "FAIL:Datos incompletos");
        return;
    }

    char log_msg[200];
    sprintf(log_msg, "Actualizando datos de compra para: %s", user);
    log_activity("PURCHASE", log_msg, ANSI_COLOR_MAGENTA);

    // Lógica de actualización (Sobrescribir archivo)
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    FILE *temp = fopen("temp.dat", "w");
    char linea[1024];
    bool found = false;
    
    while(fgets(linea, sizeof(linea), fp)) {
        eliminarSaltoLinea(linea);
        char copia[1024]; strcpy(copia, linea);
        char *u = strtok(copia, ",");
        
        if (strcmp(u, user) == 0) {
            char *p = strtok(NULL, ",");
            char *ph = strtok(NULL, ",");
            char *lo = strtok(NULL, ",");
            char *em = strtok(NULL, ",");
            fprintf(temp, "%s,%s,%s,%s,%s,%s,%s\n", user, p, ph, lo, em, 
                    new_addr ? new_addr : "None", 
                    new_card ? new_card : "None");
            found = true;
        } else {
            fprintf(temp, "%s\n", linea);
        }
    }
    fclose(fp);
    fclose(temp);
    
    if(found) {
        remove(ARCHIVO_USUARIOS);
        rename("temp.dat", ARCHIVO_USUARIOS);
        sprintf(respuesta, "OK:Compra Registrada");
        
        sprintf(log_msg, "COMPRA COMPLETADA: Usuario '%s' ha realizado un pedido. Datos actualizados.", user);
        log_activity("SUCCESS", log_msg, ANSI_COLOR_GREEN);
        
    } else {
        remove("temp.dat");
        sprintf(respuesta, "FAIL:Usuario no encontrado");
        log_activity("FAIL", "Intento de compra fallido: Usuario no encontrado", ANSI_COLOR_RED);
    }
}

void procesar_delete(char *datos, char *respuesta) {
    // datos: user,pass
    char *user = strtok(datos, ",");
    char *pass = strtok(NULL, ",");
    
    char log_msg[200];
    sprintf(log_msg, "Solicitud de ELIMINACIÓN de cuenta: %s", user);
    log_activity("DELETE", log_msg, ANSI_COLOR_RED);

    // Verificar credenciales primero (reutilizamos lógica o lo hacemos manual aquí)
    // Por simplicidad, escaneo manual rápido
    FILE *fp = fopen(ARCHIVO_USUARIOS, "r");
    FILE *temp = fopen("temp.dat", "w");
    char linea[1024];
    bool borrado = false;
    bool credenciales_ok = false;

    while(fgets(linea, sizeof(linea), fp)) {
        char copia[1024]; strcpy(copia, linea);
        eliminarSaltoLinea(copia);
        char *u = strtok(copia, ",");
        char *p = strtok(NULL, ",");
        
        if (strcmp(u, user) == 0) {
            if (strcmp(p, pass) == 0) {
                credenciales_ok = true;
                borrado = true;
                // NO escribimos esta línea en temp -> se borra
                sprintf(log_msg, "Cuenta ELIMINADA permanentemente: %s", user);
                log_activity("WARNING", log_msg, ANSI_COLOR_RED);
            } else {
                fprintf(temp, "%s", linea); // Password mal, mantenemos usuario
                sprintf(log_msg, "Fallo eliminación %s: Contraseña incorrecta", user);
                log_activity("FAIL", log_msg, ANSI_COLOR_YELLOW);
            }
        } else {
            fprintf(temp, "%s", linea);
        }
    }
    fclose(fp);
    fclose(temp);
    
    if (borrado) {
        remove(ARCHIVO_USUARIOS);
        rename("temp.dat", ARCHIVO_USUARIOS);
        sprintf(respuesta, "OK:Cuenta Borrada");
    } else {
        remove("temp.dat");
        if (!credenciales_ok) sprintf(respuesta, "FAIL:Credenciales Incorrectas");
        else sprintf(respuesta, "FAIL:Usuario no encontrado");
    }
}

void procesar_logout(char *username, char *respuesta) {
    char log_msg[200];
    sprintf(log_msg, "Usuario '%s' ha cerrado sesión.", username);
    log_activity("LOGOUT", log_msg, ANSI_COLOR_MAGENTA); // Usamos magenta para distinguir salida
    
    sprintf(respuesta, "OK:Sesion Cerrada");
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("socket failed"); exit(EXIT_FAILURE); }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { perror("setsockopt"); exit(EXIT_FAILURE); }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("bind failed"); exit(EXIT_FAILURE); }
    if (listen(server_fd, 3) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    printf("\n");
    log_activity("SYSTEM", "Servidor LV Iniciado. Esperando conexiones...", ANSI_COLOR_BLUE);
    printf("------------------------------------------------------------\n");

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept"); continue;
        }

        // ... (Log connect) ...

        read(new_socket, buffer, BUFFER_SIZE);
        
        char respuesta[BUFFER_SIZE] = {0};
        char *cmd = strtok(buffer, " ");
        char *payload = strtok(NULL, "");

        if (cmd) {
            if (strcmp(cmd, "LOGIN") == 0) {
                // ... (Login logic) ...
                char *u = strtok(payload, ",");
                char *p = strtok(NULL, ",");
                procesar_login(u, p, respuesta, &address);
            } 
            else if (strcmp(cmd, "REGISTER") == 0) {
                procesar_registro(payload, respuesta);
            }
            else if (strcmp(cmd, "UPDATE_PAY") == 0) { 
                procesar_update(payload, respuesta);
            }
            else if (strcmp(cmd, "DELETE") == 0) {
                procesar_delete(payload, respuesta);
            }
            // NUEVO CASO
           else if (strcmp(cmd, "ACTION") == 0) {
                log_activity("ACTION", payload ? payload : "(vacio)", ANSI_COLOR_MAGENTA);
                sprintf(respuesta, "OK:LOG");
            }
            else {
                // ... (Error logic) ...
                sprintf(respuesta, "FAIL:Comando desconocido");
            }
        }
        // ... (Send response & close) ...
        send(new_socket, respuesta, strlen(respuesta), 0);
        close(new_socket);
        memset(buffer, 0, BUFFER_SIZE);
    }
    return 0;
}