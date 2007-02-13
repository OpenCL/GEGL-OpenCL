=begin
extconf.rb for Ruby/Gegl extention library
=end

PACKAGE_NAME = "gegl"
PACKAGE_ID   = "gegl"


# uncomment these if building within ruby-gnome2

#TOPDIR = File.expand_path(File.dirname(__FILE__) + '/..')
#MKMF_GNOME2_DIR = TOPDIR + '/glib/src/lib'
#$LOAD_PATH.unshift MKMF_GNOME2_DIR
#SRCDIR = TOPDIR + '/rgegl/src'
#add_depend_package("glib2", "glib/src", TOPDIR)
#
SRCDIR = File.expand_path(File.dirname(__FILE__) + '/src')

require 'mkmf-gnome2'

#
# detect GTK+ configurations
#

PKGConfig.have_package(PACKAGE_ID) or exit 1
PKGConfig.have_package("gtk+-2.0") or exit 1
setup_win32(PACKAGE_NAME)

have_func('gegl_node_process')

make_version_header("GEGL", PACKAGE_ID)

create_makefile_at_srcdir(PACKAGE_NAME, SRCDIR, "-DRUBY_GEGL2_COMPILATION")

create_top_makefile
