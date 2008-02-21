include $(top_srcdir)/operations/Makefile-operations-old.am

OFILES      = $(subst $(srcdir)/,,$(CFILES:.c=.o))

SOBJS          = affine$(SHREXT)
INSTALLED_ITEMS = $(ext_dir)/$(SOBJS)


%.o: %.c chant.h module.h matrix.h $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) -c $< 

affine$(SHREXT): $(OFILES)
	@echo $@;$(CC) $(OFILES) -o affine$(SHREXT) $(LDFLAGS) $(LDADD)
