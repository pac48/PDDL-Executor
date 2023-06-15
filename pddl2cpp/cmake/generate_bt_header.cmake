# Copyright 2022 PickNik Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#
#    * Neither the name of the PickNik Inc. nor the names of its
#      contributors may be used to endorse or promote products derived from
#      this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

function(generate_bt_header DOMAIN_FILE LIB_NAME)
  find_program(pddl2cpp_BIN NAMES "pddl2cpp")
  if(NOT pddl2cpp_BIN)
    message(FATAL_ERROR "pddl2cpp() variable 'pddl2cpp_BIN' must not be empty")
  endif()

  # Make the include directory
  set(OUTPUT_FILE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}/include/)
  file(MAKE_DIRECTORY ${OUTPUT_FILE_DIR})

  # Set the domain file parameter to be relative to the current source dir
  set(DOMAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${DOMAIN_FILE})

  # Set the output parameter header file name
  set(HEADER_FILE ${OUTPUT_FILE_DIR}/${LIB_NAME}.hpp)

  message("Running `${pddl2cpp_BIN} ${DOMAIN_FILE} ${HEADER_FILE}`")

  # Generate the header for the library
  add_custom_command(
    OUTPUT ${HEADER_FILE}
    COMMAND ${pddl2cpp_BIN} ${DOMAIN_FILE} ${HEADER_FILE}
    DEPENDS ${DOMAIN_FILE}
    COMMENT
    "Running `${pddl2cpp_BIN} ${DOMAIN_FILE} ${HEADER_FILE}`"
    VERBATIM
  )
#  add_custom_target(${LIB_NAME} DEPENDS ${HEADER_FILE})
  add_library(${LIB_NAME} INTERFACE ${HEADER_FILE})
  target_include_directories(${LIB_NAME} INTERFACE
      $<BUILD_INTERFACE:${OUTPUT_FILE_DIR}>
      $<INSTALL_INTERFACE:include/${LIB_NAME}>
      )
  target_include_directories(${LIB_NAME} INTERFACE ${pddl_parser_INCLUDE_DIRS})
  target_link_libraries(${LIB_NAME} INTERFACE pddl_parser::pddl_parser)

  install(DIRECTORY ${OUTPUT_FILE_DIR} DESTINATION include/${LIB_NAME})
endfunction()
