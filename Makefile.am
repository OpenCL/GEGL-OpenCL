SUBDIRS = gegl

bin_SCRIPTS = \
	gegl-config

EXTRA_DIST = 		\
	gegl.m4		\
	geglConf.sh.in	\
	gegl-config.in

m4datadir = $(datadir)/aclocal
m4data_DATA = gegl.m4

## Put `exec' in the name because this should be installed by
## `install-exec', not `install-data'.

confexecdir=$(libdir)
