aclocal
automake -a
autoconf
./configure --enable-maintainer-mode $*
