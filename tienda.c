#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h> 
#include <stdlib.h>
#include "prendas.h"
#include <libsoup-2.4/libsoup/soup.h>
#include <string.h>
#include <curl/curl.h>
#include "listaRopa.h"
#include "auth.h"
#include<stdio.h>

// ====================================================================
// VARIABLES GLOBALES DE ESTADO
// ====================================================================
char current_username[100] = "INVITADO"; 
char current_phone[50] = "";
char current_location[50] = ""; 
char current_email[100] = "";
char current_address[200] = ""; 
char current_card_info[100] = ""; 

GtkWidget *global_lbl_username_display = NULL; 

// Lista de Estados
const char *MX_STATES[] = {
    "Aguascalientes", "Baja California", "Baja California Sur", "Campeche", 
    "Chiapas", "Chihuahua", "Ciudad de México", "Coahuila", "Colima", 
    "Durango", "Estado de México", "Guanajuato", "Guerrero", "Hidalgo", 
    "Jalisco", "Michoacán", "Morelos", "Nayarit", "Nuevo León", "Oaxaca", 
    "Puebla", "Querétaro", "Quintana Roo", "San Luis Potosí", "Sinaloa", 
    "Sonora", "Tabasco", "Tamaulipas", "Tlaxcala", "Veracruz", "Yucatán", "Zacatecas"
};
const int NUM_STATES = 32;

// Estructura del carrito
typedef struct NodoCarrito {
    Prenda *prenda;
    struct NodoCarrito *siguiente;
} NodoCarrito;
NodoCarrito *carrito_global = NULL;
GtkWidget *global_lbl_total_carrito = NULL;
GtkWidget *global_box_items_carrito = NULL;

// Widgets de Auth
typedef struct {
    GtkWidget *entry_user;
    GtkWidget *entry_pass;
    GtkWidget *entry_pass_confirm; 
    GtkWidget *entry_phone;   
    GtkWidget *entry_email;        
    GtkWidget *combo_location;
    GtkWidget *stack;
    GtkWidget *window;
    GtkWidget *navbar_auth_area; 
} AuthWidgets;

// Widgets de Checkout
typedef struct {
    GtkWidget *entry_address;
    GtkWidget *entry_postal;
    GtkWidget *entry_card_num;
    GtkWidget *combo_card_type;
    GtkWidget *stack;
    GtkWidget *window;
} CheckoutWidgets;

// Prototipos
void cerrar_sesion(GtkWidget *widget, gpointer data);
double calcular_total_carrito(); 

// ====================================================================
// LÓGICA DE CARRITO
// ====================================================================
void agregar_al_carrito_logic(Prenda *p) {
    NodoCarrito *nuevo = g_malloc(sizeof(NodoCarrito));
    nuevo->prenda = p; 
    nuevo->siguiente = carrito_global;
    carrito_global = nuevo;
}

double calcular_total_carrito() {
    double total = 0.0;
    NodoCarrito *actual = carrito_global;
    while (actual != NULL) {
        total += actual->prenda->precio;
        actual = actual->siguiente;
    }
    return total;
}

void limpiar_carrito_logic() {
    NodoCarrito *actual = carrito_global;
    while (actual != NULL) {
        NodoCarrito *temp = actual;
        actual = actual->siguiente;
        g_free(temp);
    }
    carrito_global = NULL;
}

void mostrar_alerta(GtkWidget *window, const char *msg) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ====================================================================
// CALLBACKS DE PAGO Y CHECKOUT
// ====================================================================

void on_btn_place_order_clicked(GtkWidget *widget, gpointer data) {
    CheckoutWidgets *w = (CheckoutWidgets*)data;
    
    const char *addr = gtk_entry_get_text(GTK_ENTRY(w->entry_address));
    const char *postal = gtk_entry_get_text(GTK_ENTRY(w->entry_postal));
    const char *card = gtk_entry_get_text(GTK_ENTRY(w->entry_card_num));
    const char *type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w->combo_card_type));

    if (strlen(addr) < 5 || strlen(card) < 4) {
        mostrar_alerta(w->window, "Please fill in valid shipping and payment details.");
        if(type) g_free((gpointer)type);
        return;
    }

    // Guardar datos
    char full_address[200];
    snprintf(full_address, sizeof(full_address), "%s, CP %s", addr, postal);
    
    char saved_card[100];
    int len = strlen(card);
    const char *last4 = (len > 4) ? &card[len-4] : card;
    snprintf(saved_card, sizeof(saved_card), "%s ending in %s", type ? type : "Card", last4);

    strcpy(current_address, full_address);
    strcpy(current_card_info, saved_card);
    actualizarDatosUsuario(current_username, current_address, current_card_info);

    mostrar_alerta(w->window, "ORDER CONFIRMED!\nThank you for shopping with Louis Vuitton.\nA confirmation email has been sent.");
    
    limpiar_carrito_logic();
    gtk_stack_set_visible_child_name(GTK_STACK(w->stack), "page_novedades");
    
    if(type) g_free((gpointer)type);
}


// ====================================================================
// CALLBACKS DE BORRADO DE CUENTA
// ====================================================================

void realizar_borrado(GtkWidget *dialog, gint response_id, gpointer user_data) {
    AuthWidgets *w = (AuthWidgets*)user_data;
    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkWidget *entry_pass = g_object_get_data(G_OBJECT(dialog), "entry_pass");
        const char *pass = gtk_entry_get_text(GTK_ENTRY(entry_pass));
        
        if (eliminarUsuario(current_username, pass)) {
            gtk_widget_destroy(dialog);
            mostrar_alerta(w->window, "Account deleted successfully. Goodbye.");
            cerrar_sesion(NULL, w);
        } else {
            mostrar_alerta(w->window, "Incorrect password. Deletion failed.");
            gtk_widget_destroy(dialog); 
        }
    } else {
        gtk_widget_destroy(dialog);
    }
}

void on_btn_delete_account_clicked(GtkWidget *widget, gpointer data) {
    AuthWidgets *w = (AuthWidgets*)data;
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Confirm Deletion",
                                                    GTK_WINDOW(w->window),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Cancel", GTK_RESPONSE_REJECT,
                                                    "DELETE", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new("Enter your password to confirm account deletion.\nThis action cannot be undone.");
    GtkWidget *entry_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "Password");
    
    gtk_container_add(GTK_CONTAINER(content_area), label);
    gtk_container_add(GTK_CONTAINER(content_area), entry_pass);
    g_object_set_data(G_OBJECT(dialog), "entry_pass", entry_pass);
    
    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(realizar_borrado), w);
}

// ====================================================================
// INTERFAZ CHECKOUT (NUEVA PÁGINA A DOS COLUMNAS)
// ====================================================================
GtkWidget *global_box_checkout_items = NULL;
GtkWidget *lbl_total_pay;
GtkWidget *lbl_total_checkout = NULL;  // Label que mostrará el total en checkout

GtkWidget *entry_address;
GtkWidget *entry_postal;
GtkWidget *entry_card;
GtkWidget *box_card_meta; 
GtkWidget *entry_exp;
GtkWidget *entry_cvv;

void actualizar_checkout() {
    // Limpia el contenedor
    gtk_container_foreach(GTK_CONTAINER(global_box_checkout_items), (GtkCallback)gtk_widget_destroy, NULL);

    NodoCarrito *nodo = carrito_global;
    double total =0.0;
    while (nodo != NULL) {

        // Caja horizontal para cada item
        GtkWidget *item_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        // IMAGEN
        GtkWidget *img = NULL;
        if (nodo->prenda->urlImagen != NULL && strlen(nodo->prenda->urlImagen) > 0) {
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
                nodo->prenda->urlImagen,
                80, 80,
                TRUE,
                NULL
            );
            if (pixbuf) {
                img = gtk_image_new_from_pixbuf(pixbuf);
                g_object_unref(pixbuf);
            } else {
                img = gtk_label_new("[Sin imagen]");
            }
        } else {
            img = gtk_label_new("[Sin imagen]");
        }

        gtk_box_pack_start(GTK_BOX(item_box), img, FALSE, FALSE, 0);


        
        // NOMBRE + PRECIO
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s - $%.2f", nodo->prenda->nombre, nodo->prenda->precio);
        GtkWidget *lbl = gtk_label_new(buffer);
        gtk_box_pack_start(GTK_BOX(item_box), lbl, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(global_box_checkout_items), item_box, FALSE, FALSE, 5);
        total+=nodo->prenda->precio;
        nodo = nodo->siguiente;
    }

    
printf("%f\n", total);
    // Actualizamos el label global
    char total_buffer[50];
    snprintf(total_buffer, sizeof(total_buffer), "TOTAL TO PAY: $%.2f", total);
    gtk_label_set_text(GTK_LABEL(lbl_total_pay), total_buffer);
    gtk_widget_show(lbl_total_pay);                 // asegúrate de mostrar el label



    //texto default
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_address), "Escribe tu direccion");
    //Texto guardado
    gtk_entry_set_text(GTK_ENTRY(entry_address), current_address);
    gtk_widget_show(entry_address);

    //texto default
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_postal), "Escribe tu CP");
    //Texto guardado
    gtk_entry_set_text(GTK_ENTRY(entry_postal), current_card_info);
    gtk_widget_show(entry_postal);


    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_card), "Ingresa tu tarjeta"); // Placeholder
// gtk_entry_set_text(GTK_ENTRY(entry_card), ); // Texto inicial
//     gtk_widget_show(entry_card);
    
    gtk_widget_show_all(global_box_checkout_items);

    
}
// Label para mostrar el total del carrito

void on_btn_pagar_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *stack = (GtkWidget*)data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);

    if (carrito_global == NULL) { 
        mostrar_alerta(toplevel, "YOUR BAG IS EMPTY"); 
        return; 
    }

    if (strcmp(current_username, "INVITADO") == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            "Please SIGN IN or REGISTER to complete your purchase.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_login");
    } else {
        
        actualizar_checkout();
        //refrescar_vista_carrito();
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_checkout");
        // IMPORTANTE: LLENAR CON ITEMS DEL CARRITO
    
    }
}



GtkWidget* create_checkout_page(GtkWidget *stack, GtkWidget *window) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 900, -1); 
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_add(GTK_CONTAINER(scroll), main_vbox);
    gtk_widget_set_margin_top(main_vbox, 40);
    gtk_widget_set_margin_bottom(main_vbox, 40);
    gtk_widget_set_halign(main_vbox, GTK_ALIGN_CENTER);

    // Titulo
    GtkWidget *lbl_title = gtk_label_new("SECURE CHECKOUT");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "auth-title");
    gtk_box_pack_start(GTK_BOX(main_vbox), lbl_title, FALSE, FALSE, 20);

    // Contenedor Horizontal (2 Columnas)
    GtkWidget *hbox_columns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 50); 
    gtk_box_pack_start(GTK_BOX(main_vbox), hbox_columns, TRUE, TRUE, 0);

    // --- COLUMNA IZQUIERDA: FORMULARIO ---
    GtkWidget *col_form = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_size_request(col_form, 400, -1);
    gtk_box_pack_start(GTK_BOX(hbox_columns), col_form, FALSE, FALSE, 0);

    // Envío
    GtkWidget *lbl_ship = gtk_label_new("SHIPPING ADDRESS");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_ship), "auth-subtitle");
    gtk_widget_set_halign(lbl_ship, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(col_form), lbl_ship, FALSE, FALSE, 5);

    entry_address = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_address), "Street Address");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_address), "auth-entry");
    
    entry_postal = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_postal), "Postal Code");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_postal), "auth-entry");

    gtk_box_pack_start(GTK_BOX(col_form), entry_address, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(col_form), entry_postal, FALSE, FALSE, 10);

    // Pago
    GtkWidget *lbl_pay = gtk_label_new("PAYMENT METHOD");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_pay), "auth-subtitle");
    gtk_widget_set_halign(lbl_pay, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(col_form), lbl_pay, FALSE, FALSE, 5);

    GtkWidget *combo_type = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_type), "Credit Card", "Credit Card");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_type), "Debit Card", "Debit Card");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_type), "PayPal", "PayPal");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_type), 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(combo_type), "auth-combo-hybrid");
    
   

    entry_card = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_card), "Card Number");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_card), "auth-entry");
    


    box_card_meta = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    entry_exp = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_exp), "MM/YY");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_exp), "auth-entry");
    
    entry_cvv = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_cvv), "CVV");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry_cvv), "auth-entry");
    gtk_entry_set_visibility(GTK_ENTRY(entry_cvv), FALSE);

    gtk_box_pack_start(GTK_BOX(box_card_meta), entry_exp, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box_card_meta), entry_cvv, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(col_form), combo_type, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(col_form), entry_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(col_form), box_card_meta, FALSE, FALSE, 20);


    // --- COLUMNA DERECHA: RESUMEN ---
    GtkWidget *col_summary = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_size_request(col_summary, 300, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(col_summary), "auth-card"); 
    gtk_box_pack_start(GTK_BOX(hbox_columns), col_summary, FALSE, FALSE, 0);

    GtkWidget *lbl_sum_title = gtk_label_new("ORDER SUMMARY");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_sum_title), "auth-subtitle");
    gtk_box_pack_start(GTK_BOX(col_summary), lbl_sum_title, FALSE, FALSE, 10);

    GtkWidget *lbl_items = gtk_label_new("Items in Bag");
    gtk_widget_set_halign(lbl_items, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(col_summary), lbl_items, FALSE, FALSE, 0);
    global_box_checkout_items = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
gtk_box_pack_start(GTK_BOX(col_summary), global_box_checkout_items, FALSE, FALSE, 5);


    


    double total = calcular_total_carrito(); 
    char total_str[100];
    snprintf(total_str, sizeof(total_str), "TOTAL TO PAY: $%.2f", total);
    lbl_total_pay = gtk_label_new(total_str);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_total_pay), "cart-total");
    gtk_box_pack_start(GTK_BOX(col_summary), lbl_total_pay, FALSE, FALSE, 20);



    GtkWidget *btn_confirm = gtk_button_new_with_label("PLACE ORDER");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_confirm), "boton-lv-primary");
    gtk_box_pack_start(GTK_BOX(col_summary), btn_confirm, FALSE, FALSE, 0);

    CheckoutWidgets *w = g_malloc(sizeof(CheckoutWidgets));
    w->entry_address = entry_address;
    w->entry_postal = entry_postal;
    w->entry_card_num = entry_card;
    w->combo_card_type = combo_type;
    w->stack = stack;
    w->window = window;

    g_signal_connect(btn_confirm, "clicked", G_CALLBACK(on_btn_place_order_clicked), w);

    return scroll;
}

// ====================================================================
// FUNCIONES HELPERS Y AUTH
// ====================================================================

void on_cell_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    Prenda *p = (Prenda*)data;
    agregar_al_carrito_logic(p);
    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
    if (gtk_widget_is_toplevel(toplevel)) mostrar_alerta(toplevel, "ITEM ADDED TO BAG");
}

void refrescar_vista_carrito() {
    if (!global_box_items_carrito) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(global_box_items_carrito));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    NodoCarrito *actual = carrito_global;
    if (actual == NULL) {
        GtkWidget *lbl = gtk_label_new("YOUR SHOPPING BAG IS EMPTY");
        gtk_box_pack_start(GTK_BOX(global_box_items_carrito), lbl, TRUE, TRUE, 20);
    }
    while (actual != NULL) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
        gtk_style_context_add_class(gtk_widget_get_style_context(row), "cart-item-row");
        
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(actual->prenda->urlImagen ? actual->prenda->urlImagen : "./imagenes/default.jpg", 60, 60, TRUE, NULL);
        if (!pb) pb = gdk_pixbuf_new_from_file_at_scale("./imagenes/default.jpg", 60, 60, TRUE, NULL);
        
        GtkWidget *img = gtk_image_new_from_pixbuf(pb);
        GtkWidget *lbl_name = gtk_label_new(actual->prenda->nombre);
        gtk_widget_set_halign(lbl_name, GTK_ALIGN_START);
        char precio_str[50];
        snprintf(precio_str, sizeof(precio_str), "$%.2f %s", actual->prenda->precio, actual->prenda->moneda);
        GtkWidget *lbl_price = gtk_label_new(precio_str);
        
        gtk_box_pack_start(GTK_BOX(row), img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), lbl_name, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(row), lbl_price, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(global_box_items_carrito), row, FALSE, FALSE, 5);
        actual = actual->siguiente;
    }
    gtk_widget_show_all(global_box_items_carrito);

    if (global_lbl_total_carrito) {
        double total = calcular_total_carrito();
        char total_str[100];
        snprintf(total_str, sizeof(total_str), "ESTIMATED TOTAL: $%.2f", total);
        gtk_label_set_text(GTK_LABEL(global_lbl_total_carrito), total_str);
    }
}

// CSV Helpers
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) { FILE *fp = (FILE *)data; return fwrite(ptr, size, nmemb, fp); }
int descargarCSV(const char *url, const char *archivoSalida) {
    CURL *curl = curl_easy_init(); if (!curl) return 0;
    FILE *fp = fopen(archivoSalida, "wb"); if (!fp) { curl_easy_cleanup(curl); return 0; }
    curl_easy_setopt(curl, CURLOPT_URL, url); curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp); curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl); fclose(fp); curl_easy_cleanup(curl); return (res == CURLE_OK);
}
void cargarDesdeCSV(const char *archivo, ListaRopa *lista) {
    FILE *f = fopen(archivo, "r"); if (!f) return;
    char linea[2000]; fgets(linea, sizeof(linea), f);
    while (fgets(linea, sizeof(linea), f)) {
        linea[strcspn(linea, "\n\r")] = 0;
        char *id = strtok(linea, ","); char *marca = strtok(NULL, ","); char *nombre = strtok(NULL, ","); char *precioSTR = strtok(NULL, ","); char *moneda = strtok(NULL, ","); char *urlImagen = strtok(NULL, ","); char *genero = strtok(NULL, ","); char *categoria = strtok(NULL, ","); char *coleccion = strtok(NULL, ",");
        if (!id || !precioSTR) continue;
        agregarAlFinal(lista, crearPrenda(id, marca, nombre, atof(precioSTR), moneda, urlImagen, genero, categoria, coleccion));
    } fclose(f);
}


//Para actulizar page user cada vez que se autentica el usuario


GtkWidget *lbl_user_value;
GtkWidget *lbl_email_value;
GtkWidget *lbl_phone_value;
GtkWidget *lbl_location_value;
GtkWidget *lbl_address_value;
GtkWidget *lbl_card_value;

GtkWidget* create_user_page(GtkWidget *window, GtkWidget *navbar_auth_area, GtkWidget *stack) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(box, 30);

    GtkWidget *lbl_title = gtk_label_new("MY PROFILE");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "auth-title");
    gtk_box_pack_start(GTK_BOX(box), lbl_title, FALSE, FALSE, 10);

    // -------------------------
    // USER
    // -------------------------
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Current User:"), FALSE, FALSE, 0);
    lbl_user_value = gtk_label_new(current_username);
    gtk_box_pack_start(GTK_BOX(box), lbl_user_value, FALSE, FALSE, 0);

    // -------------------------
    // EMAIL
    // -------------------------
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Email:"), FALSE, FALSE, 0);
    lbl_email_value = gtk_label_new(current_email);
    gtk_box_pack_start(GTK_BOX(box), lbl_email_value, FALSE, FALSE, 0);

    // -------------------------
    // PHONE
    // -------------------------
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Phone:"), FALSE, FALSE, 0);
    lbl_phone_value = gtk_label_new(current_phone);
    gtk_box_pack_start(GTK_BOX(box), lbl_phone_value, FALSE, FALSE, 0);

    // -------------------------
    // LOCATION
    // -------------------------
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Location (State):"), FALSE, FALSE, 0);
    lbl_location_value = gtk_label_new(current_location);
    gtk_box_pack_start(GTK_BOX(box), lbl_location_value, FALSE, FALSE, 0);

    // -------------------------
    // ADDRESS
    // -------------------------
    gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("SAVED SHIPPING ADDRESS:"), FALSE, FALSE, 0);
    lbl_address_value = gtk_label_new(current_address[0] ? current_address : "Not Set");
    gtk_box_pack_start(GTK_BOX(box), lbl_address_value, FALSE, FALSE, 0);

    // -------------------------
    // CARD
    // -------------------------
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("SAVED PAYMENT METHOD:"), FALSE, FALSE, 0);
    lbl_card_value = gtk_label_new(current_card_info[0] ? current_card_info : "Not Set");
    gtk_box_pack_start(GTK_BOX(box), lbl_card_value, FALSE, FALSE, 0);

    return box;
}


void update_user_page() {
    gtk_label_set_text(GTK_LABEL(lbl_user_value), current_username);
    gtk_label_set_text(GTK_LABEL(lbl_email_value), current_email);
    gtk_label_set_text(GTK_LABEL(lbl_phone_value), current_phone);
    gtk_label_set_text(GTK_LABEL(lbl_location_value), current_location);
    gtk_label_set_text(GTK_LABEL(lbl_address_value), current_address[0] ? current_address : "Not Set");
    gtk_label_set_text(GTK_LABEL(lbl_card_value), current_card_info[0] ? current_card_info : "Not Set");
}




// AUTH HELPERS
void update_username_display(const char *name) {
    if (global_lbl_username_display) {
        char buffer[200];
        char *upper_name = g_utf8_strup(name, -1);
        snprintf(buffer, sizeof(buffer), "%s", upper_name);
        g_free(upper_name);
        gtk_label_set_text(GTK_LABEL(global_lbl_username_display), buffer);
    }
}

void ir_a_registro(GtkWidget *widget, gpointer stack) { gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_register"); }
void ir_a_login(GtkWidget *widget, gpointer stack) { gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_login"); }

void cerrar_sesion(GtkWidget *widget, gpointer data) {
    AuthWidgets *w = (AuthWidgets*)data;
    if (w->navbar_auth_area) gtk_stack_set_visible_child_name(GTK_STACK(w->navbar_auth_area), "view_guest");
    
    strcpy(current_username, "INVITADO");
    strcpy(current_phone, "");
    strcpy(current_location, ""); 
    strcpy(current_email, "");
    strcpy(current_address, "");
    strcpy(current_card_info, "");
    
    update_username_display("INVITADO");
    
    if(w->entry_user) gtk_entry_set_text(GTK_ENTRY(w->entry_user), "");
    if(w->entry_pass) gtk_entry_set_text(GTK_ENTRY(w->entry_pass), "");
    
    gtk_stack_set_visible_child_name(GTK_STACK(w->stack), "page_novedades");
}

void on_btn_login_clicked(GtkWidget *widget, gpointer data) {
    AuthWidgets *w = (AuthWidgets*)data;
    const char *user = gtk_entry_get_text(GTK_ENTRY(w->entry_user));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(w->entry_pass));
    strcpy(current_username, user);

    if (validarLogin(user, pass)) {
        //strcpy(current_username, user);
        
        obtenerDatosUsuarioCompleto(user, current_phone, current_location, current_email, current_address, current_card_info);
        printf("Usuario registrado: %s\n", current_username);  // <-- Mejor impresión
        printf("Telefono registrado: %s\n", current_phone);  // <-- Mejor impresión
        printf("E.mailregistrado: %s\n", current_email);  // <-- Mejor impresión
        printf("direccion registrada: %s\n", current_address);  // <-- Mejor impresión
        //printf("card registrado: %s\n", current_card_info);  // <-- Mejor impresión
        update_username_display(user); 
        update_user_page();

        if (w->navbar_auth_area) gtk_stack_set_visible_child_name(GTK_STACK(w->navbar_auth_area), "view_logged");
        gtk_stack_set_visible_child_name(GTK_STACK(w->stack), "page_novedades");
    } else {
        mostrar_alerta(w->window, "Login Failed. Check credentials.");
    }
}

void on_btn_do_register_clicked(GtkWidget *widget, gpointer data) {
    AuthWidgets *w = (AuthWidgets*)data;
    const char *user = gtk_entry_get_text(GTK_ENTRY(w->entry_user));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(w->entry_pass));
    const char *conf = gtk_entry_get_text(GTK_ENTRY(w->entry_pass_confirm));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(w->entry_phone));
    const char *email = gtk_entry_get_text(GTK_ENTRY(w->entry_email));
    char *location = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w->combo_location));
    
    if (!location) location = g_strdup("Select Location");

    if (strcmp(pass, conf) != 0) {
        mostrar_alerta(w->window, "Passwords do not match.");
        g_free(location);
        return;
    }

    char errorMsg[200];
    if (registrarUsuario(user, pass, phone, location, email, errorMsg)) {
        // ✔ GUARDAR usuario actual
        strcpy(current_username, user);
        printf("Usuario registrado: %s\n", current_username);  // <-- Mejor impresión
        mostrar_alerta(w->window, "Account Created. Please Sign In.");
        update_user_page();
        
        obtenerDatosUsuarioCompleto(user, current_phone, current_location, current_email, current_address, current_card_info);
        gtk_stack_set_visible_child_name(GTK_STACK(w->stack), "page_user");
    } else {
        mostrar_alerta(w->window, errorMsg);
    }
    g_free(location);
}

// ====================================================================
// PÁGINAS UI RESTANTES
// ====================================================================

void on_combo_cycle(GtkWidget *btn, gpointer combo_widget) {
    GtkComboBox *combo = GTK_COMBO_BOX(combo_widget);
    int current = gtk_combo_box_get_active(combo);
    const char *op = gtk_button_get_label(GTK_BUTTON(btn));
    if (g_strcmp0(op, "<") == 0) current = (current - 1 + NUM_STATES) % NUM_STATES;
    else current = (current + 1) % NUM_STATES;
    gtk_combo_box_set_active(combo, current);
}

GtkWidget* create_register_page(GtkWidget *stack, GtkWidget *window) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll, 450, 550); 
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER); gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    
    GtkStyleContext *context = gtk_widget_get_style_context(box);
    gtk_style_context_add_class(context, "auth-card"); 

    GtkWidget *lbl_title = gtk_label_new("CREATE ACCOUNT");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "auth-title");
    gtk_box_pack_start(GTK_BOX(box), lbl_title, FALSE, FALSE, 10);

    GtkWidget *entry_user = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_user), "USERNAME"); gtk_style_context_add_class(gtk_widget_get_style_context(entry_user), "auth-entry");
    GtkWidget *entry_email = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_email), "EMAIL ADDRESS"); gtk_style_context_add_class(gtk_widget_get_style_context(entry_email), "auth-entry");
    GtkWidget *entry_pass = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "PASSWORD"); gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE); gtk_style_context_add_class(gtk_widget_get_style_context(entry_pass), "auth-entry");
    GtkWidget *entry_pass_conf = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass_conf), "CONFIRM PASSWORD"); gtk_entry_set_visibility(GTK_ENTRY(entry_pass_conf), FALSE); gtk_style_context_add_class(gtk_widget_get_style_context(entry_pass_conf), "auth-entry");
    GtkWidget *entry_phone = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_phone), "PHONE NUMBER"); gtk_style_context_add_class(gtk_widget_get_style_context(entry_phone), "auth-entry");

    GtkWidget *lbl_loc = gtk_label_new("SELECT LOCATION"); gtk_widget_set_halign(lbl_loc, GTK_ALIGN_START);
    GtkWidget *box_selector = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btn_prev = gtk_button_new_with_label("<"); GtkWidget *btn_next = gtk_button_new_with_label(">");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_prev), "nav-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "nav-button");
    GtkWidget *combo_states = gtk_combo_box_text_new();
    for(int i=0; i<NUM_STATES; i++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_states), MX_STATES[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_states), 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(combo_states), "auth-combo-hybrid");
    g_signal_connect(btn_prev, "clicked", G_CALLBACK(on_combo_cycle), combo_states);
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_combo_cycle), combo_states);
    gtk_box_pack_start(GTK_BOX(box_selector), btn_prev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_selector), combo_states, TRUE, TRUE, 10); 
    gtk_box_pack_start(GTK_BOX(box_selector), btn_next, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), entry_user, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), entry_email, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(box), entry_pass, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), entry_pass_conf, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(box), entry_phone, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), lbl_loc, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), box_selector, FALSE, FALSE, 10);

    GtkWidget *btn_reg = gtk_button_new_with_label("REGISTER"); gtk_style_context_add_class(gtk_widget_get_style_context(btn_reg), "boton-lv-primary");
    GtkWidget *btn_back = gtk_button_new_with_label("BACK TO LOGIN"); gtk_style_context_add_class(gtk_widget_get_style_context(btn_back), "boton-lv-text");
    gtk_box_pack_start(GTK_BOX(box), btn_reg, FALSE, FALSE, 15); gtk_box_pack_start(GTK_BOX(box), btn_back, FALSE, FALSE, 5);

    AuthWidgets *w = g_malloc(sizeof(AuthWidgets));
    w->entry_user = entry_user; w->entry_pass = entry_pass; w->entry_pass_confirm = entry_pass_conf;
    w->entry_email = entry_email; w->entry_phone = entry_phone; w->combo_location = combo_states;
    w->stack = stack; w->window = window;
    g_signal_connect(btn_reg, "clicked", G_CALLBACK(on_btn_do_register_clicked), w);
    
    g_signal_connect(btn_back, "clicked", G_CALLBACK(ir_a_login), stack);
    return scroll;
}

GtkWidget* create_login_page(GtkWidget *stack, GtkWidget *window, GtkWidget *navbar_auth_area) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER); gtk_widget_set_valign(box, GTK_ALIGN_CENTER); gtk_widget_set_size_request(box, 350, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "auth-card"); 
    GtkWidget *lbl_title = gtk_label_new("L O U I S  V U I T T O N"); gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "auth-title");
    gtk_box_pack_start(GTK_BOX(box), lbl_title, FALSE, FALSE, 10);
    GtkWidget *lbl_sub = gtk_label_new("MEMBER LOGIN"); gtk_style_context_add_class(gtk_widget_get_style_context(lbl_sub), "auth-subtitle");
    gtk_box_pack_start(GTK_BOX(box), lbl_sub, FALSE, FALSE, 20);
    GtkWidget *entry_user = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_user), "USERNAME"); gtk_style_context_add_class(gtk_widget_get_style_context(entry_user), "auth-entry");
    GtkWidget *entry_pass = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "PASSWORD"); gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE); gtk_style_context_add_class(gtk_widget_get_style_context(entry_pass), "auth-entry");
    gtk_box_pack_start(GTK_BOX(box), entry_user, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(box), entry_pass, FALSE, FALSE, 0);
    GtkWidget *btn_login = gtk_button_new_with_label("SIGN IN"); gtk_style_context_add_class(gtk_widget_get_style_context(btn_login), "boton-lv-primary");
    GtkWidget *btn_reg = gtk_button_new_with_label("CREATE AN ACCOUNT"); gtk_style_context_add_class(gtk_widget_get_style_context(btn_reg), "boton-lv-text");
    gtk_box_pack_start(GTK_BOX(box), btn_login, FALSE, FALSE, 15); gtk_box_pack_start(GTK_BOX(box), btn_reg, FALSE, FALSE, 5);
    AuthWidgets *w = g_malloc(sizeof(AuthWidgets)); w->entry_user = entry_user; w->entry_pass = entry_pass; w->stack = stack; w->window = window; w->navbar_auth_area = navbar_auth_area;
    g_signal_connect(btn_login, "clicked", G_CALLBACK(on_btn_login_clicked), w);
    g_signal_connect(btn_reg, "clicked", G_CALLBACK(ir_a_registro), stack);
    return box;
}

GtkWidget* create_cart_page(GtkWidget *stack, GtkWidget *window) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_valign(box, GTK_ALIGN_START); gtk_widget_set_halign(box, GTK_ALIGN_CENTER); gtk_widget_set_size_request(box, 700, -1);
    GtkWidget *lbl_title = gtk_label_new("SHOPPING BAG"); gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "auth-title");
    gtk_box_pack_start(GTK_BOX(box), lbl_title, FALSE, FALSE, 20);
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL); gtk_widget_set_size_request(scrolled, -1, 400); gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    global_box_items_carrito = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); gtk_container_add(GTK_CONTAINER(scrolled), global_box_items_carrito);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 10);
    global_lbl_total_carrito = gtk_label_new("TOTAL ESTIMADO: $0.00"); gtk_style_context_add_class(gtk_widget_get_style_context(global_lbl_total_carrito), "cart-total");
    gtk_box_pack_start(GTK_BOX(box), global_lbl_total_carrito, FALSE, FALSE, 10);
    GtkWidget *btn_pagar = gtk_button_new_with_label("PROCEED TO CHECKOUT"); 
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_pagar), "boton-lv-primary");
    
    
    // Pasamos 'stack' para poder redirigir
    g_signal_connect(btn_pagar, "clicked", G_CALLBACK(on_btn_pagar_clicked), stack);
    gtk_box_pack_start(GTK_BOX(box), btn_pagar, FALSE, FALSE, 10);
    return box;
}


// ====================================================================
// FUNCIONES RESTAURADAS Y UI HELPERS (Sin cambios)
// ====================================================================

GtkWidget *create_content_label(const gchar *text, const gchar *css_class) {
    GtkWidget *label = gtk_label_new(text);
    GtkStyleContext *context = gtk_widget_get_style_context(label);
    if (css_class && *css_class != '\0') {
        gtk_style_context_add_class(context, css_class); 
    }
    gtk_widget_set_vexpand(label, TRUE); 
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER); 
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    return label;
}

GtkWidget* create_cell(Prenda *prenda, int index) {
    GtkWidget *event = gtk_event_box_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(event), box);

    char rutaImagen[256];
    if(strcmp("Hombre", prenda->genero) == 0){
        snprintf(rutaImagen, sizeof(rutaImagen), "./imagenes/imagenesHombre/imagen_%03d.jpg", index);
        strcpy(prenda->urlImagen, rutaImagen);
    }else{
        if(strcmp("Mujer", prenda->genero) == 0){
            snprintf(rutaImagen, sizeof(rutaImagen), "./imagenes/imagenesMujer/imagen_%03d.jpg", index);
            strcpy(prenda->urlImagen, rutaImagen);
        }
    }
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(prenda->urlImagen ? prenda->urlImagen : rutaImagen, 120, 120, TRUE, NULL);
    if (!pixbuf) {
        pixbuf = gdk_pixbuf_new_from_file_at_scale("p.jpg", 120, 120, TRUE, NULL);
    }

    GtkWidget *img = gtk_image_new_from_pixbuf(pixbuf);
    gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);

    GtkWidget *label = gtk_label_new(prenda->nombre);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    
    g_signal_connect(event, "button-press-event", G_CALLBACK(on_cell_clicked), prenda);

    return event;
}

GtkWidget* create_scrolleable_grid_prendas(ListaRopa *lista, int columnas) {
    if (lista == NULL || lista->raiz == NULL) {
        return gtk_label_new("No hay prendas para mostrar");
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);
    
    gtk_widget_set_size_request(grid, 600, -1);
    gtk_container_add(GTK_CONTAINER(scroll), grid);

    Prenda *actual = lista->raiz;
    int r = 0, c = 0;
    int index = 1;

    while (actual != NULL) {
        GtkWidget *cell = create_cell(actual, index);
        gtk_grid_attach(GTK_GRID(grid), cell, c, r, 1, 1);

        actual = actual->siguiente;
        index++;
        c++;
        if (c >= columnas) {
            c = 0;
            r++;
        }
    }
    //imprimirListaRopa(lista);
    return scroll;
}


// Helpers navegación
void on_nav_item_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = (GtkStack *)user_data;
    const gchar *page_name = gtk_widget_get_name(GTK_WIDGET(button));
    gtk_stack_set_visible_child_name(stack, page_name);
    if (g_strcmp0(page_name, "page_carrito") == 0) refrescar_vista_carrito();
    
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
    while (parent && !GTK_IS_POPOVER(parent)) parent = gtk_widget_get_parent(parent);
    if (GTK_IS_POPOVER(parent)) gtk_popover_popdown(GTK_POPOVER(parent));
}

GtkWidget *create_nav_button(GtkStack *stack, const gchar *label, const gchar *page_name) {
    GtkWidget *button = gtk_button_new_with_label(label);
    gtk_widget_set_name(button, page_name);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "nav-button");
    g_signal_connect(button, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
    return button;
}
GtkWidget *create_icon_nav_button(GtkStack *stack, const gchar *icon_name, const gchar *page_name) {
    GtkWidget *button = gtk_button_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU); 
    gtk_widget_set_name(button, page_name);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "nav-button");
    g_signal_connect(button, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
    return button;
}

// Helper para Carrusel
typedef struct { GtkStack *stack; const gchar **names; gint count; } CarouselData;
const gchar *IMAGE_PATHS[] = { "./imagenesCarrusel/chamarra.jpg", "./imagenesCarrusel/jerseyAmericano.jpg", "./imagenesCarrusel/chamarraMezclilla.jpg", "./imagenesCarrusel/sudaderaCafe.jpg" };
const gchar *IMAGE_NAMES[] = { "img_1", "img_2", "img_3", "img_4" };

// ====================================================================
// NUEVA LÓGICA DE CARRUSEL CÍCLICO (Imágenes)
// ====================================================================
static void on_carrusel_button_clicked(GtkButton *button, gpointer user_data) {
    CarouselData *data = (CarouselData *)user_data;
    GtkStack *stack = data->stack;
    const gchar *current_name = gtk_stack_get_visible_child_name(stack);
    gint current_index = -1;
    
    for (gint i = 0; i < data->count; i++) {
        if (g_strcmp0(current_name, data->names[i]) == 0) {
            current_index = i;
            break;
        }
    }
    
    gint next_index = -1;
    const gchar *button_label = gtk_button_get_label(button);
    bool is_forward = (g_strcmp0(button_label, ">") == 0);

    if (is_forward) {
        next_index = (current_index + 1) % data->count;
        gtk_stack_set_transition_type(stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT);
    } else {
        next_index = (current_index - 1 + data->count) % data->count; 
        gtk_stack_set_transition_type(stack, GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
    }

    if (next_index != -1) {
        gtk_stack_set_visible_child_name(stack, data->names[next_index]);
    }
}

static void on_carrusel_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    CarouselData *data = (CarouselData *)user_data;
    GtkStack *stack = data->stack;
    const gchar *current_name = gtk_stack_get_visible_child_name(stack);
    gint current_index = -1;
    
    for (gint i = 0; i < data->count; i++) {
        if (g_strcmp0(current_name, data->names[i]) == 0) {
            current_index = i;
            break;
        }
    }
    
    gint next_index = -1;
    
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_LEFT) {
        next_index = (current_index - 1 + data->count) % data->count; 
        gtk_stack_set_transition_type(stack, GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
    } else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_RIGHT) {
        next_index = (current_index + 1) % data->count;
        gtk_stack_set_transition_type(stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT);
    }

    if (next_index != -1) {
        gtk_stack_set_visible_child_name(stack, data->names[next_index]);
    }
}

void cargar_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./style.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

static void activate (GtkApplication* app, gpointer user_data) {
    cargar_css();
    ListaRopa *lista = crearListaRopa();
    //if (descargarCSV("https://docs.google.com/spreadsheets/d/e/2PACX-1vTQuF8mUFi66yyufmwTLU2bx4bwlUA_XAOLuxh2xIkx50a7uGZsFFRDN8Opn7-B32MeNdKkJRsqRsjb/pub?output=csv", "ropa.csv")) {
        cargarDesdeCSV("ropa.csv", lista);
    //}
    

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "LV STORE");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // NAVBAR PRINCIPAL
    GtkWidget *navbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15); 
    gtk_widget_set_halign(navbar_box, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(navbar_box), "navbar"); 
    gtk_box_pack_start(GTK_BOX(vbox), navbar_box, FALSE, FALSE, 0); 
    
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT); 
    gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0); 
    gtk_widget_set_vexpand(stack, TRUE);

    // STACK DE AUTH (Solo login/logout)
    GtkWidget *navbar_auth_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(navbar_auth_stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);

    // --- CONSTRUCCIÓN NAVBAR ---
    gtk_box_pack_start(GTK_BOX(navbar_box), create_nav_button(stack, "PROXIMAMENTE", "page_novedades"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(navbar_box), create_nav_button(stack, "MUJER", "page_mujer"), FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(navbar_box), create_nav_button(stack, "HOMBRE", "page_hombre"), FALSE, FALSE, 0);

    GtkWidget *spacer = gtk_label_new(""); gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(navbar_box), spacer, TRUE, TRUE, 0);

    global_lbl_username_display = gtk_label_new("INVITADO");
    gtk_box_pack_start(GTK_BOX(navbar_box), global_lbl_username_display, FALSE, FALSE, 15);

    GtkWidget *btn_bag = create_nav_button(stack, "SHOPPING BAG", "page_carrito");
    gtk_box_pack_start(GTK_BOX(navbar_box), btn_bag, FALSE, FALSE, 5);

    GtkWidget *box_guest = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *btn_go_login = gtk_button_new_with_label("LOGIN");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_go_login), "nav-button");
    g_signal_connect(btn_go_login, "clicked", G_CALLBACK(ir_a_login), stack);
    gtk_box_pack_start(GTK_BOX(box_guest), btn_go_login, FALSE, FALSE, 0);

    GtkWidget *box_logged = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btn_profile = create_icon_nav_button(stack, "avatar-default", "page_user");
    GtkWidget *btn_logout = gtk_button_new_with_label("LOGOUT");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_logout), "nav-button");
    
    AuthWidgets *logout_w = g_new(AuthWidgets, 1);
    logout_w->stack = stack; logout_w->window = window; logout_w->navbar_auth_area = navbar_auth_stack; 
    logout_w->entry_user = gtk_entry_new(); logout_w->entry_pass = gtk_entry_new();
    g_signal_connect(btn_logout, "clicked", G_CALLBACK(cerrar_sesion), logout_w);

    gtk_box_pack_start(GTK_BOX(box_logged), btn_profile, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_logged), btn_logout, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(navbar_auth_stack), box_guest, "view_guest");
    gtk_stack_add_named(GTK_STACK(navbar_auth_stack), box_logged, "view_logged");
    gtk_box_pack_start(GTK_BOX(navbar_box), navbar_auth_stack, FALSE, FALSE, 0);

    // --- AGREGAR PÁGINAS ---
    gtk_stack_add_named(GTK_STACK(stack), create_login_page(stack, window, navbar_auth_stack), "page_login");
    gtk_stack_add_named(GTK_STACK(stack), create_register_page(stack, window), "page_register");
    
    // Pasamos STACK al creador del carrito para poder navegar
    gtk_stack_add_named(GTK_STACK(stack), create_cart_page(stack, window), "page_carrito");
    
    gtk_stack_add_named(GTK_STACK(stack), create_user_page(window, navbar_auth_stack, stack), "page_user");
    // NUEVA: Página de Checkout
    gtk_stack_add_named(GTK_STACK(stack), create_checkout_page(stack, window), "page_checkout");
    
    GtkWidget *gridCarrusel = gtk_grid_new();
    gtk_widget_add_events(gridCarrusel, GDK_SCROLL_MASK);
    gtk_widget_set_hexpand(gridCarrusel, TRUE);
    gtk_widget_set_vexpand(gridCarrusel, TRUE);

    GtkWidget *stackImgs = gtk_stack_new();

    // ← centrado del stack
    gtk_widget_set_halign(stackImgs, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(stackImgs, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(stackImgs, TRUE);
    gtk_widget_set_vexpand(stackImgs, TRUE);

    CarouselData *cdata = g_new(CarouselData, 1);
    for(int i=0;i<4;i++) {
        GtkWidget *ic = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(
            IMAGE_PATHS[i], 600, 600, FALSE, NULL
        );
        gtk_container_add(GTK_CONTAINER(ic),
            pb ? gtk_image_new_from_pixbuf(pb)
            : gtk_label_new(IMAGE_NAMES[i])
        );

        gtk_stack_add_titled(GTK_STACK(stackImgs), ic, IMAGE_NAMES[i], IMAGE_NAMES[i]);
    }

    cdata->stack = GTK_STACK(stackImgs); 
    cdata->names = IMAGE_NAMES;
    cdata->count = 4;

    // botones
    GtkWidget *b1 = gtk_button_new_with_label("<");
    GtkWidget *b2 = gtk_button_new_with_label(">");

    g_signal_connect(b1, "clicked", G_CALLBACK(on_carrusel_button_clicked), cdata);
    g_signal_connect(b2, "clicked", G_CALLBACK(on_carrusel_button_clicked), cdata);
    g_signal_connect(gridCarrusel, "scroll-event", G_CALLBACK(on_carrusel_scroll), cdata);

    // evitar que los botones estiren el grid
    gtk_widget_set_halign(b1, GTK_ALIGN_START);
    gtk_widget_set_valign(b1, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(b1, FALSE);

    gtk_widget_set_halign(b2, GTK_ALIGN_END);
    gtk_widget_set_valign(b2, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(b2, FALSE);

    // agregar al grid
    gtk_grid_attach(GTK_GRID(gridCarrusel), b1,       0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(gridCarrusel), stackImgs,1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(gridCarrusel), b2,       2, 0, 1, 1);

    gtk_stack_add_named(GTK_STACK(stack), gridCarrusel, "page_novedades");


    
    gtk_stack_add_named(GTK_STACK(stack),create_content_label("MUJER SECTION", "mujer") , "page_mujer");
    gtk_stack_add_named(GTK_STACK(stack), create_scrolleable_grid_prendas(lista, 3) , "page_hombre");

    gtk_widget_show_all(window);
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_novedades");
    gtk_stack_set_visible_child_name(GTK_STACK(navbar_auth_stack), "view_guest");
}

int main (int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.gtk.tienda", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}