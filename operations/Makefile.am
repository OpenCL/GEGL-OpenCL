SUBDIRS = \
	affine		\
	blur      	\
	color		\
	core		\
	enhance		\
	io		\
	generated	\
	meta		\
	render		\
	svg		\
	transparency

if ENABLE_WORKSHOP
SUBDIRS+= workshop
endif

EXTRA_DIST= \
	Makefile-operations.am

clean-local:
	rm -f */*$(SHREXT) */*/*$(SHREXT)
