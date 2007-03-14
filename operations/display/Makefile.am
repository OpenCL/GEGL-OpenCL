include $(top_srcdir)/operations/Makefile-operations.am

display$(SHREXT): display.c $(GEGLHEADERS)
	@[ ! -z "$(SDL_LIBS)" ] &&\
	(echo $@;$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD) $(SDL_CFLAGS) $(SDL_LIBS)) \
	|| true
