cmake_minimum_required(VERSION 3.10)
project(strutil VERSION 1.0.0 LANGUAGES C)

# Derleme seçenekleri
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build test suite" ON)
option(ENABLE_THREADING "Enable thread safety features" ON)
option(ENABLE_OPTIMIZATION "Enable optimization flags" ON)
option(ENABLE_WARNINGS "Enable extra warning flags" ON)

# Çıktı dizinlerini ayarla
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Kaynak dosyaları
set(STRUTIL_HEADERS
    ${CMAKE_SOURCE_DIR}/src/strutil.h
)

# Derleyici bayrakları
if(ENABLE_OPTIMIZATION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
endif()

if(ENABLE_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# Thread desteği
if(ENABLE_THREADING)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

# Header-only kütüphane için interface target
add_library(strutil INTERFACE)
target_include_directories(strutil INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

if(ENABLE_THREADING)
    target_link_libraries(strutil INTERFACE Threads::Threads)
endif()

# Örnekler
if(BUILD_EXAMPLES)
    add_executable(example ${CMAKE_SOURCE_DIR}/example/example.c)
    target_link_libraries(example PRIVATE strutil)
    
    # Örnek programın çıktı dizinini ayarla
    set_target_properties(example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples
    )
endif()

# Testler
if(BUILD_TESTS)
    enable_testing()
    add_executable(strutil_test ${CMAKE_SOURCE_DIR}/test/strutil_test.c)
    target_link_libraries(strutil_test PRIVATE strutil)
    
    # Test programının çıktı dizinini ayarla
    set_target_properties(strutil_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests
    )
    
    # Test hedefini ekle
    add_test(NAME StrUtilTest COMMAND strutil_test)
endif()

# Kurulum hedefleri
install(TARGETS strutil
    EXPORT strutilTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES ${STRUTIL_HEADERS} DESTINATION include)

# Versiyon bilgisi
message(STATUS "")
message(STATUS "StrUtil Configuration Summary")
message(STATUS "============================")
message(STATUS "Version:          ${PROJECT_VERSION}")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  Threading:      ${ENABLE_THREADING}")
message(STATUS "  Optimization:   ${ENABLE_OPTIMIZATION}")
message(STATUS "  Warnings:       ${ENABLE_WARNINGS}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  Examples:       ${BUILD_EXAMPLES}")
message(STATUS "  Tests:         ${BUILD_TESTS}")
message(STATUS "")
message(STATUS "Paths:")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Binary dir:     ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Library dir:    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "============================")
message(STATUS "") 
