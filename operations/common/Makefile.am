SUBDIRS = perlin

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

BUILT_SOURCES = $(subst .c,.c.h,$(wildcard $(srcdir)/*.c))

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
	alien-map.la \
	antialias.la \
	apply-lens.la \
	bilateral-filter-fast.la \
	bilateral-filter.la \
	box-blur.la \
	brightness-contrast.la \
	buffer-sink.la \
	buffer-source.la \
	bump-map.la \
	c2g.la \
	cartoon.la \
	channel-mixer.la \
	checkerboard.la \
	color-enhance.la \
	color-exchange.la \
	color-reduction.la \
	color-rotate.la \
	color-temperature.la \
	color-to-alpha.la \
	color.la \
	contrast-curve.la \
	convolution-matrix.la \
	copy-buffer.la \
	cubism.la \
	deinterlace.la \
	difference-of-gaussians.la \
	diffraction-patterns.la \
	displace.la \
	display.la \
	distance-transform.la \
	dropshadow.la \
	edge.la \
	edge-laplace.la \
	edge-sobel.la \
	emboss.la \
	engrave.la \
	exp-combine.la \
	exposure.la \
	fattal02.la \
	fractal-explorer.la \
	fractal-trace.la \
	gaussian-blur.la \
	gaussian-blur-iir.la \
	gblur-1d.la \
	gegl-buffer-load-op.la \
	gegl-buffer-save-op.la \
	grey.la \
	grid.la \
	high-pass.la \
	illusion.la \
	image-compare.la \
	introspect.la \
	invert-gamma.la \
	invert-linear.la \
	layer.la \
	lens-distortion.la \
	lens-flare.la \
	levels.la \
	linear-gradient.la \
	load.la \
	magick-load.la \
	mantiuk06.la \
	map-absolute.la \
	map-relative.la \
	matting-global.la \
	maze.la \
	mblur.la \
	mirrors.la \
	mono-mixer.la \
	mosaic.la \
	motion-blur-circular.la \
	motion-blur-linear.la \
	motion-blur-zoom.la \
	noise-cie-lch.la \
	noise-hsv.la \
	noise-hurl.la \
	noise-perlin.la \
	noise-pick.la \
	noise-reduction.la \
	noise-rgb.la \
	noise-simplex.la \
	noise-slur.la \
	noise-solid.la \
        noise-spread.la \
	noise-cell.la \
	oilify.la \
	opacity.la \
	open-buffer.la \
	over.la \
	panorama-projection.la \
	photocopy.la \
	pixelize.la \
	plasma.la \
	polar-coordinates.la \
	posterize.la \
	dcraw-load.la \
	radial-gradient.la \
	rectangle.la \
	red-eye-removal.la \
	reinhard05.la \
	remap.la \
	ripple.la \
	save.la \
	sepia.la \
	shift.la \
	sinus.la \
	snn-mean.la \
	softglow.la \
	stress.la \
	stretch-contrast-hsv.la \
	stretch-contrast.la \
	supernova.la \
	svg-huerotate.la \
	svg-luminancetoalpha.la \
	svg-matrix.la \
	svg-saturate.la \
	texturize-canvas.la \
	threshold.la \
	tile.la \
	tile-glass.la \
	tile-paper.la \
	tile-seamless.la \
	unsharp-mask.la \
	value-invert.la \
	value-propagate.la \
        video-degradation.la \
	vignette.la \
	warp.la \
	waves.la \
	weighted-blend.la \
	whirl-pinch.la \
	wind.la \
	write-buffer.la
