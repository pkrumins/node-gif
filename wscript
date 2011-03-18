import Options
from os import unlink, symlink, popen
from os.path import exists

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")

  if not conf.check_cxx(lib='gif', libpath=['/lib', '/usr/lib', '/usr/local/lib', '/usr/local/libgif/lib', '/usr/local/giflib/lib', '/usr/local/libungif/lib', '/usr/local/pkg/giflib-4.1.6/lib']):
    conf.fatal("Missing libgif library from giflib package")
  if not conf.check_cxx(header_name='gif_lib.h'):
    conf.fatal("Missing gif_lib.h header from giflib-devel package")

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "gif"
  obj.source = "src/common.cpp src/utils.cpp src/palette.cpp src/quantize.cpp src/gif_encoder.cpp src/gif.cpp src/dynamic_gif_stack.cpp src/animated_gif.cpp src/async_animated_gif.cpp src/module.cpp src/buffer_compat.cpp"
  obj.uselib = "GIF"
  obj.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]

def shutdown():
  if Options.commands['clean']:
    if exists('gif.node'): unlink('gif.node')
  else:
    if exists('build/default/gif.node') and not exists('gif.node'):
      symlink('build/default/gif.node', 'gif.node')

