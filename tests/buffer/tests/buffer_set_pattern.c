TEST ()
{
  gint i, j, k;
  GeglBuffer    *pattern, *output;
  GeglRectangle  pat_extent[3]    = {{0, 0, 5, 5},
                                     {-1, -2, 2, 3},
                                     {0, 0, 3, 2}};

  GeglRectangle  output_extent[3] = {{0, 0, 20, 20},
                                     {1, -1, 10, 7},
                                     {-1, -2, 2, 2}};

  gint offsets[4][2] = {{0, 0}, {1, 0}, {0, -1}, {4, 0}};

  test_start ();

  for (i = 0; i < G_N_ELEMENTS(offsets); ++i)
    for (j = 0; j < G_N_ELEMENTS(output_extent); ++j)
      for (k = 0; k < G_N_ELEMENTS(pat_extent); ++k)
        {
          GeglRectangle *current_pat_extent = pat_extent + k;
          GeglRectangle *current_output_extent = output_extent + j;
          gint           offset_x = offsets[i][0];
          gint           offset_y = offsets[i][1];

          pattern = gegl_buffer_new (current_pat_extent,    babl_format ("Y float"));
          output  = gegl_buffer_new (current_output_extent, babl_format ("Y float"));

          vgrad (pattern);

          gegl_buffer_set_pattern (output, NULL, pattern, offset_x, offset_y);

          print(("\npattern: (%d %d %d %d)\nbuffer: (%d %d %d %d)\noffsets: (%d %d)\n",
                 current_pat_extent->x, current_pat_extent->y,
                 current_pat_extent->width, current_pat_extent->height,
                 current_output_extent->x, current_output_extent->y,
                 current_output_extent->width, current_output_extent->height,
                 offset_x, offset_y));

          print(("Vertical gradient\n"));
          print_buffer (output);


          hgrad (pattern);
          gegl_buffer_set_pattern (output, NULL, pattern, offset_x, offset_y);

          print(("Horizontal gradient\n"));
          print_buffer (output);

          g_object_unref (pattern);
          g_object_unref (output);
        }

  test_end ();
}
