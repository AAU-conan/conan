# Rust linking
include(FetchContent)

FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG master # Optionally specify a commit hash, version tag or branch here
)
# Set any global configuration variables such as `Rust_TOOLCHAIN` before this line!
FetchContent_MakeAvailable(Corrosion)

# Import targets defined in a package or workspace manifest `Cargo.toml` file
corrosion_import_crate(MANIFEST_PATH rust_test/Cargo.toml)

# Rust libraries must be installed using `corrosion_install`.
corrosion_install(TARGETS rust_lib EXPORT RustLibTargets)

# Installs the main target
install(
        EXPORT RustLibTargets
        NAMESPACE RustLib::
        DESTINATION lib/cmake/RustLib
)

# Necessary for packaging helper commands
include(CMakePackageConfigHelpers)
# Create a file for checking version compatibility
# Optional
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/RustLibConfigVersion.cmake"
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        COMPATIBILITY AnyNewerVersion
)

# Configures the main config file that cmake loads
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/RustLibConfig.cmake"
        INSTALL_DESTINATION lib/cmake/RustLib
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# Install all generated files
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/RustLibConfigVersion.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/RustLibConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/corrosion/RustLibTargetsCorrosion.cmake
        DESTINATION lib/cmake/RustLib
)

corrosion_add_cxxbridge(rust_test_cxx
        CRATE rust_lib
        MANIFEST_PATH rust
        FILES
        lib.rs
        ltl.rs
)

target_link_libraries(downward PUBLIC rust_test_cxx)

if(MSVC)
    # Note: This is required because we use `cxx` which uses `cc` to compile and link C++ code.
    corrosion_set_env_vars(cxxbridge_crate "CFLAGS=-MDd" "CXXFLAGS=-MDd")
endif()