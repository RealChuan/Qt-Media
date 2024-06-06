#!/usr/bin/env ruby

require "fileutils"
require "open3"
require "shellwords"

include FileUtils::Verbose

def safe_system(*args)
  puts args.shelljoin
  system(*args) || abort("Fail to run the last command!")
end

# 定义一个类 Dep，它有两个属性 dest 和 src
class Dependent
  attr_accessor :dest, :src

  # 定义一个初始化方法，接受两个参数
  def initialize(dest, src)
    @dest = dest
    @src = src
  end
end

class DylibFile
  OTOOL_RX = /\t(.*) \(compatibility version (?:\d+\.)*\d+, current version (?:\d+\.)*\d+\)/

  attr_reader :path, :id, :deps

  def initialize(path)
    @path = path
    parse_otool_L_output!
  end

  def parse_otool_L_output!
    stdout, stderr, status = Open3.capture3("otool -L \"#{path}\"")
    abort(stderr) unless status.success?
    libs = stdout.split("\n")
    libs.shift # first line is the filename
    @id = libs.shift[OTOOL_RX, 1]
    @deps = libs.map { |lib| lib[OTOOL_RX, 1] }.compact
  end

  def ensure_writeable
    saved_perms = nil
    unless File.writable_real?(path)
      saved_perms = File.stat(path).mode
      FileUtils.chmod 0644, path
    end
    yield
  ensure
    FileUtils.chmod saved_perms, path if saved_perms
  end

  def change_id!
    ensure_writeable do
      safe_system "install_name_tool", "-id", "@rpath/#{File.basename(self.id)}", path
    end
  end

  def change_install_name!(old_name, new_name)
    ensure_writeable do
      safe_system "install_name_tool", "-change", old_name, new_name, path
    end
  end
end

if ARGV.length <= 1
  abort <<~END
    Usage: change_lib_dependencies.rb prefix libraries...

    If you're using Homebrew, your invocation might look like this:
      $ ./change_lib_dependencies.rb "$(brew --prefix)" "$(brew --prefix mpv-iina)/lib/libmpv.dylib"

    If you're using MacPorts, your invocation might look like this:
      $ port contents mpv | grep '\.dylib$' | xargs ./change_lib_dependencies.rb /opt/local
  END
end

prefix = ARGV.shift

linked_files = ARGV

proj_path = File.expand_path(File.join(File.dirname(__FILE__), '../'))
lib_folder = File.join(proj_path, "macos/lib/")

deps = []

rm_rf lib_folder
mkdir lib_folder

linked_files.each do |file|
  # Grab the actual library on disk.
  file = File.realpath(file)

  # Keep the output filename the same as the library's install name
  dylib = DylibFile.new file
  dylib.parse_otool_L_output!
  dest = File.join(lib_folder, File.basename(dylib.id))

  puts "cp -p #{file} #{dest}"
  copy_entry file, dest, preserve: true
  deps << Dependent.new(dest, file)
end

fix_count = 0

while !deps.empty?
  dependent = deps.pop
  file = dependent.dest
  loader_path = File.dirname(dependent.src)
  puts "loader_path: #{loader_path}"
  puts "=== Fix dependencies for #{file} ==="
  dylib = DylibFile.new file
  dylib.change_id!
  dylib.deps.each do |dep|
    old_dep = dep
    if dep.start_with?("@loader_path")
      dep = dep.gsub("@loader_path", loader_path)
    end
    # 如果是软连接，需要获取真实路径
    if File.symlink?(dep)
      dep = File.realpath(dep)
    end
    # puts "dep: #{dep}"

    if dep.start_with?(prefix)
      fix_count += 1
      basename = File.basename(dep)
      new_name = "@rpath/#{basename}"
      dylib.change_install_name!(old_dep, new_name)
      dest = File.join(lib_folder, basename)
      # 检查是否已经存在
      unless File.exist?(dest)
        copy_entry dep, dest, preserve: true
        deps << Dependent.new(dest, dep)
      end
    end
  end
end

puts "Total #{fix_count}"