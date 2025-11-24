#ifndef AUTH_H
#define AUTH_H

#include <gtk/gtk.h>
#include <stdbool.h>

// Funciones principales
// Modificado: Ahora soporta guardar dirección completa y tarjeta
bool registrarUsuario(const char *username, const char *password, const char *phone, const char *location, const char *email, char *mensajeError);
bool validarLogin(const char *username, const char *password);
bool usuarioExiste(const char *username);
bool eliminarUsuario(const char *username, const char *password);

// Modificado: Recupera datos extendidos para el checkout
// address_dest: Calle y número
// card_dest: "Visa **** 1234" o similar
bool obtenerDatosUsuarioCompleto(const char *username, char *phone_dest, char *location_dest, char *email_dest, char *address_dest, char *card_dest);

// Nueva función para actualizar datos de pago/envío tras una compra
bool actualizarDatosUsuario(const char *username, const char *new_address, const char *new_card_info);

#endif