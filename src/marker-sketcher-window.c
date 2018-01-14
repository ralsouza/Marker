/*
 * marker-sketcher-window.c
 *
 * Copyright (C) 2017 - 2018 Marker Project
 *
 * Marker is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * Marker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with Marker; see the file LICENSE.md. If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */


#include "marker-sketcher-window.h"
#include <math.h>

struct _MarkerSketcherWindow
{
  GtkApplicationWindow        parent_instance;
};

G_DEFINE_TYPE(MarkerSketcherWindow, marker_sketcher_window, GTK_TYPE_APPLICATION_WINDOW)

MarkerSketcherWindow*  
marker_sketcher_window_new (GtkApplication * app)
{
    return g_object_new(MARKER_TYPE_SKETCHER_WINDOW, "application", app, NULL);
}


static void
marker_sketcher_window_class_init(MarkerSketcherWindowClass* class)
{
  
}

static void
marker_sketcher_window_init (MarkerSketcherWindow *sketcher)
{

}

static cairo_surface_t *surface = NULL;
static gboolean         status = FALSE;
static gdouble          old_x, old_y;

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
configure_event_cb (GtkWidget         *widget,
                    GdkEventConfigure *event,
                    gpointer           data)
{
  if (surface)
    cairo_surface_destroy (surface);

  surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               gtk_widget_get_allocated_width (widget),
                                               gtk_widget_get_allocated_height (widget));

  /* Initialize the surface to white */
  clear_surface ();

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush (GtkWidget *widget,
            gdouble    x,
            gdouble    y)
{
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);
  if (!status)
  {

    cairo_arc(cr, x, y, 3, 0, 2.0*M_PI) ;
    cairo_fill (cr);

    cairo_destroy (cr);
    gtk_widget_queue_draw_area (widget, x - 3, y - 3, 6, 6);
  } else
  {
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 6.0);
    cairo_move_to(cr, old_x, old_y);
    cairo_line_to(cr, x, y);
    cairo_stroke(cr);
    gdouble l = x > old_x ? old_x : x;
    gdouble t = y > old_y ? old_y : y;


    gtk_widget_queue_draw_area (widget, l - 3, t - 3, fabs(x-old_x) + 6, fabs(y-old_y) + 6);

  }
  status = TRUE;

  old_x = x;
  old_y = y;
  /* Now invalidate the affected region of the drawing area. */
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean
button_press_event_cb (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      draw_brush (widget, event->x, event->y);
    }
  else if (event->button == GDK_BUTTON_SECONDARY)
    {
      status = FALSE;
      clear_surface ();
      gtk_widget_queue_draw (widget);
    }

  /* We've handled the event, stop processing */
  return TRUE;
}

static gboolean
button_release_event_cb (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
  if (event->button == GDK_BUTTON_PRIMARY)
    {
        status = FALSE;
    }
  /* We've handled the event, stop processing */
  return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean
motion_notify_event_cb (GtkWidget      *widget,
                        GdkEventMotion *event,
                        gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->state & GDK_BUTTON1_MASK)
    draw_brush (widget, event->x, event->y);

  /* We've handled it, stop processing */
  return TRUE;
}



static void
init_ui ()
{
    GtkWidget *window;
    GtkWidget *frame;
    GtkWidget *drawing_area;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title (GTK_WINDOW (window), "Drawing Area");


    gtk_container_set_border_width (GTK_CONTAINER (window), 8);

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (window), frame);

    drawing_area = gtk_drawing_area_new ();
    /* set a minimum size */
    gtk_widget_set_size_request (drawing_area, 800, 600);

    gtk_container_add (GTK_CONTAINER (frame), drawing_area);

    /* Signals used to handle the backing surface */
    g_signal_connect (drawing_area, "draw",
                        G_CALLBACK (draw_cb), NULL);
    g_signal_connect (drawing_area,"configure-event",
                        G_CALLBACK (configure_event_cb), NULL);

    /* Event signals */
    g_signal_connect (drawing_area, "motion-notify-event",
                        G_CALLBACK (motion_notify_event_cb), NULL);
    g_signal_connect (drawing_area, "button-press-event",
                        G_CALLBACK (button_press_event_cb), NULL);
    g_signal_connect (drawing_area, "button-release-event",
                        G_CALLBACK (button_release_event_cb), NULL);

    /* Ask to receive events the drawing area doesn't normally
    * subscribe to. In particular, we need to ask for the
    * button press and motion notify events that want to handle.
    */
    gtk_widget_set_events (drawing_area, gtk_widget_get_events (drawing_area)
                                        | GDK_BUTTON_PRESS_MASK
                                        | GDK_BUTTON_RELEASE_MASK
                                        | GDK_POINTER_MOTION_MASK);

    gtk_widget_show_all (window);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
}


void
marker_sketcher_window_show()
{
    init_ui();    
}