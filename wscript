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
  conf.check(lib='gif', libpath=['/lib', '/usr/lib', '/usr/local/lib', '/usr/local/libgif/lib', '/usr/local/giflib/lib', '/usr/local/libungif/lib', '/usr/local/pkg/giflib-4.1.6/lib'])

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "gif"
  obj.source = "src/common.cc src/gif_encoder.cc src/gif.cc src/dynamic_gif_stack.cc src/module.cc"
  obj.uselib = "GIF"

def shutdown():
  if Options.commands['clean']:
    if exists('gif.node'): unlink('gif.node')
  else:
    if exists('build/default/gif.node') and not exists('gif.node'):
      symlink('build/default/gif.node', 'gif.node')

