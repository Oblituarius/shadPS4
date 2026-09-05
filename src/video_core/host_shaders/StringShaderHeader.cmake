# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(SOURCE_FILE ${CMAKE_ARGV3})
set(HEADER_FILE ${CMAKE_ARGV4})
set(INPUT_FILE ${CMAKE_ARGV5})

get_filename_component(CONTENTS_NAME ${SOURCE_FILE} NAME)
string(REPLACE "." "_" CONTENTS_NAME ${CONTENTS_NAME})
string(TOUPPER ${CONTENTS_NAME} CONTENTS_NAME)

# Function to recursively parse #include directives and replace them with file contents
function(parse_includes file_path output_content)
    file(READ ${file_path} file_content)
    # This regex includes \n at the begin to (hackish) avoid including comments
    string(REGEX MATCHALL "\n#include +\"[^\"]+\"" includes "${file_content}")

    set(parsed_content "${file_content}")
    foreach (include_match ${includes})
        string(REGEX MATCH "\"([^\"]+)\"" _ "${include_match}")
        set(include_file ${CMAKE_MATCH_1})
        get_filename_component(include_full_path "${file_path}" DIRECTORY)
        set(include_full_path "${include_full_path}/${include_file}")

        if (NOT EXISTS "${include_full_path}")
            message(FATAL_ERROR "Included file not found: ${include_full_path} from ${file_path}")
        endif ()

        parse_includes("${include_full_path}" sub_content)
        string(REPLACE "${include_match}" "\n${sub_content}" parsed_content "${parsed_content}")
    endforeach ()
    set(${output_content} "${parsed_content}" PARENT_SCOPE)
endfunction()

parse_includes("${SOURCE_FILE}" CONTENTS)

# D1-PRESERVATION FORK-LOCAL BUILD FIX - NOT AN UPSTREAM FIX
# MSVC limits a C++ literal to 65535 bytes (C2026); split >32 KiB sources into 16 KiB raw string chunks, always wrapped.
string(LENGTH "${CONTENTS}" _contents_len)
if (_contents_len GREATER 32768)
    set(CHUNK_SIZE 16384)
    # compute ceil(_contents_len / CHUNK_SIZE) portably: divide, then
    # add 1 if there's any remainder.
    math(EXPR _num_chunks "${_contents_len} / ${CHUNK_SIZE}")
    math(EXPR _remainder "${_contents_len} % ${CHUNK_SIZE}")
    if (_remainder GREATER 0)
        math(EXPR _num_chunks "${_num_chunks} + 1")
    endif()
    set(_chunks "")
    foreach(i RANGE 0 ${_num_chunks})
        math(EXPR _start "${i} * ${CHUNK_SIZE}")
        if (_start LESS _contents_len)
            string(SUBSTRING "${CONTENTS}" ${_start} ${CHUNK_SIZE} _chunk)
            if (i EQUAL 0)
                set(_chunks "R\"shader_src(${_chunk})shader_src\"")
            else()
                set(_chunks "${_chunks} R\"shader_src(${_chunk})shader_src\"")
            endif()
        endif()
    endforeach()
    set(CONTENTS "${_chunks}")
else()
    # Small shader: wrap the content in a single R"shader_src(...)" literal
    # so the template's bare @CONTENTS@ substitution is still a valid
    # string literal. Without this wrapper the GLSL `#version`, `//`,
    # and other C++-incompatible syntax leaks into the C++ TU and trips
    # C1021 ("invalid preprocessor command 'version'") and friends.
    set(CONTENTS "R\"shader_src(${CONTENTS})shader_src\"")
endif()

get_filename_component(OUTPUT_DIR ${HEADER_FILE} DIRECTORY)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
configure_file(${INPUT_FILE} ${HEADER_FILE} @ONLY)
