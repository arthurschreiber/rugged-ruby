require_relative 'lib/rugged/ruby/version'
VERSION = Rugged::Ruby::Version

Gem::Specification.new do |s|
  s.name                  = "rugged-ruby"
  s.version               = VERSION
  s.date                  = Time.now.strftime('%Y-%m-%d')
  s.summary               = ""
  s.homepage              = ""
  s.email                 = "schreiber.arthur@gmail.com"
  s.authors               = [ "Arthur Schreiber" ]
  s.license               = "MIT"
  s.files                 = %w( README.md LICENSE )
  s.files                 += Dir.glob("lib/**/*.rb")
  s.files                 += Dir.glob("ext/**/*.[ch]")
  s.files                 += Dir.glob("vendor/libgit2/cmake/**/*")
  s.files                 += Dir.glob("vendor/libgit2/{include,src,deps}/**/*")
  s.files                 += Dir.glob("vendor/libgit2/{CMakeLists.txt,Makefile.embed,AUTHORS,COPYING,libgit2.pc.in}")
  s.extensions            = ['ext/rugged-ruby/extconf.rb']
  s.required_ruby_version = '>= 1.9.3'
  s.description           = <<desc
Define Rugged backends with Ruby.
desc
  s.add_development_dependency "rake-compiler", ">= 0.9.0"
  s.add_development_dependency "minitest", "~> 3.0"
end
