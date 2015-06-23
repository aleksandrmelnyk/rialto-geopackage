# usage:
#   to build: $ scons [debug=1] pdal_prefix=FOO install_prefix=BAR
#   to install:  $ scons [debug=1] pdal_prefix=FOO install_prefix=BAR install
#   to clean build:  $ scons [debug=1] pdal_prefix=FOO install_prefix=BAR -c
#   to clean install:  $ scons [debug=1] pdal_prefix=FOO install_prefix=BAR -c install

env = Environment()

debug = ARGUMENTS.get('debug', 0)
if int(debug):
    env.Append(CCFLAGS = '-g')
else:
    env.Append(CCFLAGS = '-O3')

install_prefix = ARGUMENTS.get('install_prefix', "/tmp/rialtoinstall")

pdal_prefix = ARGUMENTS.get('pdal_prefix', "/tmp/pdalinstall")

env.Append(CXXFLAGS = "-std=c++11")

env["pdal_prefix"] = pdal_prefix
Export('env', 'install_prefix')

SConscript("src/SConscript", variant_dir="mybuild/src")
SConscript("tool/SConscript", variant_dir="mybuild/tool")
SConscript("vendor/gtest-SConscript", variant_dir="mybuild/vendor/gtest-1.7.0")
SConscript("test/SConscript", variant_dir="mybuild/test")

env.Alias('install', install_prefix)
