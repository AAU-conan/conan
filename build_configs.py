no_cudd = [
    "-DUSE_CUDD=FALSE",
    "-DLIBRARY_SYMBOLIC_ENABLED=FALSE",
    "-DLIBRARY_SYMBOLIC_SEARCH_ENABLED=FALSE",
]
gcc14 = [
    "-DCMAKE_CXX_COMPILER=g++-14"
]  # Set the C++ compiler to g++-14, for systems with old default compiler

release = ["-DCMAKE_BUILD_TYPE=Release"] + no_cudd + gcc14
release_nogcc14 = ["-DCMAKE_BUILD_TYPE=Release"] + no_cudd
debug = ["-DCMAKE_BUILD_TYPE=Debug"] + no_cudd + gcc14
release_no_lp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_LIBRARIES_BY_DEFAULT=YES"]

DEFAULT = "release"
DEBUG = "debug"
