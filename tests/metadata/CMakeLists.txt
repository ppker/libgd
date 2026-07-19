LIST(APPEND TESTS_FILES
	metadata_core
)

add_executable(test_metadata_codec_metadata codec_metadata.c)
target_link_libraries(test_metadata_codec_metadata gdTest gd)
add_test(NAME test_metadata_codec_metadata COMMAND test_metadata_codec_metadata)
if(JPEG_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_JPEG=1)
endif()
if(PNG_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_PNG=1)
endif()
if(WEBP_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_WEBP=1)
endif()
if(AVIF_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_AVIF=1)
endif()
if(HEIF_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_HEIF=1)
endif()
if(JXL_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_JXL=1)
endif()
if(TIFF_FOUND)
	target_compile_definitions(test_metadata_codec_metadata PRIVATE GD_TEST_HAVE_TIFF=1)
endif()

IF(JPEG_FOUND OR PNG_FOUND)
set(HAVE_EXIV2_TESTS OFF)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(EXIV2 QUIET exiv2)
endif()
if(EXIV2_FOUND)
	set(HAVE_EXIV2_TESTS ON)
else()
	find_path(EXIV2_INCLUDE_DIR exiv2/exiv2.hpp
		HINTS ${EXIV2_ROOT}
		PATH_SUFFIXES include
	)
	find_library(EXIV2_LIBRARY
		NAMES exiv2 libexiv2
		HINTS ${EXIV2_ROOT}
		PATH_SUFFIXES lib
	)
	if(EXIV2_INCLUDE_DIR AND EXIV2_LIBRARY)
		set(HAVE_EXIV2_TESTS ON)
		set(EXIV2_INCLUDE_DIRS ${EXIV2_INCLUDE_DIR})
		set(EXIV2_LIBRARIES ${EXIV2_LIBRARY})
	endif()
endif()
if(NOT HAVE_EXIV2_TESTS)
	message(STATUS "exiv2 not found; metadata exiv2 validation tests will be skipped")
endif()
ENDIF()

set(GD_SKIP_PNG_EXIV2_TEST OFF)
if(WIN32)
	set(GD_SKIP_PNG_EXIV2_TEST ON)
	message(STATUS "Skipping PNG exiv2 validation test on Windows")
endif()

function(gd_metadata_link_exiv2_test target)
	target_include_directories(${target} PRIVATE ${EXIV2_INCLUDE_DIRS})
	target_compile_options(${target} PRIVATE ${EXIV2_CFLAGS_OTHER})
	target_link_directories(${target} PRIVATE ${EXIV2_LIBRARY_DIRS})
	target_link_libraries(${target} gdTest ${EXIV2_LIBRARIES})
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
		target_link_libraries(${target} stdc++)
	endif()
endfunction()

IF(JPEG_FOUND)
LIST(APPEND TESTS_FILES
	jpeg_metadata
)
IF(HAVE_EXIV2_TESTS)
add_executable(test_metadata_jpeg_exiv2 jpeg_exiv2.cpp)
gd_metadata_link_exiv2_test(test_metadata_jpeg_exiv2)
add_test(NAME test_metadata_jpeg_exiv2 COMMAND test_metadata_jpeg_exiv2)
SET_PROPERTY(TEST test_metadata_jpeg_exiv2 PROPERTY SKIP_RETURN_CODE 77)
ENDIF(HAVE_EXIV2_TESTS)
ENDIF(JPEG_FOUND)

IF(PNG_FOUND)
LIST(APPEND TESTS_FILES
	png_metadata
)
IF(HAVE_EXIV2_TESTS AND NOT GD_SKIP_PNG_EXIV2_TEST)
add_executable(test_metadata_png_exiv2 png_exiv2.cpp)
gd_metadata_link_exiv2_test(test_metadata_png_exiv2)
add_test(NAME test_metadata_png_exiv2 COMMAND test_metadata_png_exiv2)
SET_PROPERTY(TEST test_metadata_png_exiv2 PROPERTY SKIP_RETURN_CODE 77)
ENDIF(HAVE_EXIV2_TESTS AND NOT GD_SKIP_PNG_EXIV2_TEST)
ENDIF(PNG_FOUND)

ADD_GD_TESTS()
