#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h> 
#include<stdlib.h>
#include "prendas.h"
#include <libsoup-3.0/libsoup/soup.h>
#include<string.h>
#include<curl/curl.h>
#include "listaRopa.h"

void on_cell_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data) {

    Prenda *p = (Prenda*)data;

    printf("Hiciste clic en: %s (ID %d)\n", p->nombre, p->id);

    // Aquí puedes abrir otra ventana, mostrar detalles, etc.
}



// ====================================================================
// Definimos Listas de Prendas, Hombre y mujer
// ====================================================================
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
    FILE *fp = (FILE *)data;
    return fwrite(ptr, size, nmemb, fp);
}

// ---------------------------
// Función de descarga CSV
// ---------------------------
int descargarCSV(const char *url, const char *archivoSalida) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: No se pudo inicializar CURL\n");
        return 0;
    }

    FILE *fp = fopen(archivoSalida, "wb");
    if (!fp) {
        perror("Error al crear archivo CSV");
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
        fclose(fp);
        curl_easy_cleanup(curl);
        return 0;
    }

    fclose(fp);
    curl_easy_cleanup(curl);
    return 1;
}

// ---------------------------
// Función de carga CSV a lista
// ---------------------------
void cargarDesdeCSV(const char *archivo, ListaRopa *lista) {
    FILE *f = fopen(archivo, "r");
    if (!f) {
        perror("Error al abrir CSV para lectura");
        return;
    }

    char linea[2000];
    // Saltar encabezado
    fgets(linea, sizeof(linea), f);

    while (fgets(linea, sizeof(linea), f)) {
        
        // **********************************************
        // SOLUCIÓN: Eliminar el '\n' al final de la línea
        // **********************************************
        // strcspn busca la primera ocurrencia de '\n' o '\r'
        // y lo reemplaza con el terminador de cadena '\0'.
        linea[strcspn(linea, "\n\r")] = 0; 

        // ----------------------------------------------------
        
        char *id = strtok(linea, ",");
        char *marca = strtok(NULL, ",");
        char *nombre = strtok(NULL, ",");
        char *precioSTR = strtok(NULL, ",");
        char *moneda = strtok(NULL, ","); // Este es el valor que creías perdido
        char *urlImagen = strtok(NULL, ",");
        char *genero = strtok(NULL, ",");
        char *categoria = strtok(NULL, ",");
        char *coleccion = strtok(NULL, ",");

        // VERIFICAR QUE SE HAYAN LEÍDO TODOS LOS CAMPOS
        if (!id || !marca || !nombre || !precioSTR || !moneda || !urlImagen || !genero || !categoria || !coleccion) {
            fprintf(stderr, "Advertencia: Fila incompleta o mal formada saltada.\n");
            continue; // Saltar a la siguiente línea si faltan campos
        }

        float precio = atof(precioSTR);

        Prenda *p = crearPrenda(id, marca, nombre, precio, moneda, urlImagen, genero, categoria, coleccion);
        agregarAlFinal(lista, p);
    }

    fclose(f);
}



// ====================================================================
// CONFIGURACIÓN DE IMÁGENES Y TAMAÑOS
// ====================================================================

#define TARGET_WIDTH 600
#define TARGET_HEIGHT 600

// Definición de las rutas de imagen 
const gchar *IMAGE_PATHS[] = {
    "/home/edu-gar/Escritorio/ProyectoSistemasOperativos/imagenes/imagenesCarrusel/chamarra.jpg", // IMAGEN 1
    "/home/edu-gar/Escritorio/ProyectoSistemasOperativos/imagenes/imagenesCarrusel/jerseyAmericano.jpg", // IMAGEN 2
    "/home/edu-gar/Escritorio/ProyectoSistemasOperativos/imagenes/imagenesCarrusel/chamarraMezclilla.jpg",  // IMAGEN 3
    "/home/edu-gar/Escritorio/ProyectoSistemasOperativos/imagenes/imagenesCarrusel/sudaderaCafe.jpg",  // IMAGEN 4
};
const gchar *IMAGE_NAMES[] = { "img_1", "img_2", "img_3", "img_4" };
const int NUM_IMAGES = G_N_ELEMENTS(IMAGE_PATHS);


// ====================================================================
// ESTRUCTURAS Y CALLBACKS
// ====================================================================

typedef struct {
    GtkStack *stack;
    const gchar **names;
    gint count;
} CarouselData;

// Callback para los botones del Carrusel
static void
on_carrusel_button_clicked(GtkButton *button, gpointer user_data)
{
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

    if (g_strcmp0(button_label, ">") == 0) {
        next_index = (current_index + 1) % data->count;
    } else if (g_strcmp0(button_label, "<") == 0) {
        next_index = (current_index - 1 + data->count) % data->count; 
    }

    if (next_index != -1) {
        gtk_stack_set_visible_child_name(stack, data->names[next_index]);
    }
}

// Callback para los botones de la barra de navegación (incluyendo sub-menús)
static void on_nav_item_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = (GtkStack *)user_data;
    // El nombre del widget contiene el ID de la página del stack a mostrar
    const gchar *page_name = gtk_widget_get_name(GTK_WIDGET(button));

    gtk_stack_set_visible_child_name(stack, page_name);
    
    // Si el botón está en un Popover, lo cerramos para buena UX
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
    while (parent && !GTK_IS_POPOVER(parent)) {
        parent = gtk_widget_get_parent(parent);
    }
    if (GTK_IS_POPOVER(parent)) {
        gtk_popover_popdown(GTK_POPOVER(parent));
    }
}


// ====================================================================
// FUNCIONES AUXILIARES DE NAVEGACIÓN Y CONTENIDO
// ====================================================================

// Crea un GtkButton simple para la navegación (con texto)
GtkWidget *create_nav_button(GtkStack *stack, const gchar *label, const gchar *page_name) {
    GtkWidget *button = gtk_button_new_with_label(label);
    gtk_widget_set_name(button, page_name);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "nav-button");
    g_signal_connect(button, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
    return button;
}

// Crea un GtkButton para el menú de usuario (con icono)
GtkWidget *create_icon_nav_button(GtkStack *stack, const gchar *icon_name, const gchar *page_name) {
    GtkWidget *button = gtk_button_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU); 
    gtk_widget_set_name(button, page_name);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "nav-button");
    g_signal_connect(button, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
    
    // Ajuste de margen para botones de icono
    gtk_widget_set_margin_start(button, 5);
    gtk_widget_set_margin_end(button, 5);
    
    return button;
}


// Crea un GtkMenuButton con un Popover para opciones desplegables
GtkWidget *create_menu_button(
    GtkStack *stack, 
    const gchar *main_label, 
    gint num_options, 
    const gchar *option_labels[], 
    const gchar *option_page_names[]) 
{
    GtkWidget *menu_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(menu_button), main_label);
    gtk_style_context_add_class(gtk_widget_get_style_context(menu_button), "menu-nav-button");

    GtkWidget *popover = gtk_popover_new(menu_button);
    GtkWidget *box_menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Crear botones dentro del Popover
    for (gint i = 0; i < num_options; i++) {
        GtkWidget *opt_button = gtk_button_new_with_label(option_labels[i]);
        gtk_widget_set_name(opt_button, option_page_names[i]); // ID de la sub-página
        gtk_style_context_add_class(gtk_widget_get_style_context(opt_button), "menu-item");

        g_signal_connect(opt_button, "clicked", G_CALLBACK(on_nav_item_clicked), stack);
        
        gtk_box_pack_start(GTK_BOX(box_menu), opt_button, TRUE, TRUE, 0);
    }

    gtk_container_add(GTK_CONTAINER(popover), box_menu);
    gtk_popover_set_modal(GTK_POPOVER(popover), FALSE); // No bloquea la aplicación
    
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);

    // Es importante mostrar todo el contenido del popover antes de usarlo
    gtk_widget_show_all(box_menu);
    
    return menu_button;
}

// Crea un GtkLabel con el texto y le aplica una clase CSS
GtkWidget *create_content_label(const gchar *text, const gchar *css_class) {

    GtkWidget *label = gtk_label_new(text);
    GtkStyleContext *context = gtk_widget_get_style_context(label);
    // Aplicar la clase CSS para el fondo/color (ej: 'mujer', 'hombre', 'alerta')
    if (css_class && *css_class != '\0') {
        gtk_style_context_add_class(context, css_class); 
    }
    // Asegurar que la etiqueta se centre y se expanda
    gtk_widget_set_vexpand(label, TRUE); 
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER); 
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    return label;
}


//PAra imagen desde Google Sheets



struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback para entregar a curl la memoria
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Descargar URL → Pixbuf
GdkPixbuf* load_pixbuf_from_url(const char *url) {

    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();

    if (!curl_handle)
        return NULL;

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "Error descargando %s: %s\n", url, curl_easy_strerror(res));
        curl_easy_cleanup(curl_handle);
        free(chunk.memory);
        return NULL;
    }

    curl_easy_cleanup(curl_handle);

    GInputStream *stream = g_memory_input_stream_new_from_data(chunk.memory, chunk.size, g_free);
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

    g_object_unref(stream);

    return pixbuf;
}


//Creamos cada celda del grid de ropa
GtkWidget* create_cell(Prenda *prenda) {

    GtkWidget *event = gtk_event_box_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(event), box);
    GdkPixbuf *pixbuf = load_pixbuf_from_url(prenda->urlImagen);

    if (pixbuf) {
        pixbuf = gdk_pixbuf_new_from_file_at_scale("/home/edu-gar/Escritorio/ProyectoSistemasOperativos/p.jpg", 120, 120, TRUE, NULL);
    }

    GtkWidget *img = gtk_image_new_from_pixbuf(pixbuf);
    gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);

    // TEXTO
    GtkWidget *label = gtk_label_new(prenda->nombre);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    return event;
}



//PAra crear Grid y mostrar imagenes
GtkWidget* create_scrolleable_grid_prendas(ListaRopa *lista, int columnas) {

    if (lista == NULL || lista->raiz == NULL) {
        return gtk_label_new("No hay prendas para mostrar");
    }

    // Contenedor con scroll
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,        // scroll horizontal fijo
                                   GTK_POLICY_AUTOMATIC);   // scroll vertical automático

    // Grid interno
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);   // horizontal
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);   // vertical

    gtk_container_add(GTK_CONTAINER(scroll), grid);

    // Recorrer la lista y generar celdas
    Prenda *actual = lista->raiz;
    int r = 0, c = 0;

    while (actual != NULL) {

        GtkWidget *cell = create_cell(actual);
        gtk_grid_attach(GTK_GRID(grid), cell, c, r, 1, 1);

        actual = actual->siguiente;

        c++;
        if (c >= columnas) {
            c = 0;
            r++;
        }
    }

    return scroll;
}


















// ====================================================================
// FUNCIÓN DE CSS Y ACTIVACIÓN PRINCIPAL
// ====================================================================

// RESTAURADO: Carga CSS desde el archivo local
void cargar_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    // RUTA DE ARCHIVO EXTERNO
    gtk_css_provider_load_from_path(provider, "/home/edu-gar/Escritorio/ProyectoSistemasOperativos/style.css", &error);

    if (error) {
        g_print("ERROR CSS: No se pudo cargar style.css: %s\n", error->message);
        g_error_free(error);
    } else {
        g_print("CSS cargado correctamente desde el archivo.\n");
    }

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    g_object_unref(provider);
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
    // Llama a la función para cargar el archivo CSS
    cargar_css();

    ListaRopa *listaActual = crearListaRopa();


    // URL del Google Sheets (CSV público)
    const char *url = "https://docs.google.com/spreadsheets/d/e/2PACX-1vTQuF8mUFi66yyufmwTLU2bx4bwlUA_XAOLuxh2xIkx50a7uGZsFFRDN8Opn7-B32MeNdKkJRsqRsjb/pub?output=csv";

    // Descargar CSV
    if (!descargarCSV(url, "ropa.csv")) {
        fprintf(stderr, "No se pudo descargar el CSV.\n");
        return 1;
    }
    printf("CSV descargado correctamente.\n");

    // Cargar datos a la lista
    cargarDesdeCSV("ropa.csv", listaActual);

    // Verificar cargado
    imprimirListaRopa(listaActual);



    

    GtkWidget *window;
    GtkWidget *vbox;        // 1. Contenedor principal
    GtkWidget *stack;       // 2. Stack principal (TODAS las páginas)
    GtkWidget *navbar_box;  // Contenedor para la barra de navegación personalizada
    
    // 4. Componentes del Carrusel (página NOVEDADES)
    GtkWidget *gridCarrusel; 
    GtkWidget *stackImagenes;
    GtkWidget *switcherCarrusel;
    GtkWidget *atras, *adelante;
    
    CarouselData *carousel_data = g_new(CarouselData, 1);
  
    // ==========================================
    // 1. CONFIGURACIÓN BASE DE LA VENTANA
    // ==========================================
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Ejemplo GTK: Navegación con Menús");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 800);

    // 2. Crear el Contenedor Vertical (Box) principal
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 3. Crear Stack principal (alojará todas las vistas)
    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT); 

    // **********************************************************
    // *** 4. BARRA DE NAVEGACIÓN PERSONALIZADA ***
    // **********************************************************
    navbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20); // 20px de espacio
    gtk_widget_set_halign(navbar_box, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(navbar_box), "navbar"); 

    // 5. Agregar la barra de navegación y el Stack al Box principal
    gtk_box_pack_start(GTK_BOX(vbox), navbar_box, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0); 
    gtk_widget_set_vexpand(stack, TRUE);


    // ==========================================
    // 6. CREACIÓN DEL CARRUSEL (PÁGINA NOVEDADES)
    // ==========================================
    
    gridCarrusel = gtk_grid_new();
    gtk_widget_set_name(gridCarrusel, "carrusel-principal"); 
    gtk_grid_set_row_spacing(GTK_GRID(gridCarrusel), 10);
    gtk_grid_set_column_spacing(GTK_GRID(gridCarrusel), 10);
    gtk_widget_set_valign(gridCarrusel, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(gridCarrusel, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(gridCarrusel, TRUE);

    stackImagenes = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stackImagenes), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT); 
    gtk_widget_set_vexpand(stackImagenes, TRUE); 

    switcherCarrusel = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcherCarrusel), GTK_STACK(stackImagenes));
    gtk_widget_set_halign(switcherCarrusel, GTK_ALIGN_CENTER); 

    atras = gtk_button_new_with_label("<");
    adelante = gtk_button_new_with_label(">");
    gtk_widget_set_name(atras, "carrusel_control");
    gtk_widget_set_name(adelante, "carrusel_control");
    
    // Llenar el Stack de Imágenes
    for (int i = 0; i < NUM_IMAGES; i++) {
        GtkWidget *image_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget *image_widget = gtk_image_new();
        
        GdkPixbuf *original_pixbuf = NULL;
        GdkPixbuf *scaled_pixbuf = NULL;
        GError *pixbuf_error = NULL;

        // Intentar cargar la imagen desde la ruta local
        original_pixbuf = gdk_pixbuf_new_from_file(IMAGE_PATHS[i], &pixbuf_error);
        if (pixbuf_error) {
            g_print("ERROR: No se pudo cargar la imagen %s: %s\n", IMAGE_PATHS[i], pixbuf_error->message);
            g_error_free(pixbuf_error);
            // Fallback a un label si no se encuentra la imagen
            GtkWidget *fallback = gtk_label_new(g_strdup_printf("IMAGEN %d NO ENCONTRADA", i + 1));
            gtk_container_add(GTK_CONTAINER(image_container), fallback);
        } else {
            scaled_pixbuf = gdk_pixbuf_scale_simple(original_pixbuf, TARGET_WIDTH, TARGET_HEIGHT, GDK_INTERP_BILINEAR);
            gtk_image_set_from_pixbuf(GTK_IMAGE(image_widget), scaled_pixbuf);
            gtk_container_add(GTK_CONTAINER(image_container), image_widget);
            g_object_unref(original_pixbuf);
            if (scaled_pixbuf) g_object_unref(scaled_pixbuf);
        }

        gtk_stack_add_titled(GTK_STACK(stackImagenes), image_container, IMAGE_NAMES[i], g_strdup_printf("%d", i + 1));
    }
    
    carousel_data->stack = GTK_STACK(stackImagenes);
    carousel_data->names = IMAGE_NAMES;
    carousel_data->count = NUM_IMAGES;

    g_signal_connect(atras, "clicked", G_CALLBACK(on_carrusel_button_clicked), carousel_data);
    g_signal_connect(adelante, "clicked", G_CALLBACK(on_carrusel_button_clicked), carousel_data);

    gtk_grid_attach(GTK_GRID(gridCarrusel), atras, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(gridCarrusel), stackImagenes, 1, 0, 1, 1); 
    gtk_grid_attach(GTK_GRID(gridCarrusel), adelante, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(gridCarrusel), switcherCarrusel, 1, 1, 1, 1);
    

    // ==========================================
    // 7. CONSTRUCCIÓN DE LA BARRA DE NAVEGACIÓN
    // ==========================================

    // --- Botón 0: PERFIL DE USUARIO (Icono) ---
    GtkWidget *btn_usuario = create_icon_nav_button(stack, "avatar-default", "page_user");
    gtk_box_pack_start(GTK_BOX(navbar_box), btn_usuario, FALSE, FALSE, 0);
    

    // --- Botón 1: NOVEDADES (Simple) ---
    GtkWidget *btn_novedades = create_nav_button(stack, "NOVEDADES", "page_novedades");
    gtk_box_pack_start(GTK_BOX(navbar_box), btn_novedades, FALSE, FALSE, 0);

    // --- Botón 2: MUJER (Menú Desplegable) ---
    const gchar *mujer_labels[] = {"Camisas y Blusas", "Faldas y Vestidos", "Accesorios"};
    const gchar *mujer_pages[] = {"page_mujer_camisas", "page_mujer_faldas", "page_mujer_accesorios"};
    GtkWidget *menu_button_mujer = create_menu_button(stack, "MUJER", 3, mujer_labels, mujer_pages);
    gtk_box_pack_start(GTK_BOX(navbar_box), menu_button_mujer, FALSE, FALSE, 0);

    // --- Botón 3: HOMBRE (Menú Desplegable) ---
    const gchar *hombre_labels[] = {"Pantalones", "Chaquetas y Abrigos", "Calzado"};
    const gchar *hombre_pages[] = {"page_hombre_pantalones", "page_hombre_chaquetas", "page_hombre_calzado"};
    GtkWidget *menu_button_hombre = create_menu_button(stack, "HOMBRE", 3, hombre_labels, hombre_pages);
    gtk_box_pack_start(GTK_BOX(navbar_box), menu_button_hombre, FALSE, FALSE, 0);

    // --- Botón 4: OFERTAS HOT (Simple) ---
    GtkWidget *btn_ofertas = create_nav_button(stack, "OFERTAS HOT", "page_ofertas");
    gtk_box_pack_start(GTK_BOX(navbar_box), btn_ofertas, FALSE, FALSE, 0);


    // ==========================================
    // 8. DEFINIR PÁGINAS FINALES DEL STACK PRINCIPAL
    // ==========================================

    // Botón usuario
    GtkWidget *page_user_content = create_content_label("PANEL DE CONTROL DEL USUARIO (MI CUENTA)", "");
    gtk_stack_add_named(GTK_STACK(stack), page_user_content, "page_user");

    // --- 1. NOVEDADES (El Carrusel) ---
    gtk_stack_add_named(GTK_STACK(stack), gridCarrusel, "page_novedades");
    
    // --- 2. MUJER Sub-páginas (Aplicando clase 'mujer') ---
    gtk_stack_add_named(GTK_STACK(stack), create_scrolleable_grid_prendas(listaActual, 3), "page_mujer_camisas");
    // gtk_stack_add_named(GTK_STACK(stack), create_grid_scrolleable(10,10), "page_mujer_faldas");
    // gtk_stack_add_named(GTK_STACK(stack), create_grid_scrolleable(10,10), "page_mujer_accesorios");

    // --- 3. HOMBRE Sub-páginas (Aplicando clase 'hombre') ---
    gtk_stack_add_named(GTK_STACK(stack), create_content_label("CATALOGO DE PANTALONES", "hombre"), "page_hombre_pantalones");
    gtk_stack_add_named(GTK_STACK(stack), create_content_label("CATALOGO DE CHAQUETAS Y ABRIGOS", "hombre"), "page_hombre_chaquetas");
    gtk_stack_add_named(GTK_STACK(stack), create_content_label("CATALOGO DE CALZADO DE HOMBRE", "hombre"), "page_hombre_calzado");


    // --- 4. OFERTAS HOT (Aplicando clase 'alerta') ---
    GtkWidget *page_ofertas = create_content_label("¡Grandes OFERTAS HOT!", "alerta");
    gtk_stack_add_named(GTK_STACK(stack), page_ofertas, "page_ofertas");


    gtk_widget_show_all(window);
    // Establecer NOVEDADES como la página inicial
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "page_novedades"); 
}

int
main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    // 1. Inicializar el sistema de archivos
    init_user_system();

    // 2. Crear el botón de "Login" o "Cuenta" en la Navbar
    // Suponiendo que tienes un botón en la esquina derecha
    GtkWidget *btn_cuenta = create_nav_button(stack, "LOGIN / CUENTA", "page_login");
    // Añade este botón a tu grid o caja de navegación

    // 3. Crear la página de Login y añadirla al Stack Principal
    // Le pasamos el 'btn_cuenta' para que cuando hagas login cambie el texto a "HOLA, USUARIO"
    GtkWidget *login_page = create_login_register_page(stack, btn_cuenta);

    // Añadimos la página al stack con el nombre "page_login"
    gtk_stack_add_named(GTK_STACK(stack), login_page, "page_login");

    //
    
    return status;
}


