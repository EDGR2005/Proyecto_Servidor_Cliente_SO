#include "behavior.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Archivo donde guardaremos los usuarios
#define USERS_FILE "users.csv"

// Variables globales para manejo de UI interna del login
static GtkWidget *entry_login_user;
static GtkWidget *entry_login_pass;
static GtkWidget *entry_reg_user;
static GtkWidget *entry_reg_email;
static GtkWidget *entry_reg_pass;
static GtkWidget *stack_auth_forms; // Para alternar entre form login y form register
static GtkWidget *lbl_mensaje_error;
static GtkWidget *lbl_navbar_user_ref; // Referencia al label del usuario en la navbar principal
static GtkStack *main_app_stack;       // Referencia al stack principal de la app

// ===============================================================
// LÓGICA DE DATOS (Backend simple)
// ===============================================================

void init_user_system() {
    FILE *f = fopen(USERS_FILE, "a+");
    if (f) fclose(f);
}

// Retorna 1 si el usuario existe y la contraseña coincide
// Retorna 0 si falla
// Retorna 2 si el usuario no existe
int validate_login(const char *user, const char *pass) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) return 0;

    char line[256];
    char f_user[50], f_pass[50], f_email[50];

    while (fgets(line, sizeof(line), f)) {
        // Formato CSV: usuario,password,email
        sscanf(line, "%[^,],%[^,],%s", f_user, f_pass, f_email);
        
        if (strcmp(user, f_user) == 0) {
            fclose(f);
            if (strcmp(pass, f_pass) == 0) return 1; // Éxito
            else return 0; // Contraseña incorrecta
        }
    }
    fclose(f);
    return 2; // Usuario no encontrado
}

// Retorna 1 si se registra éxito, 0 si el usuario ya existe
int register_user(const char *user, const char *pass, const char *email) {
    // 1. Verificar si ya existe
    FILE *f = fopen(USERS_FILE, "r");
    char line[256];
    char f_user[50], f_pass[50], f_email[50];
    
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            sscanf(line, "%[^,],%[^,],%s", f_user, f_pass, f_email);
            if (strcmp(user, f_user) == 0) {
                fclose(f);
                return 0; // Ya existe
            }
        }
        fclose(f);
    }

    // 2. Guardar nuevo
    f = fopen(USERS_FILE, "a");
    if (!f) return -1;
    fprintf(f, "%s,%s,%s\n", user, pass, email);
    fclose(f);
    return 1;
}

// ===============================================================
// CALLBACKS DE INTERFAZ
// ===============================================================

// Cambiar entre vista Login y Registro
static void on_switch_mode_clicked(GtkButton *btn, gpointer data) {
    const char *target = (const char *)data;
    gtk_stack_set_visible_child_name(GTK_STACK(stack_auth_forms), target);
    gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), ""); // Limpiar errores
}

// Acción de Botón "INICIAR SESIÓN"
static void on_btn_login_action(GtkButton *btn, gpointer data) {
    const char *u = gtk_entry_get_text(GTK_ENTRY(entry_login_user));
    const char *p = gtk_entry_get_text(GTK_ENTRY(entry_login_pass));

    if (strlen(u) == 0 || strlen(p) == 0) {
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "Por favor completa todos los campos.");
        return;
    }

    int result = validate_login(u, p);
    if (result == 1) {
        // LOGIN EXITOSO
        gchar *welcome_msg = g_strdup_printf("HOLA, %s", u);
        // Actualizar algún label en la navbar si existe (se pasa como param opcionalmente)
        if (lbl_navbar_user_ref) {
            gtk_button_set_label(GTK_BUTTON(lbl_navbar_user_ref), welcome_msg);
        }
        g_free(welcome_msg);
        
        // Redirigir a inicio (asumiendo que la página de inicio se llama "page0" o "inicio")
        // Nota: Esto depende de cómo nombres tus páginas en el stack principal
        if (main_app_stack) {
            gtk_stack_set_visible_child_name(main_app_stack, "page0"); // Ir a inicio
        }
        
        // Limpiar campos
        gtk_entry_set_text(GTK_ENTRY(entry_login_user), "");
        gtk_entry_set_text(GTK_ENTRY(entry_login_pass), "");
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "");

    } else if (result == 0) {
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "Contraseña incorrecta.");
    } else {
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "El usuario no existe.");
    }
}

// Acción de Botón "CREAR CUENTA"
static void on_btn_register_action(GtkButton *btn, gpointer data) {
    const char *u = gtk_entry_get_text(GTK_ENTRY(entry_reg_user));
    const char *e = gtk_entry_get_text(GTK_ENTRY(entry_reg_email));
    const char *p = gtk_entry_get_text(GTK_ENTRY(entry_reg_pass));

    if (strlen(u) == 0 || strlen(e) == 0 || strlen(p) == 0) {
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "Todos los campos son obligatorios.");
        return;
    }

    int res = register_user(u, p, e);
    if (res == 1) {
        // Registro exitoso, iniciamos sesión automáticamente
        if (lbl_navbar_user_ref) {
            gchar *welcome_msg = g_strdup_printf("HOLA, %s", u);
            gtk_button_set_label(GTK_BUTTON(lbl_navbar_user_ref), welcome_msg);
            g_free(welcome_msg);
        }
        if (main_app_stack) {
            gtk_stack_set_visible_child_name(main_app_stack, "page0");
        }
        gtk_entry_set_text(GTK_ENTRY(entry_reg_user), "");
        gtk_entry_set_text(GTK_ENTRY(entry_reg_email), "");
        gtk_entry_set_text(GTK_ENTRY(entry_reg_pass), "");
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "");
    } else {
        gtk_label_set_text(GTK_LABEL(lbl_mensaje_error), "El usuario ya está registrado.");
    }
}


// ===============================================================
// CONSTRUCTOR DE UI (Login Page)
// ===============================================================

GtkWidget *create_login_register_page(GtkStack *main_stack, GtkWidget *label_usuario_navbar) {
    main_app_stack = main_stack;
    lbl_navbar_user_ref = label_usuario_navbar;

    // Contenedor principal centrado
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(main_box, 400, -1); // Ancho fijo estilo tarjeta

    // Título Principal
    GtkWidget *lbl_brand = gtk_label_new("MARKETWOLF ID");
    GtkStyleContext *context = gtk_widget_get_style_context(lbl_brand);
    gtk_style_context_add_class(context, "login-brand");
    gtk_box_pack_start(GTK_BOX(main_box), lbl_brand, FALSE, FALSE, 10);

    // Stack interno para cambiar entre Login y Registro sin cambiar de página
    stack_auth_forms = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack_auth_forms), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    
    // --- FORMULARIO LOGIN ---
    GtkWidget *box_login = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    
    GtkWidget *lbl_title_log = gtk_label_new("INICIAR SESIÓN");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title_log), "login-header");
    
    entry_login_user = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_login_user), "USUARIO");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_login_user), "login-input");

    entry_login_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_login_pass), "CONTRASEÑA");
    gtk_entry_set_visibility(GTK_ENTRY(entry_login_pass), FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_login_pass), "login-input");

    GtkWidget *btn_do_login = gtk_button_new_with_label("ENTRAR");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_do_login), "login-btn-primary");
    g_signal_connect(btn_do_login, "clicked", G_CALLBACK(on_btn_login_action), NULL);

    GtkWidget *btn_goto_reg = gtk_button_new_with_label("¿NO TIENES CUENTA? REGÍSTRATE");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_goto_reg), "login-btn-text");
    g_signal_connect(btn_goto_reg, "clicked", G_CALLBACK(on_switch_mode_clicked), "register_view");

    gtk_box_pack_start(GTK_BOX(box_login), lbl_title_log, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box_login), entry_login_user, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_login), entry_login_pass, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_login), btn_do_login, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box_login), btn_goto_reg, FALSE, FALSE, 5);

    // --- FORMULARIO REGISTRO ---
    GtkWidget *box_reg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);

    GtkWidget *lbl_title_reg = gtk_label_new("CREAR CUENTA");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title_reg), "login-header");

    entry_reg_user = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_reg_user), "USUARIO");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_reg_user), "login-input");

    entry_reg_email = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_reg_email), "E-MAIL");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_reg_email), "login-input");

    entry_reg_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_reg_pass), "CONTRASEÑA");
    gtk_entry_set_visibility(GTK_ENTRY(entry_reg_pass), FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_reg_pass), "login-input");

    GtkWidget *btn_do_reg = gtk_button_new_with_label("CREAR CUENTA");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_do_reg), "login-btn-primary");
    g_signal_connect(btn_do_reg, "clicked", G_CALLBACK(on_btn_register_action), NULL);

    GtkWidget *btn_goto_log = gtk_button_new_with_label("¿YA TIENES CUENTA? INICIA SESIÓN");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_goto_log), "login-btn-text");
    g_signal_connect(btn_goto_log, "clicked", G_CALLBACK(on_switch_mode_clicked), "login_view");

    gtk_box_pack_start(GTK_BOX(box_reg), lbl_title_reg, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box_reg), entry_reg_user, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_reg), entry_reg_email, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_reg), entry_reg_pass, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_reg), btn_do_reg, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box_reg), btn_goto_log, FALSE, FALSE, 5);

    // Añadir formularios al stack interno
    gtk_stack_add_named(GTK_STACK(stack_auth_forms), box_login, "login_view");
    gtk_stack_add_named(GTK_STACK(stack_auth_forms), box_reg, "register_view");

    // Label para mensajes de error
    lbl_mensaje_error = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_mensaje_error), "login-error");
    
    // Empaquetado final
    gtk_box_pack_start(GTK_BOX(main_box), stack_auth_forms, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(main_box), lbl_mensaje_error, FALSE, FALSE, 10);

    gtk_widget_show_all(main_box);
    return main_box;
}

// ===============================================================
// (MANTENER CÓDIGO ANTERIOR DE behavior.c ABAJO)
// ===============================================================

static void on_nav_item_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(user_data);
    const gchar *page_name = gtk_widget_get_name(GTK_WIDGET(button));
    gtk_stack_set_visible_child_name(stack, page_name);

    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
    while (parent && !GTK_IS_POPOVER(parent))
        parent = gtk_widget_get_parent(parent);
    if (GTK_IS_POPOVER(parent))
        gtk_popover_popdown(GTK_POPOVER(parent));
}

static void on_carrusel_button_clicked(GtkButton *button, gpointer user_data) {
    CarouselData *data = user_data;
    const gchar *current_name = gtk_stack_get_visible_child_name(data->stack);
    int current = -1;
    for (int i = 0; i < data->count; i++)
        if (g_strcmp0(current_name, data->names[i]) == 0) current = i;

    int next = current;
    if (g_strcmp0(gtk_button_get_label(button), ">") == 0) next = (current + 1) % data->count;
    else next = (current - 1 + data->count) % data->count;
    
    gtk_stack_set_visible_child_name(data->stack, data->names[next]);
}

GtkWidget *create_nav_button(GtkStack *stack, const gchar *label, const gchar *page_name) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_widget_set_name(btn, page_name);
    GtkStyleContext *context = gtk_widget_get_style_context(btn);
    gtk_style_context_add_class(context, "nav-button");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
    return btn;
}

GtkWidget *create_icon_nav_button(GtkStack *stack, const gchar *icon_name, const gchar *page_name) {
    GtkWidget *btn = gtk_button_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_name(btn, page_name); 
    GtkStyleContext *context = gtk_widget_get_style_context(btn);
    gtk_style_context_add_class(context, "nav-button");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
    return btn;
}

GtkWidget *create_menu_button(GtkStack *stack, const gchar *main_label, gint num_options, const gchar *option_labels[], const gchar *option_page_names[]) {
    GtkWidget *menu_btn = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(menu_btn), main_label);
    
    GtkStyleContext *context = gtk_widget_get_style_context(menu_btn);
    gtk_style_context_add_class(context, "menu-nav-button");

    GtkWidget *popover = gtk_popover_new(menu_btn);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(popover), box);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);

    for (int i = 0; i < num_options; i++) {
        GtkWidget *item = gtk_button_new_with_label(option_labels[i]);
        gtk_widget_set_name(item, option_page_names[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(item), "menu-item");
        gtk_widget_set_halign(item, GTK_ALIGN_FILL); 
        g_signal_connect(item, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
        gtk_box_pack_start(GTK_BOX(box), item, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(box);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_btn), popover);
    return menu_btn;
}

GtkWidget *create_content_label(const gchar *text, const gchar *css_class) {
    GtkWidget *label = gtk_label_new(text);
    if (css_class) {
        GtkStyleContext *context = gtk_widget_get_style_context(label);
        gtk_style_context_add_class(context, css_class);
    }
    return label;
}

GtkWidget *create_cell(const char *ruta, const char *texto) {
    GtkWidget *event_box = gtk_event_box_new(); 
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(ruta, 150, 200, TRUE, NULL);
    GtkWidget *image;
    if (pixbuf) {
        image = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
    } else {
        image = gtk_label_new("[IMAGEN NO ENCONTRADA]");
    }
    
    GtkWidget *label = gtk_label_new(texto);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_size_request(label, 150, -1);

    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(event_box), box);
    return event_box;
}

GtkWidget *create_grid_scrolleable(int filas, int columnas) {
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(scrolled_window), grid);

    return scrolled_window;
}

void setup_carrusel(GtkWidget **out_grid, GtkWidget **out_stack_img, CarouselData **out_carousel_data, const gchar *paths[], const gchar *names[], int count, int width, int height) {
    GtkWidget *grid = gtk_grid_new();
    *out_grid = grid;
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

    GtkWidget *stack = gtk_stack_new();
    *out_stack_img = stack;
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 500);
    
    gtk_widget_set_size_request(stack, width, height);

    GtkWidget *btn_prev = gtk_button_new_with_label("<");
    GtkWidget *btn_next = gtk_button_new_with_label(">");
    gtk_widget_set_name(btn_prev, "carrusel_control");
    gtk_widget_set_name(btn_next, "carrusel_control");

    for (int i = 0; i < count; i++) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget *img = gtk_image_new();
        GdkPixbuf *orig = gdk_pixbuf_new_from_file(paths[i], NULL);
        if (orig) {
            GdkPixbuf *scaled = gdk_pixbuf_scale_simple(orig, width, height, GDK_INTERP_BILINEAR);
            gtk_image_set_from_pixbuf(GTK_IMAGE(img), scaled);
            g_object_unref(orig);
            g_object_unref(scaled);
        }
        gtk_container_add(GTK_CONTAINER(box), img);
        gtk_stack_add_named(GTK_STACK(stack), box, names[i]);
    }

    CarouselData *data = g_new(CarouselData, 1);
    data->stack = GTK_STACK(stack);
    data->names = names;
    data->count = count;
    *out_carousel_data = data;

    g_signal_connect(btn_prev, "clicked", G_CALLBACK(on_carrusel_button_clicked), data);
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_carrusel_button_clicked), data);

    gtk_grid_attach(GTK_GRID(grid), btn_prev, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), stack, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_next, 2, 0, 1, 1);
}