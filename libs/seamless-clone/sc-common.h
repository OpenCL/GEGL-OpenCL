/**
 * The name of the Babl type used for working on colors.
 * WARNING: THE CODE ASSUMES THAT NO MATTER WHAT THE FORMAT IS, THE
 * ALPHA CHANNEL IS ALWAYS LAST. DO NOT CHANGE THIS WITHOUT UPDATING
 * THE REST OF THE CODE!
 */
#define SC_COLOR_BABL_NAME     "R'G'B'A float"

/**
 * The type used for individual color channels
 */
typedef gfloat ScColorChannel;

/**
 * The amount of channels per color
 */
#define SC_COLORA_CHANNEL_COUNT 4
#define SC_COLOR_CHANNEL_COUNT  ((SC_COLORA_CHANNEL_COUNT) - 1)

/**
 * The index of the alpha channel in the color
 */
#define SC_COLOR_ALPHA_INDEX   SC_COLOR_CHANNEL_COUNT

/**
 * The type used for a whole color
 */
typedef ScColorChannel ScColor[SC_COLORA_CHANNEL_COUNT];

/**
 * Apply a macro once for each non-alpha color channel, with the
 * channel index as an input
 */
#define sc_color_process()  \
G_STMT_START                \
  sc_color_expr(0);         \
  sc_color_expr(1);         \
  sc_color_expr(2);         \
G_STMT_END

typedef struct {
  GeglBuffer    *bg;
  GeglRectangle *bg_rect;

  GeglBuffer    *fg;
  GeglRectangle *fg_rect;

  gint           xoff;
  gint           yoff;

  gboolean       render_bg;
} ScRenderInfo;

typedef struct {
  gint           x;
  gint           y;
} ScPoint;
