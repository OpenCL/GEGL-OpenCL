CFILES = $(wildcard *.c)
#CFILES = comp.c  # to just run individual test..

bins   = $(subst ,,$(CFILES:.c=))

all: $(bins)

%: %.c $(GEGLHEADERS)
	$(CC) `pkg-config gegl --cflags --libs` -Wall -O2  -o $@ $<

check: $(bins)
	for a in $(bins); do ./$$a;done

clean:
	rm -f $(bins)

