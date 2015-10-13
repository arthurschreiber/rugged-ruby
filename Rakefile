require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('rugged-ruby') do |r|
  r.name = 'rugged_ruby'
  r.ext_dir = 'ext/rugged-ruby'
  r.lib_dir = 'lib/rugged'
end

Rake::TestTask.new do |t|
  t.libs << 'lib' << 'test'
  t.pattern = 'test/**/*_test.rb'
  t.verbose = false
  t.warning = true
end
