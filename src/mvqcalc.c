/*
 *  Name: mvqcalc.c
 *  Description: graphical vector, matrix, quaternion calculator 
 *
 *  Copyright (C) 2012  Jason Hall
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * I am not terribly interested in adding a lot more features into this 
 * program, it fulfills my needs as is currently. However I thought it
 * would make a nice addition to the Open Source Destkop. So I am releasing
 * it publicly. It requires more work (such as internationalization) to make
 * it work well for everyone. I will gladly accept & apply well written
 * patches to the program and upload new releases.
 *
 * This is my first open source project, so take it easy on me. Also note
 * that I only check my email every few days.
 */

/* Roadmap To 1.0:
 *
 *  1. Verify Vector<>Vector Angle operation is producing the right answer
 *  2. Inverse / Negate operations
 *  3. Needs more & better explanations in the information window on the math.
 *  4. Something to help people understand the relations of how a number
 *     is affected by other numbers. (colorize boxes on hover?)
 *  5. Tablet style number input widget
 *  6. Internationalization & "Lingual" Translations :)
 *  7. Split into multiple files (this one is kind of large)
 *  8. Make it work correctly on lower resolution displays
 *  9. Matrix -> Quaternion conversion
 * 10. Wrap at 80 characters?
 * 11. Fix vec3_div macro names to allow both scalar & vector division
 * 12. Make sure errors are checked for everywhere they need to be
 * 13. Check for memory leaks & try to free up some more runtime memory
 * 14. Other platform support (FreeBSD, MacOS, Windows, etc)
 * 15. Check for integer & other overflows and other similar programming errors.
 * 16. Reduce number of exported symbols
 * 17. LERP, SLERP (anything else missing?) math operations
 * 18. Application Icon
 * 19. User Documentation
 * 20. 1.0 release
 *
 * POST 1.0:
 *  1. easy to use graphing capability (to visualize rotations over a series of numbers)
 *  2. ability to work with more than a, b & result (ability to create a sequence of ops)
 *  3. This program could probably be written more cleverly so that operations aren't
 *     replicated for each data type.
 */

/* fprintf() */
#include <stdio.h>

/* malloc()/free() */
#include <stdlib.h>

/* memset()/strncpy() */
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

/* stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "mathlib.h"

#include "../config.h"

char *glade_search_dirs[] = {
    "./",
    "share",
    "../share",
    "../../share",
    DATADIR,
    PACKAGE_SOURCE_DIR "/share"
};

typedef struct {
    GtkWidget *widget;
    GladeXML *xml;
} glade_widget_t;

// gtk composite widget types
typedef struct {
    glade_widget_t *gwidget;
    GtkWidget	*entries[16];
    mat4x4_t	m;
} gtk_matrix_t;

typedef struct {
    glade_widget_t *gwidget;
    GtkWidget	*entries[3];
    vec3_t	v;
} gtk_vector_t;

typedef struct {
    glade_widget_t *gwidget;
    GtkWidget	*entries[4];
    quat_t	q;
} gtk_quat_t;

typedef struct {
    glade_widget_t *gwidget;
    GtkWidget	*entry;
    real	s;
} gtk_scalar_t;

///////////////////////////////////////////////////////////////////////

// toolbar widgets

#define MATRIX_TOOLBAR_CONVERT_VECTOR 1
#define MATRIX_TOOLBAR_CONVERT_QUAT   2
typedef struct {
    glade_widget_t *gwidget;
    gtk_matrix_t *m;
    GtkWidget *button_id;
    GtkWidget *button_zero;
    GtkWidget *convert_select;
    GtkWidget *convert_button;
} gtk_matrix_toolbar_t;

typedef struct {
    glade_widget_t *gwidget;
    gtk_matrix_t *m;
    GtkWidget *button_copy_a;
    GtkWidget *button_copy_b;
} gtk_matrix_result_toolbar_t;

typedef struct {
    glade_widget_t *gwidget;
    gtk_vector_t *v;
    GtkWidget *button_zero;
    GtkWidget *button_unitize;
    GtkWidget *button_length;
} gtk_vector_toolbar_t;

typedef struct {
    glade_widget_t *gwidget;
    gtk_vector_t *v;
    GtkWidget *button_unitize;
    GtkWidget *button_copy_a;
    GtkWidget *button_copy_b;
} gtk_vector_result_toolbar_t;

#define QUAT_TOOLBAR_CONVERT_MATRIX 1
#define QUAT_TOOLBAR_CONVERT_X_VEC  2
#define QUAT_TOOLBAR_CONVERT_Y_VEC  3
#define QUAT_TOOLBAR_CONVERT_Z_VEC  4
typedef struct {
    glade_widget_t *gwidget;
    gtk_quat_t *q;
    GtkWidget *button_zero;
    GtkWidget *button_id;
    GtkWidget *button_unitize;
    GtkWidget *convert_select;
    GtkWidget *convert_button;
} gtk_quat_toolbar_t;

typedef struct {
    glade_widget_t *gwidget;
    gtk_quat_t *q;
    GtkWidget *button_unitize;
    GtkWidget *button_copy_a;
    GtkWidget *button_copy_b;
} gtk_quat_result_toolbar_t;

///////////////////////////////////////////////////////////////////////

#define MEMORY_BUTTONS 4

typedef struct gtk_recall_toolbar_s gtk_recall_toolbar_t;
typedef struct gtk_memory_toolbar_s gtk_memory_toolbar_t;

typedef struct memory_store {
    GSList *memory_toolbars;
    GSList *recall_toolbars;
    void *data[MEMORY_BUTTONS];
    gboolean active[MEMORY_BUTTONS];
    void *(*store)(gtk_memory_toolbar_t *toolbar);
    void (*recall)(gtk_recall_toolbar_t *toolbar, void *data);
} memory_store_t;

typedef struct {
    int slot;
    void *toolbar;
    memory_store_t *store;
} memory_event_t;

struct gtk_memory_toolbar_s {
    glade_widget_t *gwidget;
    void *cwidget; // composite widget (eg. gtk_matrix_t, gtk_quat_t, etc.)
    memory_store_t *store;
    GtkWidget *buttons[MEMORY_BUTTONS];
};

struct gtk_recall_toolbar_s {
    glade_widget_t *gwidget;
    void *cwidget; // composite widget (eg. gtk_matrix_t, gtk_quat_t, etc.)
    memory_store_t *store;
    GtkWidget *buttons[MEMORY_BUTTONS];
};

///////////////////////////////////////////////////////////////////////

// composite widgets
gtk_matrix_t *matrix1, *matrix2, *matrix3;
gtk_vector_t *vector1, *vector2, *vector3;
gtk_quat_t *quat1, *quat2, *quat3;
gtk_scalar_t *scalar1, *scalar3;

// toolbar widgets
gtk_matrix_toolbar_t *matrix_toolbar1, *matrix_toolbar2;
gtk_matrix_result_toolbar_t *matrix_result_toolbar;
gtk_quat_toolbar_t *quat_toolbar1, *quat_toolbar2;
gtk_quat_result_toolbar_t *quat_result_toolbar;
gtk_vector_toolbar_t *vector_toolbar1, *vector_toolbar2;
gtk_vector_result_toolbar_t *vector_result_toolbar;

// memory stores
memory_store_t *matrix_memory, *quat_memory, *vector_memory;

// misc gtk
GtkWidget *window; // application window
GtkWidget *text_view;
GtkTextTag  *red;
GtkTextTag  *cyan;

// operation buttons
GtkWidget *button_add;
GtkWidget *button_subtract;
GtkWidget *button_multiply;
GtkWidget *button_divide;
GtkWidget *button_cross;
GtkWidget *button_angle;
GtkWidget *button_transform;
GtkWidget *button_scale;
GtkWidget *button_translate;
GtkWidget *button_rotate;
GtkWidget *button_swap;
GtkWidget *button_dot;

// notebooks
#define NOTEBOOK_PAGE_MATRIX 0
#define NOTEBOOK_PAGE_VECTOR 1
#define NOTEBOOK_PAGE_QUAT   2
#define NOTEBOOK_PAGE_SCALAR 3
GtkWidget *notebook_a;
GtkWidget *notebook_b;
GtkWidget *notebook_c;

// math modes
#define MODE_MM		1
#define MODE_MV		2
#define MODE_VV		3
#define MODE_QQ		4
#define MODE_SV		5
#define MODE_VM		6
#define MODE_SM		7
#define MODE_VQ		8

int MathMode = 0;

void
format_number(real n, char *buf, int len)
{
    // use %g so whole numbers are displayed as a whole number
    snprintf(buf, len, "%g", n);
    if (strchr(buf, 'e') != NULL) {
	// exponent notation detected, use alternate format.
	// exponents appear on the right side of the text
	// entry boxes and can be missed by users, making them
	// think the number is smaller or larger, so use the
	// %f number format instead for these numbers
	snprintf(buf, len, "%f", n);
    }
} 

///////////////////////////////////////////////////////////////////////
/////////////////////  TEXTAREA RELATED FUNCTIONS  ////////////////////
///////////////////////////////////////////////////////////////////////

/* overload printf to print to our text area instead of stdout */
#define printf _printf
void
_printf(char *fmt, ...)
{
    /* Guess we need no more than 100 bytes. */
    int n, size = 100;
    char *p;
    va_list ap;

    if ((p = malloc (size)) == NULL)
        return;

    for (;;) {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf (p, size, fmt, ap);
        va_end(ap);

        /* If that worked, return the string. */
        if (n > -1 && n < size)
	    break;

        /* Else try again with more space. */
        if (n > -1)             // glibc 2.1
            size = n+1;         // precisely what is needed
        else                    // glibc 2.0
            size *= 2;          // twice the old size

        if ((p = realloc(p, size)) == NULL)
            return;
    }

    GtkTextIter iter;
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_end_iter(text_buffer, &iter);
    gtk_text_buffer_insert_with_tags(text_buffer, &iter, p, -1, cyan, NULL);
    gtk_text_iter_forward_to_end(&iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &iter, 0.49, TRUE, 0, 0);

    free(p);
}

void
scrolledwindow1_size_allocate_cb(GtkWidget *widget, gpointer user_data)
{
    GtkTextIter iter;
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_end_iter(text_buffer, &iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &iter, 0.49, TRUE, 0, 0);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////  CUT & PASTE  /////////////////////////////
///////////////////////////////////////////////////////////////////////

void
menu_cut_cb(GtkWidget *w, void *data)
{
    GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(window));
    if (GTK_IS_ENTRY(focused) || GTK_IS_TEXT_VIEW(focused))
	g_signal_emit_by_name(focused, "cut-clipboard", NULL);
}

void
menu_copy_cb(GtkWidget *w, void *data)
{
    GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(window));
    if (GTK_IS_ENTRY(focused) || GTK_IS_TEXT_VIEW(focused))
	g_signal_emit_by_name(focused, "copy-clipboard", NULL);
}

void
menu_paste_cb(GtkWidget *w, void *data)
{
    GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(window));
    if (GTK_IS_ENTRY(focused) || GTK_IS_TEXT_VIEW(focused))
	g_signal_emit_by_name(focused, "paste-clipboard", NULL);
}

///////////////////////////////////////////////////////////////////////
//////////////////////////  GLADE WIDGET  /////////////////////////////
///////////////////////////////////////////////////////////////////////

// file: filename to find
// path: buffer to write pathname to
// pathlen: length of path buffer
gboolean
find_data_file(char *file, char *path, int pathlen)
{
    struct stat st;
    int ret;
    char buf[BUFSIZ];
    int i;

    for (i = 0; i < sizeof(glade_search_dirs)/sizeof(glade_search_dirs[0]); i++) {
	sprintf(buf, "%s/%s", glade_search_dirs[i], file);

	if ((ret = stat(buf, &st)) < 0)
	    continue;

	strncpy(path, buf, pathlen);
	return TRUE;
    }

    return FALSE;
}

// file: filename of glade file
// window: name of the "window" object in the glade file
// container: name of container which we detach from the child
// parent: parent gtk widget we attach the child to
glade_widget_t *
glade_widget_new(char *file, char *window, char *container, GtkWidget *parent)
{
    glade_widget_t *gw;
    GtkWidget *old_window;
    char buf[BUFSIZ];

    // create object
    gw = malloc(sizeof(glade_widget_t));
    memset(gw, 0, sizeof(glade_widget_t));

    // load glade xml file
    if (find_data_file(file, buf, BUFSIZ) != TRUE) {
	fprintf(stderr, "ERROR: unable to locate glade file '%s'\n", file);
	exit(1);
    }

    gw->xml = glade_xml_new(buf, NULL, NULL);
    if (gw->xml == NULL) {
	fprintf(stderr, "ERROR: unable to load glade file '%s'\n", file);
	exit(1);
    }

    // find the window widget so we can remove the
    // elements from it
    old_window = glade_xml_get_widget(gw->xml, window);
    g_assert(old_window != NULL);

    // locate frame widget which contains the matrix elements
    gw->widget = glade_xml_get_widget(gw->xml, container);
    g_assert(gw->widget != NULL);

    // NOTE: add a reference to the widget so it isn't 
    //       automatically unallocated when we remove it
    //       from the window
    gtk_widget_ref(gw->widget);

    // remove widget from window
    gtk_container_remove(GTK_CONTAINER(old_window), gw->widget);

    // add widget to the new container
    gtk_container_add(GTK_CONTAINER(parent), gw->widget);
    gtk_widget_unref(gw->widget);

    return gw;
}

///////////////////////////////////////////////////////////////////////
////////////////////////  MATRIX WIDGET  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
matrix_widget_get(gtk_matrix_t *matrix, mat4x4_t m)
{
    const char *buf;
    int i;
    GtkWidget *w;

    for (i = 0; i < 16; i++) {
	w = matrix->entries[i];
	buf = gtk_entry_get_text(GTK_ENTRY(w));
	m[i] = atof(buf);
    }
}

void
matrix_widget_set(gtk_matrix_t *matrix, mat4x4_t m)
{
    char buf[BUFSIZ];
    int i;
    GtkWidget *w;

    for (i = 0; i < 16; i++) {
	w = matrix->entries[i];
//	sprintf(buf, NumberFormat, m[i]);
	format_number(m[i], buf, sizeof(buf));
	gtk_entry_set_text(GTK_ENTRY(w), buf);
    }
}

gtk_matrix_t *
matrix_widget_new(GtkWidget *parent)
{
    gtk_matrix_t *m;
    int i;
    char buf[BUFSIZ];

    // create object
    m = malloc(sizeof(gtk_matrix_t));
    memset(m, 0, sizeof(gtk_matrix_t));

    m->gwidget = glade_widget_new("matrix_widget.glade", "window1", "matrix_frame", parent);

    // locate all of the text entry widget
    for (i = 1; i <= 16; i++) {
	sprintf(buf, "entry_%d", i);
	m->entries[i-1] = glade_xml_get_widget(m->gwidget->xml, buf);
	if (m->entries[i-1] == NULL) {
	    fprintf(stderr, "could not locate text entry '%s'\n", buf);
	    exit(1);
	}
    }

    return m;
}

///////////////////////////////////////////////////////////////////////
///////////////////////  QUATERNION WIDGET  ///////////////////////////
///////////////////////////////////////////////////////////////////////

gtk_quat_t *
quat_widget_new(GtkWidget *parent)
{
    gtk_quat_t *q;
    char buf[BUFSIZ];
    int i;

    // create object
    q = malloc(sizeof(gtk_quat_t));
    memset(q, 0, sizeof(gtk_quat_t));

    q->gwidget = glade_widget_new("quaternion_widget.glade", "window1", "quat_frame", parent);

    // locate all of the text entry widget
    for (i = 1; i <= 4; i++) {
	sprintf(buf, "entry_%d", i);
	q->entries[i-1] = glade_xml_get_widget(q->gwidget->xml, buf);
	if (q->entries[i-1] == NULL) {
	        fprintf(stderr, "could not locate text entry '%s'\n", buf);
		    exit(1);
	}
    }

    return q;
}

void
quat_widget_get(gtk_quat_t *quat, quat_t q)
{
    const char *buf;
    int i;
    GtkWidget *w;

    for (i = 0; i < 4; i++) {
	w = quat->entries[i];
	buf = gtk_entry_get_text(GTK_ENTRY(w));
	q[i] = atof(buf);
    }
}

void
quat_widget_set(gtk_quat_t *quat, quat_t q)
{
    char buf[BUFSIZ];
    int i;
    GtkWidget *w;

    for (i = 0; i < 4; i++) {
	w = quat->entries[i];
//	sprintf(buf, NumberFormat, q[i]);
	format_number(q[i], buf, sizeof(buf));
	gtk_entry_set_text(GTK_ENTRY(w), buf);
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////   VECTOR WIDGET   /////////////////////////////
///////////////////////////////////////////////////////////////////////

gtk_vector_t *
vector_widget_new(GtkWidget *parent)
{
    gtk_vector_t *v;
    char buf[BUFSIZ];
    int i;

    // create object
    v = malloc(sizeof(gtk_vector_t));
    memset(v, 0, sizeof(gtk_vector_t));

    v->gwidget = glade_widget_new("vector_widget.glade", "window1", "vector_frame", parent);

    // locate all of the text entry widget
    for (i = 1; i <= 3; i++) {
	sprintf(buf, "entry_%d", i);
	v->entries[i-1] = glade_xml_get_widget(v->gwidget->xml, buf);
	if (v->entries[i-1] == NULL) {
	    fprintf(stderr, "could not locate text entry '%s'\n", buf);
	    exit(1);
	}
    }

    return v;
}

void
vector_widget_get(gtk_vector_t *vector, vec3_t v)
{
    const char *buf;
    int i;
    GtkWidget *w;

    for (i = 0; i < 3; i++) {
	w = vector->entries[i];
	buf = gtk_entry_get_text(GTK_ENTRY(w));
	v[i] = atof(buf);
    }
}

void
vector_widget_set(gtk_vector_t *vector, vec3_t v)
{
    char buf[BUFSIZ];
    int i;
    GtkWidget *w;

    for (i = 0; i < 3; i++) {
	w = vector->entries[i];
	//sprintf(buf, NumberFormat, v[i]);
	format_number(v[i], buf, sizeof(buf));
	gtk_entry_set_text(GTK_ENTRY(w), buf);
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////   SCALAR WIDGET   /////////////////////////////
///////////////////////////////////////////////////////////////////////

gtk_scalar_t *
scalar_widget_new(GtkWidget *parent)
{
    gtk_scalar_t *s;
    char buf[BUFSIZ];

    // create object
    s = malloc(sizeof(gtk_scalar_t));
    memset(s, 0, sizeof(gtk_scalar_t));

    // load glade xml file
    s->gwidget = glade_widget_new("scalar_widget.glade", "window1", "scalar_frame", parent);

    // locate all of the text entry widget
    s->entry = glade_xml_get_widget(s->gwidget->xml, "entry_1");
    if (s->entry == NULL) {
	fprintf(stderr, "could not locate text entry '%s'\n", buf);
	exit(1);
    }

    return s;
}

void
scalar_widget_get(gtk_scalar_t *scalar, real *s)
{
    const char *buf;
    GtkWidget *w;

    w = scalar->entry;
    buf = gtk_entry_get_text(GTK_ENTRY(w));
    *s = atof(buf);
}

void
scalar_widget_set(gtk_scalar_t *scalar, real s)
{
    char buf[BUFSIZ];
    GtkWidget *w;

    w = scalar->entry;
//    sprintf(buf, NumberFormat, s);
    format_number(s, buf, sizeof(buf));
    gtk_entry_set_text(GTK_ENTRY(w), buf);
}

///////////////////////////////////////////////////////////////////////
/////////////////////////  MEMORY STORE  //////////////////////////////
///////////////////////////////////////////////////////////////////////

memory_store_t *
memory_store_new(
	void *(*store)(gtk_memory_toolbar_t *toolbar),
	void (*recall)(gtk_recall_toolbar_t *toolbar, void *data))
{
    memory_store_t *memory_store;
    memory_store = malloc(sizeof(memory_store_t));
    memset(memory_store, 0, sizeof(memory_store_t));

    memory_store->store  = store;
    memory_store->recall = recall;

    return memory_store;
}

void
memory_store_update_recall_buttons(memory_store_t *store)
{
    int i;
    GSList *item;

    for (item = store->recall_toolbars; item; item = item->next) {
	gtk_recall_toolbar_t *toolbar = item->data;

	if (toolbar == NULL)
	    continue;

     	for (i = 0; i < MEMORY_BUTTONS; i++) {
	    if (toolbar->buttons[i] == NULL)
		continue;

	    gtk_widget_set_sensitive(toolbar->buttons[i], store->active[i]);
	}
    }
}

///////////////////////////////////////////////////////////////////////
/////////////////////////  MEMORY TOOLBAR  ////////////////////////////
///////////////////////////////////////////////////////////////////////

void *
vector_memory_toolbar_store(gtk_memory_toolbar_t *toolbar)
{
    gtk_vector_t *vector = toolbar->cwidget;
    g_assert(vector != NULL);
    vec3_t *v;

    v = vec3_new(0,0,0);

    printf(">>> vector store\n");
    vector_widget_get(vector, *v);

    return v;
}

void *
quat_memory_toolbar_store(gtk_memory_toolbar_t *toolbar)
{
    gtk_quat_t *quat = toolbar->cwidget;
    g_assert(quat != NULL);
    quat_t *q;

    q = quat_new(0,0,0,0);

    printf(">>> quat store\n");
    quat_widget_get(quat, *q);

    return q;
}

void *
matrix_memory_toolbar_store(gtk_memory_toolbar_t *toolbar)
{
    gtk_matrix_t *matrix = toolbar->cwidget;
    g_assert(matrix != NULL);
    mat4x4_t *m;

    m = mat4x4_new();

    printf(">>> matrix store\n");
    matrix_widget_get(matrix, *m);

    return m;
}

void
gtk_memory_toolbar_button_clicked_cb(GtkWidget *widget, void *data)
{
    memory_event_t *ev = data;
    g_assert(ev != NULL);

    ev->store->data[ev->slot-1]   = ev->store->store(ev->toolbar);
    ev->store->active[ev->slot-1] = TRUE;

    memory_store_update_recall_buttons(ev->store);
}

gtk_memory_toolbar_t *
gtk_memory_toolbar_new(memory_store_t *store, GtkWidget *container, void *cwidget)
{
    gtk_memory_toolbar_t *gst;
    memory_event_t *ev;
    char buf[BUFSIZ];
    int i;

    gst = malloc(sizeof(gtk_memory_toolbar_t));
    memset(gst, 0, sizeof(gtk_memory_toolbar_t));

    gst->gwidget = glade_widget_new("memory_toolbar.glade", "window1", "memory_toolbar_box", container);
    gst->store   = store;
    gst->cwidget = cwidget;

    // add this toolbar to the list in store
    store->memory_toolbars = g_slist_append(store->memory_toolbars, gst); 

    // obtain button widgets, create event data & connect signal handlers
    for (i = 0; i < MEMORY_BUTTONS; i++) {
	sprintf(buf, "button_m%d", i + 1);
	gst->buttons[i] = glade_xml_get_widget(gst->gwidget->xml, buf);
	g_assert(gst->buttons[i] != NULL);

	ev = malloc(sizeof(memory_event_t));
	memset(ev, 0, sizeof(memory_event_t));
	ev->slot = i + 1;
	ev->toolbar = gst;
	ev->store = gst->store;
	gtk_signal_connect(GTK_OBJECT(gst->buttons[i]), "clicked",
		GTK_SIGNAL_FUNC(gtk_memory_toolbar_button_clicked_cb), ev);
    }

    memory_store_update_recall_buttons(gst->store);

    return gst;
}

///////////////////////////////////////////////////////////////////////
/////////////////////////  RECALL TOOLBAR  ////////////////////////////
///////////////////////////////////////////////////////////////////////

void
vector_memory_toolbar_recall(gtk_recall_toolbar_t *toolbar, void *data)
{
    gtk_vector_t *vector = toolbar->cwidget;
    g_assert(vector != NULL);
    vec3_t *v = data;
    g_assert(v != NULL);

    printf(">>> vector recall\n");
    vector_widget_set(vector, *v);
}

void
quat_memory_toolbar_recall(gtk_recall_toolbar_t *toolbar, void *data)
{
    gtk_quat_t *quat = toolbar->cwidget;
    g_assert(quat != NULL);
    quat_t *q = data;
    g_assert(q != NULL);

    printf(">>> quat recall\n");
    quat_widget_set(quat, *q);
}

void
matrix_memory_toolbar_recall(gtk_recall_toolbar_t *toolbar, void *data)
{
    gtk_matrix_t *matrix = toolbar->cwidget;
    g_assert(matrix != NULL);
    mat4x4_t *m = data;
    g_assert(m != NULL);

    printf(">>> matrix recall\n");
    matrix_widget_set(matrix, *m);
}

void
gtk_recall_toolbar_button_clicked_cb(GtkWidget *widget, void *data)
{
    memory_event_t *ev = data;
    g_assert(ev != NULL);

    ev->store->recall(ev->toolbar, ev->store->data[ev->slot-1]);
}

gtk_recall_toolbar_t *
gtk_recall_toolbar_new(memory_store_t *store, GtkWidget *container, void *cwidget)
{
    memory_event_t *ev;
    gtk_recall_toolbar_t *grt;
    char buf[BUFSIZ];
    int i;

    grt = malloc(sizeof(gtk_recall_toolbar_t));
    memset(grt, 0, sizeof(gtk_recall_toolbar_t));

    grt->gwidget = glade_widget_new("recall_toolbar.glade", "window1", "recall_toolbar_box", container);
    grt->store   = store;
    grt->cwidget = cwidget;

    // add this toolbar to the list in store
    store->recall_toolbars = g_slist_append(store->recall_toolbars, grt); 

    // obtain button widgets, create event data & connect signal handlers
    for (i = 0; i < MEMORY_BUTTONS; i++) {
	sprintf(buf, "button_r%d", i + 1);
	grt->buttons[i] = glade_xml_get_widget(grt->gwidget->xml, buf);
	g_assert(grt->buttons[i] != NULL);

	ev = malloc(sizeof(memory_event_t));
	memset(ev, 0, sizeof(memory_event_t));
	ev->slot = i + 1;
	ev->toolbar = grt;
	ev->store = grt->store;
	gtk_signal_connect(GTK_OBJECT(grt->buttons[i]), "clicked",
		GTK_SIGNAL_FUNC(gtk_recall_toolbar_button_clicked_cb), ev);
    }

    memory_store_update_recall_buttons(grt->store);

    return grt;
}

///////////////////////////////////////////////////////////////////////
////////////////////////  RESET BUTTONS  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
reset(void)
{
    mat4x4_t m_id;
    vec3_t v_id;
    quat_t q_id;
    real s_id;
    int i;

    mat4x4_id(m_id);
    vec3_zero(v_id);
    quat_id(q_id);
    s_id = 0.0;

    matrix_widget_set(matrix1, m_id);
    matrix_widget_set(matrix2, m_id);
    matrix_widget_set(matrix3, m_id);

    vector_widget_set(vector1, v_id);
    vector_widget_set(vector2, v_id);
    vector_widget_set(vector3, v_id);

    quat_widget_set(quat1, q_id);
    quat_widget_set(quat2, q_id);
    quat_widget_set(quat3, q_id);

    scalar_widget_set(scalar1, s_id);

    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
    gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
    gtk_text_buffer_delete(text_buffer, &start_iter, &end_iter);


    for (i = 0; i < MEMORY_BUTTONS; i++) {
	matrix_memory->active[i] = FALSE;
	if (matrix_memory->data[i] != NULL) {
	    free(matrix_memory->data[i]);
	    matrix_memory->data[i] = NULL;
	}
	vector_memory->active[i] = FALSE;
	if (vector_memory->data[i] != NULL) {
	    free(vector_memory->data[i]);
	    vector_memory->data[i] = NULL;
	}
    	quat_memory->active[i] = FALSE;
	if (quat_memory->data[i] != NULL) {
	    free(quat_memory->data[i]);
	    quat_memory->data[i] = NULL;
	}
    }
    memory_store_update_recall_buttons(matrix_memory);
    memory_store_update_recall_buttons(vector_memory);
    memory_store_update_recall_buttons(quat_memory);

    gtk_combo_box_set_active(GTK_COMBO_BOX(matrix_toolbar1->convert_select), -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(matrix_toolbar2->convert_select), -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(quat_toolbar1->convert_select), -1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(quat_toolbar2->convert_select), -1);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_MATRIX);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_MATRIX);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
}

void
button_reset_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    reset();
}

///////////////////////////////////////////////////////////////////////
//////////////////////////  ADD BUTTON  ///////////////////////////////
///////////////////////////////////////////////////////////////////////

void
add_vector_vector_vector(void)
{
    vec3_t a, b, c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    vec3_add(a, b, c);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
add_quat_quat_quat(void)
{
    quat_t a, b, c;

    quat_widget_get(quat1, a);
    quat_widget_get(quat2, b);

    quat_add(a, b, c);

    quat_widget_set(quat3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_QUAT);
}

void
button_add_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VV:
	    add_vector_vector_vector();
	    break;
	case MODE_QQ:
	    add_quat_quat_quat();
	    break;
	default:
	    printf("ERROR: add mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
//////////////////////  SUBTRACT BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
subtract_vector_vector_vector(void)
{
    vec3_t a, b, c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    vec3_sub(a, b, c);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
subtract_quat_quat_quat(void)
{
    quat_t a, b, c;

    quat_widget_get(quat1, a);
    quat_widget_get(quat2, b);

    quat_sub_quick(a, b, c);

    quat_widget_set(quat3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_QUAT);
}

void
button_subtract_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VV:
	    subtract_vector_vector_vector();
	    break;
	case MODE_QQ:
	    subtract_quat_quat_quat();
	    break;
	default:
	    printf("ERROR: subtract mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////  DOT BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
dot_vector_vector_scalar(void)
{
    vec3_t a, b;
    real c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    printf(">>> obtaining dot product of vector A and vector B\n");

    c = vec3_dot(a, b);

    scalar_widget_set(scalar3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_SCALAR);
}

void
button_dot_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VV:
	    dot_vector_vector_scalar();
	    break;
	default:
	    printf("ERROR: dot mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
/////////////////////////  CROSS BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
cross_vector_vector_vector(void)
{
    vec3_t a, b, c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    printf(">>> Performing cross product operation\n");
    printf("X = (AY*BZ) - (AZ*BY)\n");
    printf("X = (%g*%g) - (%g*%g)\n", a[Y], b[Z], a[Z], b[Y]);
    printf("Y = (AZ*BX) - (AX*BZ)\n");
    printf("Y = (%g*%g) - (%g*%g)\n", a[Z], b[X], a[X], b[Z]);
    printf("Z = (AX*BY) - (AY*BX)\n");
    printf("Z = (%g*%g) - (%g*%g)\n", a[X], b[Y], a[Y], b[X]);

    vec3_cross(a, b, c);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
button_cross_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VV:
	    cross_vector_vector_vector();
	    break;
	default:
	    printf("ERROR: cross mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
/////////////////////////  ANGLE BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
angle_vector_vector_scalar(void)
{
    vec3_t a, b;
    real c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    printf("WARNING: answer may not be correct, verification required\n");
    printf(">>> calculating angle between vector A & vector B (units in radians)\n");

    vec3_angle(a, b, &c);

    scalar_widget_set(scalar3, c);

    printf("C = %g degrees\n", c * RAD2DEG);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_SCALAR);
}

void
button_angle_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VV:
	    angle_vector_vector_scalar();
	    break;
	default:
	    printf("ERROR: angle mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////  MULTIPLY BUTTON  /////////////////////////////
///////////////////////////////////////////////////////////////////////

void
multiply_matrix_matrix_matrix(void)
{
    mat4x4_t a, b, c;
    int i;

    printf(">>> multiplying matrix a & matrix b\n");

    matrix_widget_get(matrix1, a);
    matrix_widget_get(matrix2, b);

//    mat4x4_mult(a, b, c);

#define M1(row,col)  a[(col<<2)+row]
#define M2(row,col)  b[(col<<2)+row]
#define M3(row,col)  c[(col<<2)+row]

    for (i = 0; i < 4; i++) {
        const real ai0=M1(i,0),  ai1=M1(i,1),  ai2=M1(i,2),  ai3=M1(i,3);

        M3(i,0) = ai0 * M2(0,0) + ai1 * M2(1,0) + ai2 * M2(2,0) + ai3 * M2(3,0);
	printf("C%d0 = (A%d0 * B00) + (A%d1 * B10) + (A%d2 * B20) + (A%d3 * B30)\n",
		i, i, i, i, i);
	printf("C%d0 = (%g * %g) + (%g * %g) + (%g * %g) + (%g * %g)\n",
		i,
		ai0, M2(0,0),
		ai1, M2(1,0),
		ai2, M2(2,0),
		ai3, M2(3,0));


        M3(i,1) = ai0 * M2(0,1) + ai1 * M2(1,1) + ai2 * M2(2,1) + ai3 * M2(3,1);
        printf("C%d1 = (A%d0 * B01) + (A%d1 * B11) + (A%d2 * B21) + (A%d3 * B31)\n",
                i, i, i, i, i);
	printf("C%d1 = (%g * %g) + (%g * %g) + (%g * %g) + (%g * %g)\n",
		i, ai0, M2(0,1), ai1, M2(1,1), ai2, M2(2,1), ai3, M2(3,1));

        M3(i,2) = ai0 * M2(0,2) + ai1 * M2(1,2) + ai2 * M2(2,2) + ai3 * M2(3,2);
        printf("C%d2 = (A%d0 * B02) + (A%d1 * B12) + (A%d2 * B22) + (A%d3 * B32)\n",
                i, i, i, i, i);
	printf("C%d2 = (%g * %g) + (%g * %g) + (%g * %g) + (%g * %g)\n",
		i, ai0, M2(0,2), ai1, M2(1,2), ai2, M2(2,2), ai3, M2(3,2));

        M3(i,3) = ai0 * M2(0,3) + ai1 * M2(1,3) + ai2 * M2(2,3) + ai3 * M2(3,3);
        printf("C%d3 = (A%d0 * B03) + (A%d1 * B13) + (A%d2 * B23) + (A%d3 * B33)\n",
                i, i, i, i, i);
	printf("C%d3 = (%g * %g) + (%g * %g) + (%g * %g) + (%g * %g)\n",
		i, ai0, M2(0,3), ai1, M2(1,3), ai2, M2(2,3), ai3, M2(3,3));
   }

#undef M1
#undef M2
#undef M3

    matrix_widget_set(matrix3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
}

// same as scale_scalar_vector_vector
void
multiply_scalar_vector_vector(void)
{
    real a;
    vec3_t b, c;

    printf(">>> Multiplying vector by scalar\n");

    scalar_widget_get(scalar1, &a);
    vector_widget_get(vector2, b);

    vec3_cp(b, c);
    vec3_scale(c, a);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
multiply_vector_vector_vector(void)
{
    vec3_t a, b, c;

    printf(">>> Multiplying vector by vector\n");

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);
    vec3_mult(a, b, c);
    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
multiply_quat_quat_quat(void)
{
    quat_t a, b, c;


    quat_widget_get(quat1, a);
    quat_widget_get(quat2, b);

    quat_cp(a, c);

    printf(">>> Multiplying quaternion by quaternion\n");
    // q1[X] = q2[W] * tmp[X] + q2[X] * tmp[W] + q2[Y] * tmp[Z] - q2[Z] * tmp[Y];
    printf("X = (AX * BW) + (AW * BX) + (AZ * BY) - (AY * BZ)\n");
    printf("X = (%g * %g) + (%g * %g) + (%g * %g) - (%g * %g)\n",
	    a[X], b[W], a[W], b[X], a[Z], b[Y], a[Y], b[Z]);
    // q1[Y] = q2[W] * tmp[Y] - q2[X] * tmp[Z] + q2[Y] * tmp[W] + q2[Z] * tmp[X];
    printf("Y = (AY * BW) - (AZ * BX) + (AW * BY) + (AX * BZ)\n");
    printf("Y = (%g * %g) - (%g * %g) + (%g * %g) + (%g * %g)\n",
	    a[Y], b[W], a[Z], b[X], a[W], b[Y], a[X], b[Z]);
    // q1[Z] = q2[W] * tmp[Z] + q2[X] * tmp[Y] - q2[Y] * tmp[X] + q2[Z] * tmp[W];
    printf("Z = (AZ * BW) + (AY * BX) - (AX * BY) + (AW * BZ)\n");
    printf("Z = (%g * %g) + (%g * %g) - (%g * %g) + (%g * %g)\n",
	    a[Z], b[W], a[Y], b[X], a[X], b[Y], a[W], b[Z]);
    // q1[W] = q2[W] * tmp[W] - q2[X] * tmp[X] - q2[Y] * tmp[Y] - q2[Z] * tmp[Z];
    printf("W = (AW * BW) - (AX * BX) - (AY * BY) - (AZ * BZ)\n");
    printf("W = (%g * %g) - (%g * %g) - (%g * %g) - (%g * %g)\n",
	    a[W], b[W], a[X], b[X], a[Y], b[Y], a[Z], b[Z]);

    quat_mult(c, b);

    quat_widget_set(quat3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_QUAT);
}

void
button_multiply_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_MM:
	    multiply_matrix_matrix_matrix();
	    break;
	case MODE_VV:
	    multiply_vector_vector_vector();
	    break;
	case MODE_SV:
	    multiply_scalar_vector_vector();
	    break;
	case MODE_QQ:
	    multiply_quat_quat_quat();
	    break;
	default:
	    printf("ERROR: multiply mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
////////////////////////  DIVIDE BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
divide_vector_vector_vector(void)
{
    vec3_t a, b, c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    printf(">>> dividing vector A by vector B\n");

    c[X] = a[X] / b[X];
    c[Y] = a[Y] / b[Y];
    c[Z] = a[Z] / b[Z];

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

/*
 * This is sort of backwards...
 */
void
divide_scalar_vector_vector(void)
{
    real a;
    vec3_t b, c;

    scalar_widget_get(scalar1, &a);
    vector_widget_get(vector2, b);

    printf(">>> dividing vector B by scalar A\n");

    vec3_cp(b, c);
    vec3_div(c, a);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
button_divide_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VV:
	    divide_vector_vector_vector();
	    break;
	case MODE_SV:
	    divide_scalar_vector_vector();
	    break;
	default:
	    printf("ERROR: angle mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
/////////////////////  TRANSLATE BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
translate_vector_matrix_matrix(void)
{
    vec3_t a;
    mat4x4_t b, c;

    printf(">>> translating matrix by vector\n");

    vector_widget_get(vector1, a);
    matrix_widget_get(matrix2, b);

    mat4x4_copy(b, c);
    mat4x4_translate(c, a[X], a[Y], a[Z]);

    matrix_widget_set(matrix3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
}

void
button_translate_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VM:
	    translate_vector_matrix_matrix();
	    break;
	default:
	    printf("ERROR: translate mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
/////////////////////////  SCALE BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
scale_scalar_matrix_matrix(void)
{
    real a;
    mat4x4_t b, c;

    printf(">>> scaling matrix by scalar\n");

    scalar_widget_get(scalar1, &a);
    matrix_widget_get(matrix2, b);

    mat4x4_copy(b, c);
    mat4x4_scale(c, a);

    matrix_widget_set(matrix3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
}

// same as multiply_scalar_vector_vector
void
scale_scalar_vector_vector(void)
{
    real a;
    vec3_t b, c;

    printf(">>> scaling vector by scalar\n");

    scalar_widget_get(scalar1, &a);
    vector_widget_get(vector2, b);

    vec3_cp(b, c);
    vec3_scale(c, a);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
button_scale_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_SM:
	    scale_scalar_matrix_matrix();
	    break;
	case MODE_SV:
	    scale_scalar_vector_vector();
	    break;
	default:
	    printf("ERROR: scale mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
////////////////////////  ROTATE BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
rotate_vector_matrix_matrix(void)
{
    vec3_t a;
    mat4x4_t b, c, tmp;
    real angle;

    printf(">>> rotating matrix B by vector A\n");
    printf(">>> NOTE: rotating in parts: value X on the X axis, then Y on the Y axis, then Z on the Z axis\n");
    printf(">>> using degrees for input and placing radians in the matrix\n");

    vector_widget_get(vector1, a);
    matrix_widget_get(matrix2, b);

    mat4x4_copy(b, c);

#define M1(row,col) c[col * 4 + row]
    if (a[X] != 0.0) {
	mat4x4_id(tmp);
	angle = a[X] * DEG2RAD;
	M1(1,1) = cos(angle);
	M1(1,2) = -1.0 * sin(angle);
	M1(2,1) = sin(angle);
	M1(2,2) = cos(angle);
    	mat4x4_mult(c, tmp, c);
    } else if (a[Y] != 0.0) {
	mat4x4_id(tmp);
	angle = a[Y] * DEG2RAD;
	M1(0,0) = cos(angle);
	M1(0,2) = sin(angle);
	M1(2,0) = -1.0 * sin(angle);
	M1(2,2) = cos(angle);
    	mat4x4_mult(c, tmp, c);
    } else if (a[Z] != 0.0) {
	mat4x4_id(tmp);
	angle = a[Z] * DEG2RAD;
	M1(0,0) = cos(angle);
	M1(0,1) = -1.0 * sin(angle);
	M1(1,0) = sin(angle);
	M1(1,1) = cos(angle);
    	mat4x4_mult(c, tmp, c);
    }
#undef M1

    matrix_widget_set(matrix3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
}

void
rotate_vector_quat_quat(void)
{
    vec3_t a;
    quat_t b, c;

    vector_widget_get(vector1, a);
    quat_widget_get(quat2, b);

    quat_cp(b, c);

    quat_set3(c, a[X], a[Y], a[Z]);

    quat_widget_set(quat3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_QUAT);
}

void
button_rotate_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_VM:
	    rotate_vector_matrix_matrix();
	    break;
	case MODE_VQ:
	    rotate_vector_quat_quat();
	    break;
	default:
	    printf("ERROR: rotate mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
//////////////////////////  SWAP BUTTON  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
swap_matrix_matrix(void)
{
    mat4x4_t a, b, c;

    matrix_widget_get(matrix1, a);
    matrix_widget_get(matrix2, b);

    printf(">>> swapping values of matrix a & matrix b\n");

    mat4x4_copy(a, c);
    mat4x4_copy(b, a);
    mat4x4_copy(c, b);

    matrix_widget_set(matrix1, a);
    matrix_widget_set(matrix2, b);
}

void
swap_vector_vector(void)
{
    vec3_t a, b, c;

    vector_widget_get(vector1, a);
    vector_widget_get(vector2, b);

    printf(">>> swapping values of vector a & vector b\n");

    vec3_cp(a, c);
    vec3_cp(b, a);
    vec3_cp(c, b);

    vector_widget_set(vector1, a);
    vector_widget_set(vector2, b);
}

void
swap_quat_quat(void)
{
    quat_t a, b, c;

    quat_widget_get(quat1, a);
    quat_widget_get(quat2, b);

    printf(">>> swapping values of quaternion a & quaternion b\n");

    quat_cp(a, c);
    quat_cp(b, a);
    quat_cp(c, b);

    quat_widget_set(quat1, a);
    quat_widget_set(quat2, b);
}

void
button_swap_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_MM:
	    swap_matrix_matrix();
	    break;
	case MODE_VV:
	    swap_vector_vector();
	    break;
	case MODE_QQ:
	    swap_quat_quat();
	    break;
	default:
	    printf("ERROR: swap mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////  TRANSFORM BUTTON  ////////////////////////////
///////////////////////////////////////////////////////////////////////

void
transform_matrix_vector_vector(void)
{
    mat4x4_t a;
    vec3_t b, c;

    printf(">>> transforming vector b by matrix a\n");

    matrix_widget_get(matrix1, a);
    vector_widget_get(vector2, b);

    vec3_cp(b, c);

// does not apply translation
//    transform_point(cc, a, bb);

    vec3_transform(&c, 1, a);

    vector_widget_set(vector3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
}

void
button_transform_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    switch (MathMode) {
	case MODE_MV:
	    transform_matrix_vector_vector();
	    break;
	default:
	    printf("ERROR: transform mode unimplemented\n");
	    break;
    }
}

///////////////////////////////////////////////////////////////////////
////////////////////  NOTEBOOK RELATED CALLBACKS  /////////////////////
///////////////////////////////////////////////////////////////////////

void
recalculate_active_operations(void)
{
    int page_a, page_b, page_c;

    page_a = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_a));
    page_b = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_b));
    page_c = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_c));

    // disable all buttons
    gtk_widget_set_sensitive(button_add, FALSE);
    gtk_widget_set_sensitive(button_subtract, FALSE);
    gtk_widget_set_sensitive(button_multiply, FALSE);
    gtk_widget_set_sensitive(button_divide, FALSE);
    gtk_widget_set_sensitive(button_dot, FALSE);
    gtk_widget_set_sensitive(button_cross, FALSE);
    gtk_widget_set_sensitive(button_angle, FALSE);
    gtk_widget_set_sensitive(button_transform, FALSE);
    gtk_widget_set_sensitive(button_scale, FALSE);
    gtk_widget_set_sensitive(button_translate, FALSE);
    gtk_widget_set_sensitive(button_rotate, FALSE);
    gtk_widget_set_sensitive(button_swap, FALSE);

    MathMode = 0;

    // i probably could have used a table here but this is easy enough to work with
    switch (page_a) {
	case NOTEBOOK_PAGE_MATRIX:
	    switch (page_b) {
		case NOTEBOOK_PAGE_MATRIX:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
		    gtk_widget_set_sensitive(button_multiply, TRUE);
		    gtk_widget_set_sensitive(button_swap, TRUE);
		    MathMode = MODE_MM;
		    break;
		case NOTEBOOK_PAGE_VECTOR:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
		    gtk_widget_set_sensitive(button_transform, TRUE);
		    MathMode = MODE_MV;
		    break;
		default:
		    break;
	    }
	    break;
	case NOTEBOOK_PAGE_VECTOR:
	    switch (page_b) {
		case NOTEBOOK_PAGE_VECTOR:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
		    gtk_widget_set_sensitive(button_add, TRUE);
		    gtk_widget_set_sensitive(button_subtract, TRUE);
		    gtk_widget_set_sensitive(button_multiply, TRUE);
		    gtk_widget_set_sensitive(button_divide, TRUE);
		    gtk_widget_set_sensitive(button_dot, TRUE);
		    gtk_widget_set_sensitive(button_cross, TRUE);
		    gtk_widget_set_sensitive(button_angle, TRUE);
		    gtk_widget_set_sensitive(button_swap, TRUE);
		    MathMode = MODE_VV;
		    break;
		case NOTEBOOK_PAGE_MATRIX:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
		    gtk_widget_set_sensitive(button_translate, TRUE);
		    gtk_widget_set_sensitive(button_rotate, TRUE);
		    MathMode = MODE_VM;
		    break;
		case NOTEBOOK_PAGE_QUAT:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_QUAT);
		    gtk_widget_set_sensitive(button_rotate, TRUE);
		    MathMode = MODE_VQ;
		default:
		    break;
	    }
	    break;
	case NOTEBOOK_PAGE_QUAT:
	    switch (page_b) {
		case NOTEBOOK_PAGE_QUAT:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_QUAT);
		    gtk_widget_set_sensitive(button_add, TRUE);
		    gtk_widget_set_sensitive(button_subtract, TRUE);
		    gtk_widget_set_sensitive(button_multiply, TRUE);
		    gtk_widget_set_sensitive(button_swap, TRUE);
		    MathMode = MODE_QQ;
		    break;
		default:
		    break;
	    }
	    break;
	case NOTEBOOK_PAGE_SCALAR:
	    switch (page_b) {
		case NOTEBOOK_PAGE_MATRIX:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_MATRIX);
		    gtk_widget_set_sensitive(button_scale, TRUE);
		    MathMode = MODE_SM;
		    break;
		case NOTEBOOK_PAGE_VECTOR:
		    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_VECTOR);
		    // same operation
		    gtk_widget_set_sensitive(button_multiply, TRUE);
		    gtk_widget_set_sensitive(button_divide, TRUE);
		    gtk_widget_set_sensitive(button_scale, TRUE);
		    MathMode = MODE_SV;
		    break;
		default:
		    break;
	    }
	    break;
	default:
	    break;
    }
}

void
notebook_a_switch_page_cb(GtkWidget *widget, gpointer user_data)
{
    recalculate_active_operations();
}

void
notebook_b_switch_page_cb(GtkWidget *widget, gpointer user_data)
{
    recalculate_active_operations();
}

void
notebook_c_switch_page_cb(GtkWidget *widget, gpointer user_data)
{
    //recalculate_active_operations();
}

///////////////////////////////////////////////////////////////////////
///////////////////  QUATERNION TOOLBAR  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
quat_toolbar_button_zero_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_quat_toolbar_t *qt = user_data;
    g_assert(qt != NULL);

    quat_t q;

    printf(">>> zeroing quaternion: 0x%08x\n", qt);

    quat_zero(q);
    quat_widget_set(qt->q, q);
}

// NOTE: very similar function: quat_result_toolbar_button_unitize_clicked_cb
void
quat_toolbar_button_unitize_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_quat_toolbar_t *qt = user_data;
    g_assert(qt != NULL);

    quat_t q;

    printf(">>> unitizing quaternion cb: 0x%08x\n", qt);

    quat_widget_get(qt->q, q);
    quat_norm(q);
    quat_widget_set(qt->q, q);
}

void
quat_toolbar_button_id_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_quat_toolbar_t *qt = user_data;
    g_assert(qt != NULL);

    quat_t q;

    printf(">>> loading identity quaternion: 0x%08x\n", qt);

    quat_id(q);
    quat_widget_set(qt->q, q);
}

void
convert_quat_matrix(gtk_quat_t *quat, gtk_matrix_t *matrix)
{
    quat_t q;
    mat4x4_t m;

    quat_widget_get(quat, q);

    quat_to_mat(q, m);

    matrix_widget_set(matrix, m);
}

void
convert_quat_vector(gtk_quat_t *quat, gtk_vector_t *vector, int axis)
{
    quat_t q;
    vec3_t v;

    quat_widget_get(quat, q);

    if (axis == X) {
	quat_to_vecs(q, NULL, NULL, v);
    } else if (axis == Y) {
	quat_to_vecs(q, NULL, v, NULL);
    } else if (axis == Z) {
	quat_to_vecs(q, NULL, NULL, v);
    } else {
	abort();
    }

    vector_widget_set(vector, v);
}

void
quat_toolbar_convert_button_clicked_cb(GtkWidget *widget, void *user_data)
{
    gtk_quat_toolbar_t *qt = user_data;
    g_assert(qt != NULL);

    int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(qt->convert_select));

    switch (selected) {
	case QUAT_TOOLBAR_CONVERT_MATRIX:
	    if (qt == quat_toolbar1) {
		convert_quat_matrix(quat1, matrix1);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_MATRIX);
	    } else if (qt == quat_toolbar2) {
		convert_quat_matrix(quat2, matrix2);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_MATRIX);
	    }
	    break;
	case QUAT_TOOLBAR_CONVERT_X_VEC:
	    if (qt == quat_toolbar1) {
		convert_quat_vector(quat1, vector1, X);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_VECTOR);
	    } else if (qt == quat_toolbar2) {
		convert_quat_vector(quat2, vector2, X);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_VECTOR);
	    }
	    break;
	case QUAT_TOOLBAR_CONVERT_Y_VEC:
	    if (qt == quat_toolbar1) {
		convert_quat_vector(quat1, vector1, Y);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_VECTOR);
	    } else if (qt == quat_toolbar2) {
		convert_quat_vector(quat2, vector2, Y);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_VECTOR);
	    }
	    break;
	case QUAT_TOOLBAR_CONVERT_Z_VEC:
	    if (qt == quat_toolbar1) {
		convert_quat_vector(quat1, vector1, Z);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_VECTOR);
	    } else if (qt == quat_toolbar2) {
		convert_quat_vector(quat2, vector2, Z);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_VECTOR);
	    }
	    break;
	default:
	    return;
    }
}

gtk_quat_toolbar_t *
quat_toolbar_widget_new(gtk_quat_t *quat)
{
    gtk_quat_toolbar_t *qt;
    GtkWidget *box;

    // create object
    qt = malloc(sizeof(gtk_quat_toolbar_t));
    memset(qt, 0, sizeof(gtk_quat_toolbar_t));

    qt->q = quat;

    // get the container widget from the quat widget
    box = glade_xml_get_widget(quat->gwidget->xml, "quat_toolbar_container");
    g_assert(box != NULL);

    // load glade xml file
    qt->gwidget = glade_widget_new("quaternion_toolbar.glade", "window1", "quat_toolbar_box", box);

    // locate button widgets on the toolbar
    qt->button_zero = glade_xml_get_widget(qt->gwidget->xml, "quat_toolbar_button_zero");
    g_assert(qt->button_zero != NULL);

    qt->button_id = glade_xml_get_widget(qt->gwidget->xml, "quat_toolbar_button_id");
    g_assert(qt->button_id != NULL);

    qt->button_unitize = glade_xml_get_widget(qt->gwidget->xml, "quat_toolbar_button_unitize");
    g_assert(qt->button_unitize != NULL);

    qt->convert_select = glade_xml_get_widget(qt->gwidget->xml, "quat_toolbar_convert_select");
    g_assert(qt->convert_select != NULL);

    qt->convert_button = glade_xml_get_widget(qt->gwidget->xml, "quat_toolbar_convert_button");
    g_assert(qt->convert_button != NULL);

    // configure the signal handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(qt->button_zero), "clicked",
	    GTK_SIGNAL_FUNC(quat_toolbar_button_zero_clicked_cb), qt);
    gtk_signal_connect(GTK_OBJECT(qt->button_id), "clicked",
	    GTK_SIGNAL_FUNC(quat_toolbar_button_id_clicked_cb), qt);
    gtk_signal_connect(GTK_OBJECT(qt->button_unitize), "clicked",
	    GTK_SIGNAL_FUNC(quat_toolbar_button_unitize_clicked_cb), qt);
    gtk_signal_connect(GTK_OBJECT(qt->convert_button), "clicked",
	    GTK_SIGNAL_FUNC(quat_toolbar_convert_button_clicked_cb), qt);

    return qt;
}

///////////////////////////////////////////////////////////////////////
//////////////  QUATERNION RESULT TOOLBAR  ////////////////////////////
///////////////////////////////////////////////////////////////////////

void
quat_result_toolbar_button_copy_a_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_quat_result_toolbar_t *qrt = user_data;
    g_assert(qrt != NULL);

    quat_t q;

    printf(">>> copying result quaternion c to quaternion a: 0x%08x\n", qrt);

    quat_widget_get(qrt->q, q);
    quat_widget_set(quat1, q);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_QUAT);
}

void
quat_result_toolbar_button_copy_b_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_quat_result_toolbar_t *qrt = user_data;
    g_assert(qrt != NULL);

    quat_t q;

    printf(">>> copying result quaternion c to quaternion b: 0x%08x\n", qrt);

    quat_widget_get(qrt->q, q);
    quat_widget_set(quat2, q);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_QUAT);
}

void
quat_result_toolbar_button_unitize_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_quat_result_toolbar_t *qrt = user_data;
    g_assert(qrt != NULL);

    quat_t q;

    printf(">>> unitizing quaternion cb: 0x%08x\n", qrt);

    quat_widget_get(qrt->q, q);
    quat_norm(q);
    quat_widget_set(qrt->q, q);
}

gtk_quat_result_toolbar_t *
quat_result_toolbar_widget_new(gtk_quat_t *quat)
{
    gtk_quat_result_toolbar_t *qrt;
    GtkWidget *box;

    // create object
    qrt = malloc(sizeof(gtk_quat_result_toolbar_t));
    memset(qrt, 0, sizeof(gtk_quat_result_toolbar_t));

    qrt->q = quat;

    // get the container widget from the quat widget
    box = glade_xml_get_widget(quat->gwidget->xml, "quat_toolbar_container");
    g_assert(box != NULL);

    // load glade xml file
    qrt->gwidget = glade_widget_new("quaternion_result_toolbar.glade", "window1", "quat_result_toolbar_box", box);

    // locate button widgets on the toolbar
    qrt->button_copy_a = glade_xml_get_widget(qrt->gwidget->xml, "quat_result_toolbar_button_copy_a");
    g_assert(qrt->button_copy_a != NULL);

    qrt->button_copy_b = glade_xml_get_widget(qrt->gwidget->xml, "quat_result_toolbar_button_copy_b");
    g_assert(qrt->button_copy_b != NULL);

    qrt->button_unitize = glade_xml_get_widget(qrt->gwidget->xml, "quat_result_toolbar_button_unitize");
    g_assert(qrt->button_unitize != NULL);

    // configure the signal handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(qrt->button_copy_a), "clicked",
	    GTK_SIGNAL_FUNC(quat_result_toolbar_button_copy_a_clicked_cb), qrt);
    gtk_signal_connect(GTK_OBJECT(qrt->button_copy_b), "clicked",
	    GTK_SIGNAL_FUNC(quat_result_toolbar_button_copy_b_clicked_cb), qrt);
    gtk_signal_connect(GTK_OBJECT(qrt->button_unitize), "clicked",
	    GTK_SIGNAL_FUNC(quat_result_toolbar_button_unitize_clicked_cb), qrt);

    return qrt;
}

///////////////////////////////////////////////////////////////////////
///////////////////////  VECTOR TOOLBAR  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
vector_toolbar_button_zero_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_vector_toolbar_t *vt = user_data;
    g_assert(vt != NULL);

    vec3_t v;

    printf(">>> zeroing vector: 0x%08x\n", vt);

    vec3_zero(v);
    vector_widget_set(vt->v, v);
}

// NOTE: very similar function: vector_result_toolbar_button_unitize_clicked_cb
void
vector_toolbar_button_unitize_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_vector_toolbar_t *vt = user_data;
    g_assert(vt != NULL);

    vec3_t v;

    printf(">>> unitizing vector: 0x%08x\n", vt);

    vector_widget_get(vt->v, v);
    vec3_norm(v);
    vector_widget_set(vt->v, v);
}

void
vector_toolbar_button_length_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_vector_toolbar_t *vt = user_data;
    g_assert(vt != NULL);

    printf(">>> computing length of vector (distance from origin): 0x%08x\n", vt);

    vec3_t a;
    real c;

    vector_widget_get(vt->v, a);

    // #define vec3_len(v) sqrt((v)[X]*(v)[X] + (v)[Y]*(v)[Y] + (v)[Z]*(v)[Z])
    c = vec3_len(a);

    printf("C = sqrt((X*X) + (Y*Y) + (Z*Z))\n");
    printf("C = sqrt((%g*%g) + (%g*%g) + (%g*%g))\n", a[X], a[X], a[Y], a[Y], a[Z], a[Z]);
    printf("C = sqrt(%g + %g + %g)\n", a[X] * a[X], a[Y] * a[Y], a[Z] * a[Z]);
    printf("C = sqrt(%g)\n", a[X] * a[X] + a[Y] * a[Y] + a[Z] * a[Z]);

    scalar_widget_set(scalar3, c);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_c), NOTEBOOK_PAGE_SCALAR);
}

gtk_vector_toolbar_t *
vector_toolbar_widget_new(gtk_vector_t *vector)
{
    gtk_vector_toolbar_t *vt;
    GtkWidget *box;

    // create object
    vt = malloc(sizeof(gtk_vector_toolbar_t));
    memset(vt, 0, sizeof(gtk_vector_toolbar_t));

    vt->v = vector;

    // get the container widget from the vector widget
    box = glade_xml_get_widget(vector->gwidget->xml, "vector_toolbar_container");
    g_assert(box != NULL);

    // load glade xml file
    vt->gwidget = glade_widget_new("vector_toolbar.glade", "window1", "vector_toolbar_box", box);

    // locate button widgets on the toolbar
    vt->button_zero = glade_xml_get_widget(vt->gwidget->xml, "vector_toolbar_button_zero");
    g_assert(vt->button_zero != NULL);

    vt->button_length = glade_xml_get_widget(vt->gwidget->xml, "vector_toolbar_button_length");
    g_assert(vt->button_length != NULL);

    vt->button_unitize = glade_xml_get_widget(vt->gwidget->xml, "vector_toolbar_button_unitize");
    g_assert(vt->button_unitize != NULL);

    // configure the signal handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(vt->button_zero), "clicked",
	    GTK_SIGNAL_FUNC(vector_toolbar_button_zero_clicked_cb), vt);
    gtk_signal_connect(GTK_OBJECT(vt->button_length), "clicked",
	    GTK_SIGNAL_FUNC(vector_toolbar_button_length_clicked_cb), vt);
    gtk_signal_connect(GTK_OBJECT(vt->button_unitize), "clicked",
	    GTK_SIGNAL_FUNC(vector_toolbar_button_unitize_clicked_cb), vt);

    return vt;
}

///////////////////////////////////////////////////////////////////////
//////////////////  VECTOR RESULT TOOLBAR  ////////////////////////////
///////////////////////////////////////////////////////////////////////

void
vector_result_toolbar_button_copy_a_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_vector_result_toolbar_t *vrt = user_data;
    g_assert(vrt != NULL);

    vec3_t v;

    printf(">>> copying result vector c to vector a: 0x%08x\n", vrt);

    vector_widget_get(vrt->v, v);
    vector_widget_set(vector1, v);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_VECTOR);
}

void
vector_result_toolbar_button_copy_b_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_vector_result_toolbar_t *vrt = user_data;
    g_assert(vrt != NULL);

    vec3_t v;

    printf(">>> copying result vector c to vector b: 0x%08x\n", vrt);

    vector_widget_get(vrt->v, v);
    vector_widget_set(vector2, v);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_VECTOR);
}

// NOTE: very similar function: vector_toolbar_button_unitize_clicked_cb
void
vector_result_toolbar_button_unitize_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_vector_result_toolbar_t *vrt = user_data;
    g_assert(vrt != NULL);

    vec3_t v;

    printf(">>> unitizing result vector: 0x%08x\n", vrt);

    vector_widget_get(vrt->v, v);
    vec3_norm(v);
    vector_widget_set(vrt->v, v);
}

gtk_vector_result_toolbar_t *
vector_result_toolbar_widget_new(gtk_vector_t *vector)
{
    gtk_vector_result_toolbar_t *vrt;
    GtkWidget *box;

    // create object
    vrt = malloc(sizeof(gtk_vector_result_toolbar_t));
    memset(vrt, 0, sizeof(gtk_vector_result_toolbar_t));

    vrt->v = vector;

    // get the container widget from the vector widget
    box = glade_xml_get_widget(vector->gwidget->xml, "vector_toolbar_container");
    g_assert(box != NULL);

    // load glade xml file
    vrt->gwidget = glade_widget_new("vector_result_toolbar.glade", "window1", "vector_result_toolbar_box", box);

    // locate button widgets on the toolbar
    vrt->button_copy_a = glade_xml_get_widget(vrt->gwidget->xml, "vector_result_toolbar_button_copy_a");
    g_assert(vrt->button_copy_a != NULL);

    vrt->button_copy_b = glade_xml_get_widget(vrt->gwidget->xml, "vector_result_toolbar_button_copy_b");
    g_assert(vrt->button_copy_b != NULL);

    vrt->button_unitize = glade_xml_get_widget(vrt->gwidget->xml, "vector_result_toolbar_button_unitize");
    g_assert(vrt->button_unitize != NULL);

    // configure the signal handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(vrt->button_copy_a), "clicked",
	    GTK_SIGNAL_FUNC(vector_result_toolbar_button_copy_a_clicked_cb), vrt);
    gtk_signal_connect(GTK_OBJECT(vrt->button_copy_b), "clicked",
	    GTK_SIGNAL_FUNC(vector_result_toolbar_button_copy_b_clicked_cb), vrt);
    gtk_signal_connect(GTK_OBJECT(vrt->button_unitize), "clicked",
	    GTK_SIGNAL_FUNC(vector_result_toolbar_button_unitize_clicked_cb), vrt);

    return vrt;
}

///////////////////////////////////////////////////////////////////////
///////////////////////  MATRIX TOOLBAR  //////////////////////////////
///////////////////////////////////////////////////////////////////////

void
matrix_toolbar_button_id_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_matrix_toolbar_t *mt = user_data;
    g_assert(mt != NULL);

    mat4x4_t m;

    printf(">>> loading identity matrix: 0x%08x\n", mt);

    mat4x4_id(m);
    matrix_widget_set(mt->m, m);
}

void
matrix_toolbar_button_zero_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_matrix_toolbar_t *mt = user_data;
    g_assert(mt != NULL);

    mat4x4_t m;

    printf(">>> loading zeroing matrix: 0x%08x\n", mt);

    mat4x4_zero(m);
    matrix_widget_set(mt->m, m);
}

void
convert_matrix_quat(gtk_matrix_t *matrix, gtk_quat_t *quat)
{
//    mat4x4_t m;
//    quat_t q;

//    matrix_widget_get(matrix, m);

    printf(">>> matrix to quaternion conversion is not yet implemented\n");

//    quat_widget_set(quat, q);
}

void
convert_matrix_vector(gtk_matrix_t *matrix, gtk_vector_t *vector)
{
    mat4x4_t m;
    vec3_t v;

    matrix_widget_get(matrix, m);

    v[X] = m[12];
    v[Y] = m[13];
    v[Z] = m[14];

    vector_widget_set(vector, v);
}

void
matrix_toolbar_convert_button_clicked_cb(GtkWidget *widget, void *user_data)
{
    gtk_matrix_toolbar_t *mt = user_data;
    g_assert(mt != NULL);

    int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(mt->convert_select));

    switch (selected) {
	case MATRIX_TOOLBAR_CONVERT_VECTOR:
	    if (mt == matrix_toolbar1) {
		convert_matrix_vector(matrix1, vector1);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_VECTOR);
	    } else if (mt == matrix_toolbar2) {
		convert_matrix_vector(matrix2, vector2);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_VECTOR);
	    }
	    break;
	case MATRIX_TOOLBAR_CONVERT_QUAT:
	    if (mt == matrix_toolbar1) {
		convert_matrix_quat(matrix1, quat1);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_QUAT);
	    } else if (mt == matrix_toolbar2) {
		convert_matrix_quat(matrix2, quat2);
	    	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_QUAT);
	    }
	    break;
	default:
	    return;
    }
}

gtk_matrix_toolbar_t *
matrix_toolbar_widget_new(gtk_matrix_t *matrix)
{
    gtk_matrix_toolbar_t *mt;
    GtkWidget *box;

    // create object
    mt = malloc(sizeof(gtk_matrix_toolbar_t));
    memset(mt, 0, sizeof(gtk_matrix_toolbar_t));

    mt->m = matrix;

    // get the container widget from the matrix widget
    box = glade_xml_get_widget(matrix->gwidget->xml, "matrix_toolbar_container");
    g_assert(box != NULL);

    // load glade xml file
    mt->gwidget = glade_widget_new("matrix_toolbar.glade", "window1", "matrix_toolbar_box", box);

    // locate button widgets on the toolbar
    mt->button_id = glade_xml_get_widget(mt->gwidget->xml, "matrix_toolbar_button_id");
    g_assert(mt->button_id != NULL);

    mt->button_zero = glade_xml_get_widget(mt->gwidget->xml, "matrix_toolbar_button_zero");
    g_assert(mt->button_zero != NULL);

    mt->convert_select = glade_xml_get_widget(mt->gwidget->xml, "matrix_toolbar_convert_select");
    g_assert(mt->convert_select != NULL);

    mt->convert_button = glade_xml_get_widget(mt->gwidget->xml, "matrix_toolbar_convert_button");
    g_assert(mt->convert_button != NULL);

    // configure the signal handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(mt->button_id), "clicked",
	    GTK_SIGNAL_FUNC(matrix_toolbar_button_id_clicked_cb), mt);
    gtk_signal_connect(GTK_OBJECT(mt->button_zero), "clicked",
	    GTK_SIGNAL_FUNC(matrix_toolbar_button_zero_clicked_cb), mt);
    gtk_signal_connect(GTK_OBJECT(mt->convert_button), "clicked",
	    GTK_SIGNAL_FUNC(matrix_toolbar_convert_button_clicked_cb), mt);

    return mt;
}

///////////////////////////////////////////////////////////////////////
//////////////////  MATRIX RESULT TOOLBAR  ////////////////////////////
///////////////////////////////////////////////////////////////////////

void
matrix_result_toolbar_button_copy_a_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_matrix_result_toolbar_t *mrt = user_data;
    g_assert(mrt != NULL);

    mat4x4_t m;

    printf(">>> copying result matrix c to matrix a: 0x%08x\n", mrt);

    matrix_widget_get(mrt->m, m);
    matrix_widget_set(matrix1, m);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_a), NOTEBOOK_PAGE_MATRIX);
}

void
matrix_result_toolbar_button_copy_b_clicked_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_matrix_result_toolbar_t *mrt = user_data;
    g_assert(mrt != NULL);

    mat4x4_t m;

    printf(">>> copying result matrix c to matrix b: 0x%08x\n", mrt);

    matrix_widget_get(mrt->m, m);
    matrix_widget_set(matrix2, m);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_b), NOTEBOOK_PAGE_MATRIX);
}

gtk_matrix_result_toolbar_t *
matrix_result_toolbar_widget_new(gtk_matrix_t *matrix)
{
    gtk_matrix_result_toolbar_t *mrt;
    GtkWidget *box;

    // create object
    mrt = malloc(sizeof(gtk_matrix_result_toolbar_t));
    memset(mrt, 0, sizeof(gtk_matrix_result_toolbar_t));

    mrt->m = matrix;

    // get the container widget from the matrix widget
    box = glade_xml_get_widget(matrix->gwidget->xml, "matrix_toolbar_container");
    g_assert(box != NULL);

    // load glade xml file
    mrt->gwidget = glade_widget_new("matrix_result_toolbar.glade", "window1", "matrix_result_toolbar_box", box);

    // locate button widgets on the toolbar
    mrt->button_copy_a = glade_xml_get_widget(mrt->gwidget->xml, "matrix_result_toolbar_button_copy_a");
    g_assert(mrt->button_copy_a != NULL);

    mrt->button_copy_b = glade_xml_get_widget(mrt->gwidget->xml, "matrix_result_toolbar_button_copy_b");
    g_assert(mrt->button_copy_b != NULL);

    // configure the signal handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(mrt->button_copy_a), "clicked",
	    GTK_SIGNAL_FUNC(matrix_result_toolbar_button_copy_a_clicked_cb), mrt);
    gtk_signal_connect(GTK_OBJECT(mrt->button_copy_b), "clicked",
	    GTK_SIGNAL_FUNC(matrix_result_toolbar_button_copy_b_clicked_cb), mrt);

    return mrt;
}

///////////////////////////////////////////////////////////////////////
/////////////////  SCREEN RESOLTUON WARNING DIALOG  ///////////////////
///////////////////////////////////////////////////////////////////////

void
screenreswarning_dialog_run(void)
{
    GtkWidget *screenreswarning_dialog;
    GladeXML *xml;
    char buf[BUFSIZ];
    int ret;

    /* load the interface */
    if (find_data_file("main_window.glade", buf, BUFSIZ) != TRUE) {
	fprintf(stderr, "ERROR: unable to locate glade file: '%s'\n", "main_window.glade");
	exit(1);
    }
    xml = glade_xml_new(buf, NULL, NULL);
    g_assert(xml != NULL);

    // locate screenreswarning dialog widget
    screenreswarning_dialog = glade_xml_get_widget(xml, "screenreswarningdialog");
    g_assert(screenreswarning_dialog != NULL);

    gtk_window_set_position(GTK_WINDOW(screenreswarning_dialog), GTK_WIN_POS_CENTER);

    gtk_widget_set_visible(screenreswarning_dialog, TRUE);

    ret = gtk_dialog_run(GTK_DIALOG(screenreswarning_dialog));
    fprintf(stderr, "dialog returned: %d\n", ret);

    if (ret != 1) {
	exit(0);
    }
}

///////////////////////////////////////////////////////////////////////
////////////////////////  ABOUT DIALOG  ///////////////////////////////
///////////////////////////////////////////////////////////////////////

void
about_dialog_deactivate(GtkWidget *about_dialog)
{
    gtk_widget_set_visible(about_dialog, FALSE);
    gtk_widget_destroy(about_dialog);
}

void
about_dialog_activate(GtkWidget *widget)
{
    GtkWidget *about_dialog;
    GladeXML *xml;
    char buf[BUFSIZ];

    /* load the interface */
    if (find_data_file("main_window.glade", buf, BUFSIZ) != TRUE) {
	fprintf(stderr, "ERROR: unable to locate glade file: '%s'\n", "main_window.glade");
	exit(1);
    }
    xml = glade_xml_new(buf, NULL, NULL);
    g_assert(xml != NULL);

    // locate about dialog widget
    about_dialog = glade_xml_get_widget(xml, "aboutdialog");
    g_assert(about_dialog != NULL);

    gtk_window_set_position(GTK_WINDOW(about_dialog), GTK_WIN_POS_CENTER);

    gtk_widget_set_visible(about_dialog, TRUE);

    glade_xml_signal_autoconnect(xml);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void
window_delete_event_cb(GtkWidget *widget, gpointer user_data)
{
    exit(0);
}

int
main(int argc, char **argv)
{
    GladeXML *xml1;
    GtkWidget *box1, *box2, *box3, *box4, *box5, *box6, *box7, *box8, *box9, *box10, *box11;
    mat4x4_t m;
    char buf[BUFSIZ];
    gint width;

    mat4x4_id(m);

    // custom resources
    if (find_data_file("gtkrc", buf, BUFSIZ) == TRUE) {
	gtk_rc_add_default_file(buf);
    }

    gtk_init(&argc, &argv);

    GdkScreen *gdk_screen = gdk_screen_get_default();
    width = gdk_screen_get_width(gdk_screen);

    /*
     * Display a warning for users with a display 
     * less than 1280 pixels wide. I contemplated 
     * designing the user interface to be compatible
     * with lower resolution displays (like my laptop),
     * but it seemed like a less efficient design.
     * Either the user would not be able to see both
     * matrices & the result at the same time or the
     * math & notes window would have to be removed.
     *
     * Perhaps a low res / high res layout option
     * can be developed.
     */ 
    if (width < 1280)
	screenreswarning_dialog_run();

    /* load the interface */
    if (find_data_file("main_window.glade", buf, BUFSIZ) != TRUE) {
	fprintf(stderr, "ERROR: cannot locate glade file: '%s'\n", "main_window.glade");
	exit(1);
    }
    xml1 = glade_xml_new(buf, NULL, NULL);
    if (xml1 == NULL) {
	fprintf(stderr, "ERROR: cannot load glade file: '%s'\n", "main_window.glade");
	exit(1);
    }

    // locate window widget
    window = glade_xml_get_widget(xml1, "window");
    g_assert(window != NULL);
    gtk_widget_set_visible(window, TRUE);

    // locate box widgets which contain the matrix, vector, quaternion subwidgets
    box1 = glade_xml_get_widget(xml1, "matrix_vbox1");
    g_assert(box1 != NULL);

    box2 = glade_xml_get_widget(xml1, "matrix_vbox2");
    g_assert(box2 != NULL);

    box3 = glade_xml_get_widget(xml1, "matrix_vbox3");
    g_assert(box3 != NULL);

    box4 = glade_xml_get_widget(xml1, "vector_vbox1");
    g_assert(box3 != NULL);

    box5 = glade_xml_get_widget(xml1, "vector_vbox2");
    g_assert(box3 != NULL);

    box6 = glade_xml_get_widget(xml1, "vector_vbox3");
    g_assert(box6 != NULL);

    box7 = glade_xml_get_widget(xml1, "quat_vbox1");
    g_assert(box7 != NULL);

    box8 = glade_xml_get_widget(xml1, "quat_vbox2");
    g_assert(box8 != NULL);

    box9 = glade_xml_get_widget(xml1, "quat_vbox3");
    g_assert(box9 != NULL);

    box10 = glade_xml_get_widget(xml1, "scalar_vbox1");
    g_assert(box10 != NULL);

    box11 = glade_xml_get_widget(xml1, "scalar_vbox3");
    g_assert(box11 != NULL);

    ///////////////////////////////////////////////////////////

    button_add = glade_xml_get_widget(xml1, "button_add");
    g_assert(button_add != NULL);

    button_subtract = glade_xml_get_widget(xml1, "button_subtract");
    g_assert(button_subtract != NULL);

    button_multiply = glade_xml_get_widget(xml1, "button_multiply");
    g_assert(button_multiply != NULL);

    button_divide = glade_xml_get_widget(xml1, "button_divide");
    g_assert(button_divide != NULL);

    button_dot = glade_xml_get_widget(xml1, "button_dot");
    g_assert(button_dot != NULL);

    button_cross = glade_xml_get_widget(xml1, "button_cross");
    g_assert(button_cross != NULL);

    button_angle = glade_xml_get_widget(xml1, "button_angle");
    g_assert(button_angle != NULL);

    button_transform = glade_xml_get_widget(xml1, "button_transform");
    g_assert(button_transform != NULL);

    button_scale = glade_xml_get_widget(xml1, "button_scale");
    g_assert(button_scale != NULL);

    button_translate = glade_xml_get_widget(xml1, "button_translate");
    g_assert(button_translate != NULL);

    button_rotate = glade_xml_get_widget(xml1, "button_rotate");
    g_assert(button_rotate != NULL);

    button_swap = glade_xml_get_widget(xml1, "button_swap");
    g_assert(button_swap != NULL);

    ///////////////////////////////////////////////////////////

    notebook_a = glade_xml_get_widget(xml1, "notebook_a");
    g_assert(notebook_a != NULL);

    notebook_b = glade_xml_get_widget(xml1, "notebook_b");
    g_assert(notebook_b != NULL);

    notebook_c = glade_xml_get_widget(xml1, "notebook_c");
    g_assert(notebook_c != NULL);

    ///////////////////////////////////////////////////////////

    matrix1 = matrix_widget_new(box1);
    matrix_widget_set(matrix1, m);
    matrix_toolbar1 = matrix_toolbar_widget_new(matrix1);

    matrix2 = matrix_widget_new(box2);
    matrix_widget_set(matrix2, m);
    matrix_toolbar2 = matrix_toolbar_widget_new(matrix2);

    matrix3 = matrix_widget_new(box3);
    matrix_widget_set(matrix3, m);
    matrix_result_toolbar = matrix_result_toolbar_widget_new(matrix3);

    ///////////////////////////////////////////////////////////

    GtkWidget *container;

    matrix_memory = memory_store_new(matrix_memory_toolbar_store, matrix_memory_toolbar_recall);
    quat_memory   = memory_store_new(quat_memory_toolbar_store, quat_memory_toolbar_recall);
    vector_memory = memory_store_new(vector_memory_toolbar_store, vector_memory_toolbar_recall);

    ///////////////////////////////////////////////////////////

    container = glade_xml_get_widget(matrix_result_toolbar->gwidget->xml, "matrix_result_toolbar_memory_hbox");
    g_assert(container != NULL);
    gtk_memory_toolbar_new(matrix_memory, container, matrix3);

    container = glade_xml_get_widget(matrix_toolbar1->gwidget->xml, "matrix_toolbar_recall_box");
    g_assert(container != NULL);
    gtk_memory_toolbar_new(matrix_memory, container, matrix1);
    gtk_recall_toolbar_new(matrix_memory, container, matrix1);

    container = glade_xml_get_widget(matrix_toolbar2->gwidget->xml, "matrix_toolbar_recall_box");
    g_assert(container != NULL);
    gtk_memory_toolbar_new(matrix_memory, container, matrix2);
    gtk_recall_toolbar_new(matrix_memory, container, matrix2);

    ///////////////////////////////////////////////////////////

    vector1 = vector_widget_new(box4);
    vector_toolbar1 = vector_toolbar_widget_new(vector1);

    vector2 = vector_widget_new(box5);
    vector_toolbar2 = vector_toolbar_widget_new(vector2);

    vector3 = vector_widget_new(box6);
    vector_result_toolbar = vector_result_toolbar_widget_new(vector3);

    ///////////////////////////////////////////////////////////

    container = glade_xml_get_widget(vector_result_toolbar->gwidget->xml, "vector_result_toolbar_memory_hbox");
    g_assert(container != NULL);
    gtk_memory_toolbar_new(vector_memory, container, vector3);

    container = glade_xml_get_widget(vector_toolbar1->gwidget->xml, "vector_toolbar_recall_hbox");
    g_assert(container != NULL);
    gtk_recall_toolbar_new(vector_memory, container, vector1);

    container = glade_xml_get_widget(vector_toolbar2->gwidget->xml, "vector_toolbar_recall_hbox");
    g_assert(container != NULL);
    gtk_recall_toolbar_new(vector_memory, container, vector2);

    ///////////////////////////////////////////////////////////

    quat1 = quat_widget_new(box7);
    quat_toolbar1 = quat_toolbar_widget_new(quat1);

    quat2 = quat_widget_new(box8);
    quat_toolbar2 = quat_toolbar_widget_new(quat2);

    quat3 = quat_widget_new(box9);
    quat_result_toolbar = quat_result_toolbar_widget_new(quat3);

    ///////////////////////////////////////////////////////////

    container = glade_xml_get_widget(quat_result_toolbar->gwidget->xml, "quat_result_toolbar_memory_hbox");
    g_assert(container != NULL);
    gtk_memory_toolbar_new(quat_memory, container, quat3);

    container = glade_xml_get_widget(quat_toolbar1->gwidget->xml, "quat_toolbar_recall_hbox");
    g_assert(container != NULL);
    gtk_recall_toolbar_new(quat_memory, container, quat1);

    container = glade_xml_get_widget(quat_toolbar2->gwidget->xml, "quat_toolbar_recall_hbox");
    g_assert(container != NULL);
    gtk_recall_toolbar_new(quat_memory, container, quat2);

    ///////////////////////////////////////////////////////////

    scalar1 = scalar_widget_new(box10);

    scalar3 = scalar_widget_new(box11);

    ///////////////////////////////////////////////////////////

    // configure text buffer
    text_view = glade_xml_get_widget(xml1, "textview_stdout");
    g_assert(text_view != NULL);

    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    red = gtk_text_buffer_create_tag(text_buffer, "red", "foreground", "#ff0000", NULL);
    cyan = gtk_text_buffer_create_tag(text_buffer, "cyan", "foreground", "#00ffff", NULL);

    reset();
    recalculate_active_operations();

    /* connect the signals in the interface */
    glade_xml_signal_autoconnect(xml1);

    /* start the event loop */
    gtk_main();

    return 0;
}
