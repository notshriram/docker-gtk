#include <gtk/gtk.h>
#include <cairo.h>
#include <cairo-svg.h> 

static double prev_x = -1;
static double prev_y = -1;
/* Surface to store current scribbles */
static cairo_surface_t* surface = NULL;

static void load_css(void) {
    GtkCssProvider* provider;
    GdkDisplay* display;
    GdkScreen* screen;

    const gchar* css_style_file = "style.css";
    GFile* file = g_file_new_for_path(css_style_file);
    GError* error = NULL;

    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_file(provider, file, &error);

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    }
    g_object_unref(provider);
}

static void
clear_surface(void)
{
    cairo_t* cr;

    cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_destroy(cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
configure_event_cb(GtkWidget* widget,
    GdkEventConfigure* event,
    gpointer           data)
{
    if (surface)
        cairo_surface_destroy(surface);

    surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);

    /* Initialize the surface to white */
    clear_surface();

    /* We've handled the configure event, no need for further processing. */
    return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb(GtkWidget* widget,
    cairo_t* cr,
    gpointer   data)
{
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    return FALSE;
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush(GtkWidget* widget,
    gdouble    x,
    gdouble    y)
{
    if ((prev_x == -1) || (prev_y == -1)) {
        prev_x = x;
        prev_y = y;
    }
    cairo_t* cr;
    //set line width to 5


    /* Paint to the surface, where we store our state */
    cr = cairo_create(surface);
    cairo_set_line_width(cr, 10);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, prev_x, prev_y);
    cairo_stroke(cr);
    cairo_destroy(cr);


    // invalidate the drawn line region
    gtk_widget_queue_draw_area(widget,
        MIN(x, prev_x),
        MIN(y, prev_y),
        ABS(x - prev_x),
        ABS(y - prev_y));


    prev_x = x;
    prev_y = y;
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean
button_press_event_cb(GtkWidget* widget,
    GdkEventButton* event,
    gpointer        data)
{
    /* paranoia check, in case we haven't gotten a configure event */
    if (surface == NULL)
        return FALSE;

    if (event->button == GDK_BUTTON_PRIMARY)
    {
        draw_brush(widget, event->x, event->y);
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
        clear_surface();
        gtk_widget_queue_draw(widget);
    }

    /* We've handled the event, stop processing */
    return TRUE;
}

/* Handle button release event by resetting the previous position */
static gboolean
button_release_event_cb(GtkWidget* widget,
    GdkEventButton* event,
    gpointer        data)
{
    prev_x = -1;
    prev_y = -1;

    return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean
motion_notify_event_cb(GtkWidget* widget,
    GdkEventMotion* event,
    gpointer        data)
{
    /* paranoia check, in case we haven't gotten a configure event */
    if (surface == NULL)
        return FALSE;

    if (event->state & GDK_BUTTON1_MASK)
        draw_brush(widget, event->x, event->y);

    /* We've handled it, stop processing */
    return TRUE;
}

static void save_svg(GtkWidget* widget, gpointer data) {
    GtkWidget* da = (GtkWidget*)data;

    auto width = gtk_widget_get_allocated_width(da);
    auto height = gtk_widget_get_allocated_height(da);

    cairo_surface_t* target;
    target = cairo_svg_surface_create("svgfile.svg", width, height);

    cairo_t* cr;
    cr = cairo_create(target);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_source_surface(cr, surface, 0.0, 0.0);
    //cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    // cairo_set_font_size(cr, 40.0);
    // cairo_move_to(cr, 10.0, 50.0);
    // cairo_show_text(cr, "Outline.");
    cairo_paint(cr);
    cairo_surface_destroy(target);
    cairo_destroy(cr);
}

static void
close_window(void)
{
    if (surface)
        cairo_surface_destroy(surface);
}

static void
activate(GtkApplication* app,
    gpointer        user_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* window;
    GtkWidget* frame;
    GtkWidget* drawing_area;
    GtkWidget* button;
    load_css();

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Drawing Area");

    g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

    gtk_container_set_border_width(GTK_CONTAINER(window), 8);

    gtk_container_add(GTK_CONTAINER(window), vbox);



    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    drawing_area = gtk_drawing_area_new();
    /* set a minimum size */
    gtk_widget_set_size_request(drawing_area, 800, 800);
    gtk_container_add(GTK_CONTAINER(frame), drawing_area);

    //add a button to save the svg file
    button = gtk_button_new_with_label("Save SVG");
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);



    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(save_svg), (gpointer)drawing_area);

    /* Signals used to handle the backing surface */
    g_signal_connect(drawing_area, "draw",
        G_CALLBACK(draw_cb), NULL);
    g_signal_connect(drawing_area, "configure-event",
        G_CALLBACK(configure_event_cb), NULL);

    /* Event signals */
    g_signal_connect(drawing_area, "motion-notify-event",
        G_CALLBACK(motion_notify_event_cb), NULL);
    g_signal_connect(drawing_area, "button-press-event",
        G_CALLBACK(button_press_event_cb), NULL);
    g_signal_connect(drawing_area, "button-release-event",
        G_CALLBACK(button_release_event_cb), NULL);
    /* Ask to receive events the drawing area doesn't normally
     * subscribe to. In particular, we need to ask for the
     * button press and motion notify events that want to handle.
     */
    gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area)
        | GDK_BUTTON_PRESS_MASK
        | GDK_POINTER_MOTION_MASK
        | GDK_BUTTON_RELEASE_MASK);

    gtk_widget_show_all(window);
}

int
main(int    argc,
    char** argv)
{
    GtkApplication* app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

//   g_signal_connect(G_OBJECT(w->da), "motion-notify-event", G_CALLBACK(nextCallback), (gpointer)w);
//   g_signal_connect(G_OBJECT(w->da), "button-press-event", G_CALLBACK(nextCallback), (gpointer)w);