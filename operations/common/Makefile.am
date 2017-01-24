SUBDIRS = perlin

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)
AM_CFLAGS +=  -DGEGL_OP_BUNDLE
LIBS = $(op_libs)


opdir = $(ext_dir)
op_LTLIBRARIES = \
	gegl-common.la 
    
gegl_common_la_SOURCES =\
    alien-map.c \
    module.c \
	antialias.c \
	apply-lens.c \
	bilateral-filter.c \
	box-blur.c \
	brightness-contrast.c \
	buffer-sink.c \
	buffer-source.c \
	bump-map.c \
	c2g.c \
	cartoon.c \
	channel-mixer.c \
	checkerboard.c \
	color-enhance.c \
	color-exchange.c \
	dither.c \
	color-rotate.c \
	color-temperature.c \
	color-to-alpha.c \
        color-warp.c \
	color.c \
	contrast-curve.c \
	convolution-matrix.c \
	copy-buffer.c \
	cubism.c \
	deinterlace.c \
	difference-of-gaussians.c \
	diffraction-patterns.c \
	displace.c \
	display.c \
	distance-transform.c \
	dropshadow.c \
	edge.c \
	edge-laplace.c \
	edge-neon.c \
	edge-sobel.c \
	emboss.c \
	engrave.c \
	exp-combine.c \
	exposure.c \
	fattal02.c \
	fractal-explorer.c \
	fractal-trace.c \
	gaussian-blur.c \
	gaussian-blur-iir.c \
	gaussian-blur-selective.c \
	gblur-1d.c \
	gegl-buffer-load-op.c \
	gegl-buffer-save-op.c \
	gegl.c \
	grey.c \
	grid.c \
	high-pass.c \
	illusion.c \
	image-compare.c \
	introspect.c \
	invert-gamma.c \
	invert-linear.c \
	layer.c \
	lens-distortion.c \
	lens-flare.c \
	levels.c \
	linear-gradient.c \
	magick-load.c \
	mantiuk06.c \
	map-absolute.c \
	map-relative.c \
	matting-global.c \
	maze.c \
	mblur.c \
	mirrors.c \
	mono-mixer.c \
	mosaic.c \
	motion-blur-circular.c \
	motion-blur-linear.c \
	motion-blur-zoom.c \
	noise-cie-lch.c \
	noise-hsv.c \
	noise-hurl.c \
	noise-perlin.c \
	noise-pick.c \
	noise-reduction.c \
	noise-rgb.c \
	noise-simplex.c \
	noise-slur.c \
	noise-solid.c \
	noise-spread.c \
	noise-cell.c \
	oilify.c \
	opacity.c \
	open-buffer.c \
	over.c \
	panorama-projection.c \
	photocopy.c \
	pixelize.c \
	plasma.c \
	polar-coordinates.c \
	posterize.c \
	radial-gradient.c \
	rectangle.c \
	red-eye-removal.c \
	reinhard05.c \
	remap.c \
        rgb-clip.c \
	ripple.c \
	saturation.c \
	save.c \
	sepia.c \
	shift.c \
	sinus.c \
	snn-mean.c \
	softglow.c \
	stress.c \
	stretch-contrast-hsv.c \
	stretch-contrast.c \
	supernova.c \
	svg-huerotate.c \
	svg-luminancetoalpha.c \
	svg-matrix.c \
	svg-saturate.c \
	texturize-canvas.c \
	threshold.c \
	tile.c \
	tile-glass.c \
	tile-paper.c \
	tile-seamless.c \
	unsharp-mask.c \
	value-invert.c \
	value-propagate.c \
	video-degradation.c \
	vignette.c \
	warp.c \
	waves.c \
        wavelet-blur.c \
	weighted-blend.c \
	whirl-pinch.c \
	wind.c \
	write-buffer.c

module.c: $(gegl_common_la_SOURCES)
	$(top_srcdir)/tools/gen-loader.sh $^ > module.c
