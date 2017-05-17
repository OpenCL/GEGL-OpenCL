#include <stdio.h>
#include <math.h>
#include <glib.h>

#include "gegl.h"
#include "property-types/gegl-path.h"

#define SUCCESS  0
#define FAILURE -1

#define EPSILON 0.0005 // XXX < ideally we'd use a 1.5 orders of magnitude smaller epsilon

#define NSMP 3
static gboolean 
equals(double a, double b)
{
  return fabs(a-b) < EPSILON;
}
static void
distribute(double length, int num_samples, gdouble *result)
{
  int i=0;
  gdouble spacing = 0;
  if (num_samples>1)
    spacing = length/(num_samples-1);
  for (i=0; i<num_samples-1; i++)
    result[i]=spacing*i;
  if (num_samples>1)
    result[num_samples-1]= length;
}

static int 
test_path_get_length (GeglPath *path, gdouble exp_length)
{  
  gdouble length;
  length = gegl_path_get_length(path);

  if (! equals(length, exp_length))
    {
      g_printerr("test_path_get_length()\n");
      g_printerr("The length of path is incorrect.\n");
      g_printerr("length is %f, should be %f\n", length, exp_length);
      return FALSE;
    }
  return TRUE;
}

static int 
test_path_calc_values (GeglPath *path, int num_samples,
                       gdouble *exp_x, gdouble *exp_y)
{
  int i=0;
  gdouble x[num_samples], y[num_samples];
  //gdouble length;
  /* gegl_path_calc_values:
   * Compute @num_samples for a path into the provided arrays @xs and @ys
   * the returned values include the start and end positions of the path.
   */
  //length = gegl_path_get_length(path);

  gegl_path_calc_values(path,num_samples,x,y);

  for (i=0; i<num_samples; i++)
    if (! (equals(x[i],exp_x[i]) && equals(y[i],exp_y[i])) )
    {
      g_printerr("test_path_calc_values()\n");
      g_printerr("Sample %d is incorrect.\n",i);
      for ( i=0;i<NSMP;i++)
        printf("Sample %d : x=%f exp_x=%f  y=%f exp_y=%f\n",
                i, x[i], exp_x[i], y[i], exp_y[i]);
      return FALSE;
    }
  return TRUE;
}

static int 
test_path_calc (GeglPath *path, gdouble pos,
                gdouble exp_x, gdouble exp_y)
{
  gdouble x=0.0, y=0.0;
  /* gegl_path_calc_values:
   * Compute @num_samples for a path into the provided arrays @xs and @ys
   * the returned values include the start and end positions of the path.
   */
  gegl_path_calc(path,pos,&x,&y);

  if (! (equals(x,exp_x) && equals(y,exp_y)) )
    {
      g_printerr("test_path_calc()\n");
      g_printerr("Calculated point %f incorrect.\n", pos);
      printf("x=%f exp_x=%f  y=%f exp_y=%f\n",
              x, exp_x, y, exp_y);
      return FALSE;
    }
  return TRUE;
}

static int
test_path_calc_y_for_x (GeglPath *path, gdouble x, gdouble exp_y)
{
  gdouble y = -1.0;
  gegl_path_calc_y_for_x (path, x, &y);
  if (! equals (y, exp_y))
  {
    fprintf (stderr, "got %f expected %f\n", y, exp_y);
    return FALSE;
  }
  return TRUE;
}

int main(int argc, char *argv[])
{
  gdouble exp_x[NSMP],exp_y[NSMP];
  int result=SUCCESS;
  int i=1, j=1;
  GeglPath *path = NULL;

  gegl_init (&argc, &argv);

  distribute (0.0, NSMP ,exp_y);
  distribute (1.0, NSMP ,exp_x);

  path = gegl_path_new ();
  gegl_path_append (path, 'M', 0.0, 0.0);
  gegl_path_append (path, 'L', 0.5, 0.0);
  gegl_path_append (path, 'L', 1.0, 0.0);
  if(! test_path_get_length(path, 1.0) )
    {
      g_printerr("The gegl_path_get_length() test #%d failed.\n",i);
      result += FAILURE;
    }
  /* path_calc forwards */
  for ( j=0;j<NSMP;j++)
    if(! test_path_calc(path, exp_x[j], exp_x[j], exp_y[j]) )
      {
        g_printerr("The gegl_path_calc() test #%d.%d failed.\n",i,j+1);
        result += FAILURE;
      }
  /* path_calc backwards */
  for ( j=NSMP-1;j>-1;j--)
    if(! test_path_calc(path, exp_x[j], exp_x[j], exp_y[j]) )
      {
        g_printerr("The gegl_path_calc() reverse test #%d.%d failed.\n",i,j+1);
        result += FAILURE;
      }
  if(! test_path_calc_values(path, NSMP, exp_x, exp_y) )
    {
      g_printerr("The gegl_path_calc_values() test #%d failed.\n",i);
      result += FAILURE;
    }

  i++;

  path = gegl_path_new ();
  gegl_path_append (path, 'M', 0.0, 0.0);
  gegl_path_append (path, 'L', 0.5, 0.0);
  gegl_path_append (path, 'M', 0.5, 0.0);
  gegl_path_append (path, 'L', 1.0, 0.0);
  if(! test_path_get_length(path, 1.0) )
    {
      g_printerr("The gegl_path_get_length() test #%d failed.\n",i);
      result += FAILURE;
    }
  /* path_calc forwards */
  for ( j=0;j<NSMP;j++)
    if(! test_path_calc(path, exp_x[j], exp_x[j], exp_y[j]) )
      {
        g_printerr("The gegl_path_calc() test #%d.%d failed.\n",i,j+1);
        result += FAILURE;
      }
  /* path_calc backwards */
  for ( j=NSMP-1;j>-1;j--)
    if(! test_path_calc(path, exp_x[j], exp_x[j], exp_y[j]) )
      {
        g_printerr("The gegl_path_calc() reverse test #%d.%d failed.\n",i,j+1);
        result += FAILURE;
      }
  if(! test_path_calc_values(path, NSMP, exp_x, exp_y) )
    {
      g_printerr("The gegl_path_calc_values() test #%d failed.\n",i);
      result += FAILURE;
    }

  /* path1   :    |--+--+--|--+--+--|
   * path2   :    |--+--+--|
   *                       |--+--+--|
   * 1sampl  :    ^ ?
   * 2sampl  :    ^                 ^
   * 3sampl  :    ^        ^        ^
   * 4sampl  :    ^     ^     ^     ^
   */


  path = gegl_path_new ();
  gegl_path_append (path, 'M', 0.0, 0.0);
  gegl_path_append (path, 'L', 0.5, 0.23);
  gegl_path_append (path, 'L', 1.0, 0.42);
  if (! test_path_calc_y_for_x (path, 0.5, 0.23))
  {
    g_printerr("The gegl_path_calc_y_for_x() 0.5 failed\n");
    result += FAILURE;
  }
  if (! test_path_calc_y_for_x (path, 1.0, 0.42))
  {
    g_printerr("The gegl_path_calc_y_for_x() 1.0 failed\n");
    result += FAILURE;
  }
  if (! test_path_calc_y_for_x (path, 0.25, 0.115))
  {
    g_printerr("The gegl_path_calc_y_for_x() 0.25 failed\n");
    result += FAILURE;
  }


  gegl_exit ();

  return result;
}
