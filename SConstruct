
cppflags = "-std=c++11"
Export('cppflags')

targets=Split("""
    src/SConscript
    tool/SConscript
    test/SConscript
    """)

##VariantDir('mybuild/gtest-1.7.0', 'vendor/gtest-1.7.0', duplicate=0)
#SConscript("vendor/gtest-1.7.0/SConscript")

env = Environment()
build_dir = env.Dir('.').path
cmd = 'mkdir -p mybuild/gtest-1.7.0 ; cd mybuild/gtest-1.7.0 ; pwd ; cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=. ../../vendor/gtest-1.7.0 ; make '
#cmd += '; mkdir include ; cp -r ../../vendor/gtest-1.7.0/include/gtest ./include'
env.Command('libgtest.a', 'vendor/gtest-1.7.0/CMakeLists.txt', cmd)

SConscript("src/SConscript", variant_dir="mybuild/src")
SConscript("tool/SConscript", variant_dir="mybuild/tool")
SConscript("test/SConscript", variant_dir="mybuild/test")

env.Alias('install', '/tmp/foobar')
