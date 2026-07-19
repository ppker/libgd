message(STATUS "Configuring Doxygen documentation target")

find_package(Doxygen)

if(DOXYGEN_FOUND)
  # Allow relocating the project Doxyfile into docs/ while keeping root fallback.
  set(DOXYGEN_CONFIG_FILE "")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
    set(DOXYGEN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Doxyfile")
    set(DOXYGEN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../Doxyfile")
  endif()

  if(DOXYGEN_CONFIG_FILE STREQUAL "")
    message(WARNING "Doxygen found, but no Doxyfile was found in docs/ or project root. Skipping docs target.")
  else()
    # Keep docs output build-dir relative (build/docs/api) instead of a fixed path.
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/api")
    set(DOXYGEN_CONFIG_FILE_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cmake")
    set(DOXYGEN_MAINPAGE_FILE "${CMAKE_CURRENT_BINARY_DIR}/preamble.md")

    file(MAKE_DIRECTORY "${DOXYGEN_OUTPUT_DIR}")

    # Generate versioned main page content from docs/preamble.md.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/preamble.md" DOXYGEN_MAINPAGE_CONTENT)
    string(REPLACE "@VERSION@" "${GDLIB_VERSION}" DOXYGEN_MAINPAGE_CONTENT "${DOXYGEN_MAINPAGE_CONTENT}")
    # Convert the old NaturalDocs-style title into a Markdown heading for Doxygen.
    string(REGEX REPLACE "^Title:[ \t]*" "# " DOXYGEN_MAINPAGE_CONTENT "${DOXYGEN_MAINPAGE_CONTENT}")
    file(WRITE "${DOXYGEN_MAINPAGE_FILE}" "${DOXYGEN_MAINPAGE_CONTENT}")

    file(READ "${DOXYGEN_CONFIG_FILE}" DOXYGEN_CONFIG_CONTENT)
    string(REGEX REPLACE "OUTPUT_DIRECTORY[ \t]*=[^\n\r]*"
                         "OUTPUT_DIRECTORY       = ${DOXYGEN_OUTPUT_DIR}"
                         DOXYGEN_CONFIG_CONTENT
                         "${DOXYGEN_CONFIG_CONTENT}")
    string(REGEX REPLACE "PROJECT_NUMBER[ \t]*=[^\n\r]*"
                         "PROJECT_NUMBER         = \"${GDLIB_VERSION}\""
                         DOXYGEN_CONFIG_CONTENT
                         "${DOXYGEN_CONFIG_CONTENT}")
    string(REGEX REPLACE "USE_MDFILE_AS_MAINPAGE[ \t]*=[^\n\r]*"
                         "USE_MDFILE_AS_MAINPAGE = ${DOXYGEN_MAINPAGE_FILE}"
                         DOXYGEN_CONFIG_CONTENT
                         "${DOXYGEN_CONFIG_CONTENT}")
    string(REGEX REPLACE "INPUT[ \t]*=[^\n\r]*"
                         "INPUT                  = src ${DOXYGEN_MAINPAGE_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/metadata.md"
                         DOXYGEN_CONFIG_CONTENT
                         "${DOXYGEN_CONFIG_CONTENT}")
    file(WRITE "${DOXYGEN_CONFIG_FILE_OUT}" "${DOXYGEN_CONFIG_CONTENT}")

    add_custom_target(docs
      COMMAND "${DOXYGEN_EXECUTABLE}" "${DOXYGEN_CONFIG_FILE_OUT}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )

    # Include generated API HTML in install/release artifacts when present.
    if(INSTALL_API_DOCS)
      install(DIRECTORY "${DOXYGEN_OUTPUT_DIR}/html"
              DESTINATION "share/doc/gd-${GDLIB_MAJOR}.${GDLIB_MINOR}/api"
              OPTIONAL)
    endif()
  endif()
else()
  message(STATUS "SKIPPING generation of API documentation (Doxygen not found)")
endif()
