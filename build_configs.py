no_cudd = ['-DUSE_CUDD=FALSE', '-DLIBRARY_SYMBOLIC_ENABLED=FALSE', '-DLIBRARY_SYMBOLIC_SEARCH_ENABLED=FALSE']

release = ["-DCMAKE_BUILD_TYPE=Release"] + no_cudd
debug = ["-DCMAKE_BUILD_TYPE=Debug"] + no_cudd
release_no_lp = ["-DCMAKE_BUILD_TYPE=Release", "-DUSE_LP=NO"]
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_LIBRARIES_BY_DEFAULT=YES"]

DEFAULT = "release"
DEBUG = "debug"
