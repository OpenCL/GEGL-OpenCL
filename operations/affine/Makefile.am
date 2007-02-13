include $(top_srcdir)/operations/Makefile-operations.am

OFILES      = $(subst $(srcdir)/,,$(CFILES:.c=.o))
EXTRA_DIST += $(wildcard *.h)

SOBJS       = affine.$(SOsuf)

%.o: %.c chant.h module.h matrix.h $(GEGLHEADERS)
	@echo $@;$(CC) $(CFLAGS) -c $< 

affine.$(SOsuf): $(OFILES)
	@echo $@;$(CC) $(OFILES) -o affine.$(SOsuf) $(LDFLAGS) $(LDADD)
