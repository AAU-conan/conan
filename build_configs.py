compile_commands = ["-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"] # Export compile_commands.json for use with development tools
gcc14 = [
    "-DCMAKE_CXX_COMPILER=g++-14"
]  # Set the C++ compiler to g++-14, for systems with old default compiler

release = ["-DCMAKE_BUILD_TYPE=Release"]  + gcc14
release_nogcc14 = ["-DCMAKE_BUILD_TYPE=Release"]
debugdev = ["-DCMAKE_BUILD_TYPE=Debug"] + compile_commands
debug = ["-DCMAKE_BUILD_TYPE=Debug"] + gcc14
release_no_lp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"] + gcc14
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_LIBRARIES_BY_DEFAULT=YES"]

DEFAULT = "release"
DEBUG = "debug"

