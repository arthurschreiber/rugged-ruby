require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

$CFLAGS << " #{ENV["CFLAGS"]}"
$CFLAGS << " -g"
$CFLAGS << " -O3" unless $CFLAGS[/-O\d/]
$CFLAGS << " -Wall -Wno-comment"

if !(MAKE = find_executable('gmake') || find_executable('make'))
  abort "ERROR: GNU make is required to build rugged-ruby."
end

CWD = File.expand_path(File.dirname(__FILE__))
LIBGIT2_DIR = File.join(CWD, '..', '..', 'vendor', 'libgit2')

# Prepend the vendored libgit2 build dir to the $DEFLIBPATH.
#
# By default, $DEFLIBPATH includes $(libpath), which usually points
# to something like /usr/lib for system ruby versions (not those
# installed through rbenv or rvm).
#
# This was causing system-wide libgit2 installations to be preferred
# over of our vendored libgit2 version when building rugged.
#
# By putting the path to the vendored libgit2 library at the front of
# $DEFLIBPATH, we can ensure that our bundled version is always used.
dir_config('git2', "#{LIBGIT2_DIR}")

create_makefile("rugged/rugged_ruby")
