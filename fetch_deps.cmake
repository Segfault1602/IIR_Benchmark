include(FetchContent)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE)

FetchContent_Declare(
    libsndfile
    GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
    GIT_TAG 1.2.2)

set(BUILD_PROGRAMS
    OFF
    CACHE BOOL "Don't build libsndfile programs!")
set(BUILD_EXAMPLES
    OFF
    CACHE BOOL "Don't build libsndfile examples!")
set(BUILD_REGTEST
    OFF
    CACHE BOOL "Don't build libsndfile regtest!")
set(BUILD_TESTING
    OFF
    CACHE BOOL "Don't build libsndfile tests!" FORCE)
set(BUILD_PROGRAMS
    OFF
    CACHE BOOL "Don't build libsndfile programs!" FORCE)
set(ENABLE_EXTERNAL_LIBS
    OFF
    CACHE BOOL "Disable external libs support!" FORCE)
set(BUILD_TESTING
    OFF
    CACHE BOOL "Disable libsndfile tests!" FORCE)

FetchContent_Declare(
    kfr
    GIT_REPOSITORY https://github.com/kfrlib/kfr.git
    GIT_TAG 6.2.0
)

FetchContent_MakeAvailable(nanobench doctest libsndfile kfr)
