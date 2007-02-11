include $(top_srcdir)/operations/Makefile-operations.am

OFILES      = $(subst $(srcdir)/,,$(CFILES:.c=.o))
EXTRA_DIST += $(wildcard *.h)

SOBJS       = affine.so

%.o: %.c chant.h module.h matrix.h $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) -c $< 

affine.so: $(OFILES)
	@echo $@;$(CC) $(LDFLAGS) $(LDADD) $(OFILES) -o affine.so
