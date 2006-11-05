#include "gegl-plugin.h"
#include "editor.h"
#include <png.h>

static gint
gegl_buffer_export_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         src_x,
                        gint         src_y,
                        gint         width,
                        gint         height);

#include <stdio.h>


static void set_to_defined (GtkWidget *export)
{
  GeglRect rect;
  GtkEntry *x0 = g_object_get_data (G_OBJECT (export), "x0");
  GtkEntry *y0 = g_object_get_data (G_OBJECT (export), "y0");
  GtkEntry *width = g_object_get_data (G_OBJECT (export), "width");
  GtkEntry *height = g_object_get_data (G_OBJECT (export), "height");
  gchar buf[128];

  rect = gegl_node_get_defined_rect (editor.gegl);

  sprintf (buf, "%i", rect.x);
  gtk_entry_set_text (x0, buf);
  sprintf (buf, "%i", rect.y);
  gtk_entry_set_text (y0, buf);
  sprintf (buf, "%i", rect.w);
  gtk_entry_set_text (width, buf);
  sprintf (buf, "%i", rect.h);
  gtk_entry_set_text (height, buf);
}

static void button_defined_clicked (GtkButton *button,
                                    gpointer   user_data)
{
  set_to_defined (GTK_WIDGET (user_data));
}

static GeglRect get_input_rect (void)
{
  GeglNode *iter = gegl_graph_output (editor.gegl, "output");
  gegl_node_get_defined_rect (editor.gegl);  /* to trigger defined setting for all */
  while (iter &&
         gegl_node_get_connected_to (iter, "input")){
    iter = gegl_node_get_connected_to (iter, "input");
  }
  return iter->have_rect;
}

static void button_input_clicked (GtkButton *button,
                                  gpointer   user_data)
{
  GtkWidget *export = GTK_WIDGET (user_data);
  GeglRect rect={42,42,42,42};
  GtkEntry *x0 = g_object_get_data (G_OBJECT (export), "x0");
  GtkEntry *y0 = g_object_get_data (G_OBJECT (export), "y0");
  GtkEntry *width = g_object_get_data (G_OBJECT (export), "width");
  GtkEntry *height = g_object_get_data (G_OBJECT (export), "height");
  gchar buf[128];
  rect = get_input_rect ();

  sprintf (buf, "%i", rect.x);
  gtk_entry_set_text (x0, buf);
  sprintf (buf, "%i", rect.y);
  gtk_entry_set_text (y0, buf);
  sprintf (buf, "%i", rect.w);
  gtk_entry_set_text (width, buf);
  sprintf (buf, "%i", rect.h);
  gtk_entry_set_text (height, buf);
}

static void button_view_clicked (GtkButton *button,
                                 gpointer   user_data)
{
  GtkWidget *export = GTK_WIDGET (user_data);
  GeglRect rect={23,23,42,42};
  GtkEntry *x0 = g_object_get_data (G_OBJECT (export), "x0");
  GtkEntry *y0 = g_object_get_data (G_OBJECT (export), "y0");
  GtkEntry *width = g_object_get_data (G_OBJECT (export), "width");
  GtkEntry *height = g_object_get_data (G_OBJECT (export), "height");
  gchar buf[128];

  g_object_get (G_OBJECT (editor.drawing_area), "x", &rect.x, "y" ,&rect.y, NULL);
  rect.w= GTK_WIDGET(editor.drawing_area)->allocation.width;
  rect.h= GTK_WIDGET(editor.drawing_area)->allocation.height;

  sprintf (buf, "%i", rect.x);
  gtk_entry_set_text (x0, buf);
  sprintf (buf, "%i", rect.y);
  gtk_entry_set_text (y0, buf);
  sprintf (buf, "%i", rect.w);
  gtk_entry_set_text (width, buf);
  sprintf (buf, "%i", rect.h);
  gtk_entry_set_text (height, buf);
}

static void button_render_clicked (GtkButton *button,
                                   gpointer   user_data)
{
  GeglProjection *projection;
  GeglRect   rect;
  GtkWidget *export = GTK_WIDGET (user_data);
  GtkEntry *x0 = g_object_get_data (G_OBJECT (export), "x0");
  GtkEntry *y0 = g_object_get_data (G_OBJECT (export), "y0");
  GtkEntry *width = g_object_get_data (G_OBJECT (export), "width");
  GtkEntry *height = g_object_get_data (G_OBJECT (export), "height");
  GtkEntry *pathe= g_object_get_data (G_OBJECT (export), "path");
  const gchar *path;

  rect.x = atoi (gtk_entry_get_text (x0));
  rect.y = atoi (gtk_entry_get_text (y0));
  rect.w = atoi (gtk_entry_get_text (width));
  rect.h = atoi (gtk_entry_get_text (height));

  path = gtk_entry_get_text (pathe);
  
  projection = gegl_view_get_projection (GEGL_VIEW (editor.drawing_area));

  gegl_projection_update_rect (projection, rect);

  while (gegl_projection_render (projection));

  gegl_buffer_export_png (gegl_projection_get_buffer (projection),
                          path,rect.x,rect.y,rect.w,rect.h);
}

void export_window (void)
{
  GtkWidget *vbox;
  GtkWidget *window;
  GtkWidget *area_frame;
  GtkWidget *bitmapsize_frame;
  GtkWidget *filename_frame;

  if (editor.export_window)
    {
      gtk_widget_show (editor.export_window);
      /* set_to_defined (window); */
      return;
    }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Export Bitmap (Ctrl+Shift+E)");
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (editor.window));

  vbox = gtk_vbox_new (FALSE, 2);
  {
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *button;
    area_frame = gtk_frame_new ("Export Area");
    hbox = gtk_hbox_new (TRUE, 2);
    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (area_frame), vbox);

    button = gtk_button_new_with_label ("View");
    gtk_container_add (GTK_CONTAINER (hbox), button);
      g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (button_view_clicked), window);
    button = gtk_button_new_with_label ("Input");
    gtk_container_add (GTK_CONTAINER (hbox), button);
      g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (button_input_clicked), window);
    button = gtk_button_new_with_label ("Defined");
      g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (button_defined_clicked), window);
    gtk_container_add (GTK_CONTAINER (hbox), button);

    gtk_container_add (GTK_CONTAINER (vbox), hbox);

    {
      GtkWidget *hbox  = gtk_hbox_new (TRUE, 0);
      GtkWidget *label = gtk_label_new ("x0,y0");
      GtkWidget *entryx = gtk_entry_new ();
      GtkWidget *entryy = gtk_entry_new ();
      gtk_container_add (GTK_CONTAINER (hbox), label);
      gtk_container_add (GTK_CONTAINER (hbox), entryx);
      gtk_container_add (GTK_CONTAINER (hbox), entryy);
      gtk_container_add (GTK_CONTAINER (vbox), hbox);

      g_object_set_data (G_OBJECT (window), "x0", entryx);
      g_object_set_data (G_OBJECT (window), "y0", entryy);
    }

    {
      GtkWidget *hbox  = gtk_hbox_new (TRUE, 0);
      GtkWidget *label = gtk_label_new ("width&height");
      GtkWidget  *entryw = gtk_entry_new ();
      GtkWidget  *entryh = gtk_entry_new ();
      gtk_container_add (GTK_CONTAINER (hbox), label);
      gtk_container_add (GTK_CONTAINER (hbox), entryw);
      gtk_container_add (GTK_CONTAINER (hbox), entryh);
      gtk_container_add (GTK_CONTAINER (vbox), hbox);

      g_object_set_data (G_OBJECT (window), "width", entryw);
      g_object_set_data (G_OBJECT (window), "height", entryh);
    }
  }
  gtk_container_add (GTK_CONTAINER (vbox), area_frame);

  {
    GtkWidget *label = gtk_label_new ("The only supported unit at the moment is pixels\nThus the size is the above width/height.");
    bitmapsize_frame = gtk_frame_new ("Bitmap size");
    gtk_container_add (GTK_CONTAINER (bitmapsize_frame), label);
  }
  gtk_container_add (GTK_CONTAINER (vbox), bitmapsize_frame);

  {
      GtkWidget *hbox  = gtk_hbox_new (TRUE, 0);
      GtkWidget  *entry = gtk_entry_new ();
      
      gtk_container_add (GTK_CONTAINER (hbox), entry);
    filename_frame = gtk_frame_new ("Filename");
    gtk_container_add (GTK_CONTAINER (filename_frame), hbox);
      g_object_set_data (G_OBJECT (window), "path", entry);
    gtk_entry_set_text (GTK_ENTRY (entry), "/tmp/gegl-output.png");
  }
  gtk_container_add (GTK_CONTAINER (vbox), filename_frame);

    {
      GtkWidget *button = gtk_button_new_with_label ("Render");
      GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

      gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (""), TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER (vbox), hbox);

      
      g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (button_render_clicked), window);
    }
  gtk_widget_show_all (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  g_signal_connect (window, "delete-event",
                            G_CALLBACK (gtk_widget_hide_on_delete),
                            NULL);

  set_to_defined (window);
  gtk_widget_show (window);
  editor.export_window = window;
}


static gint
gegl_buffer_export_png (GeglBuffer      *gegl_buffer,
                        const gchar     *path,
                        gint             src_x,
                        gint             src_y,
                        gint             width,
                        gint             height)
{
  gint           row_stride = width * 4;
  FILE          *fp;
  gint           i;
  png_struct    *png;
  png_info      *info;
  guchar        *pixels;
  png_color_16   white;
  gchar          format_string[16];
  gint           bit_depth = 8;

  if (!strcmp (path, "-"))
    {
      fp = stdout;
    }
  else
    {
      fp = fopen (path, "wb");
    }
  if (!fp)
    {
      return -1;
    }

  strcpy (format_string, "R'G'B'A ");

  {
    BablFormat  *format = gegl_buffer->format;
    BablType   **type   = format->type;

    for (i=0; i<format->components; i++)
      if ((*type)->bits > 8)
        bit_depth = 16;
  }

  if (bit_depth == 16)
    {
      strcat (format_string, "u16");
      row_stride *= 2;
    }
  else
    {
      strcat (format_string, "u8");
    }

  png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL)
    return -1;

  info = png_create_info_struct (png);

  if (setjmp (png_jmpbuf (png)))
    return -1;
  png_set_compression_level (png, 1);
  png_init_io (png, fp);

  png_set_IHDR (png, info,
     width, height, bit_depth, PNG_COLOR_TYPE_RGB_ALPHA,
     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);

  white.red = 0xff;
  white.blue = 0xff;
  white.green = 0xff;
  png_set_bKGD (png, info, &white);

  png_write_info (png, info);

#if BYTE_ORDER == LITTLE_ENDIAN
  if (bit_depth > 8)
    png_set_swap (png);
#endif

  pixels = g_malloc0 (row_stride);

  for (i=0; i< height; i++)
    {
      GeglBuffer *rect = g_object_new (GEGL_TYPE_BUFFER,
                                       "source", gegl_buffer,
                                       "x", src_x,
                                       "y", src_y + i,
                                       "width", width,
                                       "height", 1,
                                       NULL);
      gegl_buffer_get_fmt (rect, pixels, babl_format (format_string));

      png_write_rows (png, &pixels, 1);
      g_object_unref (rect);

    }

  png_write_end (png, info);

  png_destroy_write_struct (&png, &info);
  g_free (pixels);

  if (fp!=stdout)
    fclose (fp);

  return 0;
}
