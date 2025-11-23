#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <gtk/gtk.h>

// ----------------------
// Estructuras de Datos
// ----------------------
typedef struct {
    GtkStack *stack;
    const gchar **names;
    gint count;
} CarouselData;

// Estructura para el usuario actual
typedef struct {
    char username[50];
    char email[50];
    int is_logged_in;
} CurrentSession;

// ----------------------
// Prototipos existentes
// ----------------------

GtkWidget *create_nav_button(GtkStack *stack, const gchar *label, const gchar *page_name);
GtkWidget *create_icon_nav_button(GtkStack *stack, const gchar *icon_name, const gchar *page_name);
GtkWidget *create_menu_button(GtkStack *stack,
                              const gchar *main_label,
                              gint num_options,
                              const gchar *option_labels[],
                              const gchar *option_page_names[]);
GtkWidget *create_content_label(const gchar *text, const gchar *css_class);
GtkWidget *create_cell(const char *ruta, const char *texto);
GtkWidget *create_grid_scrolleable(int filas, int columnas);

// Carrusel
void setup_carrusel(GtkWidget **out_grid,
                    GtkWidget **out_stack_img,
                    CarouselData **out_carousel_data,
                    const gchar *paths[],
                    const gchar *names[],
                    int count,
                    int width,\
                    int height);

// ----------------------
// NUEVO: Prototipos Login/Register
// ----------------------

// Crea la página completa de login/registro para añadir al Stack principal
GtkWidget *create_login_register_page(GtkStack *main_stack, GtkWidget *label_usuario_navbar);

// Inicializa el sistema de usuarios (crea el archivo si no existe)
void init_user_system();

#endif