SUBDIRS = \
	core

subdirs =\
	blur      \
	color     \
	core      \
	display   \
	file-io   \
	generated \
	render    \
	transform \
	transparency
all:
	(cd generated;./math.rb)
	(cd generated;./porter-duff.rb)
	for dir in $(subdirs);do make -C $$dir;done
install:
	for dir in $(subdirs);do make -C $$dir install ;done
clean:
	for dir in $(subdirs);do make -C $$dir clean ;done
	-rm generated/*.c *~

