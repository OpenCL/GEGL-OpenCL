include $(top_srcdir)/operations/Makefile-operations.am

OFILES      = $(subst $(srcdir)/,,$(CFILES:.c=.o))
EXTRA_DIST += $(wildcard *.h)

SOBJS          = affine$(SHREXT)
INSTALLED_ITEMS = $(ext_dir)/$(SOBJS)


%.o: %.c chant.h module.h matrix.h $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) -c $< 

affine$(SHREXT): $(OFILES)
	@echo $@;$(CC) $(OFILES) -o affine$(SHREXT) $(LDFLAGS) $(LDADD)
