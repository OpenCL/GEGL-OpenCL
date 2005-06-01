## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.7

# The name of the module.
DOC_MODULE = gegl

# The top-level XML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.sgml

# The directory containing the source code.
DOC_SOURCE_DIR = $(top_srcdir)/$(DOC_MODULE)

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS =

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS =

# Used for dependencies
HFILE_GLOB = $(DOC_SOURCE_DIR)/*.h
CFILE_GLOB = $(DOC_SOURCE_DIR)/*.c

# Headers to ignore
IGNORE_HFILES =

# Images to copy into HTML directory
HTML_IMAGES = \
	images/collectinputs.png		\
	images/composite.png			\
	images/graph.png			\
	images/graphsource.png			\
	images/inputs_outputs_for_fade_op.png	\
	images/inputs_outputs_for_over_op.png	\
	images/inputs_outputs_for_color_op.png	\
	images/node_2inputs_1output.png		\
	images/node_input_outputs.png		\
	images/node_no_input_upstream.png	\
	images/sharednode.png			\
	images/simplegraph.png

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =

# Other files to distribute
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS = -I$(top_srcdir)/gegl @DEP_CFLAGS@
GTKDOC_LIBS = $(top_builddir)/gegl/@gegltargetlib@ @DEP_LIBS@


include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += version.in
