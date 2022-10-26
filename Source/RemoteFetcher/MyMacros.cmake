# Append the all C files in the specified directory list to the source list
macro(APPEND_ALL_SOURCE_FILES_IN_DIRLIST result)
    set(filelist "")
    foreach(module ${ARGN})
        list(APPEND filelist ${module}/*.c)
        list(APPEND filelist ${module}/*.cpp)
    endforeach()
    file(GLOB_RECURSE ${result} RELATIVE ${REMOTEFETCHER_DIR} ${filelist})
#    FOREACH(file ${${result}})
#        message(STATUS ${file})
#    ENDFOREACH()
    unset(filelist)
endmacro()

# Helper macro which wraps the generate-message-receiver.py script
#   _output_source is a list name which will contain generated sources.(eg. WebKit_SOURCES)
#   _inputs are messages.in files to generate.
macro(GENERATE_MESSAGE_SOURCES _output_source _inputs)
    unset(_input_files)
    unset(_outputs)
    file(MAKE_DIRECTORY "${Messages_DERIVED_SOURCES_DIR}")
    foreach (_file IN ITEMS ${_inputs})
        get_filename_component(_name ${_file} NAME_WE)
        list(APPEND _input_files ${REMOTEFETCHER_DIR}/${_file}.messages.in)
        list(APPEND _outputs
            ${Messages_DERIVED_SOURCES_DIR}/${_name}MessageReceiver.cpp
            ${Messages_DERIVED_SOURCES_DIR}/${_name}Messages.h
            ${Messages_DERIVED_SOURCES_DIR}/${_name}MessagesReplies.h
        )
        list(APPEND ${_output_source} ${Messages_DERIVED_SOURCES_DIR}/${_name}MessageReceiver.cpp)
    endforeach ()
    list(APPEND ${_output_source} ${Messages_DERIVED_SOURCES_DIR}/MessageNames.cpp)

    set(message_header_file ${REMOTEFETCHER_DIR}/messages/MessageNames.h)
    set(message_source_file ${REMOTEFETCHER_DIR}/messages/MessageNames.cpp)
    add_custom_command(
        OUTPUT
            ${Messages_DERIVED_SOURCES_DIR}/MessageNames.cpp
            ${Messages_DERIVED_SOURCES_DIR}/MessageNames.h
            ${_outputs}
        MAIN_DEPENDENCY ${REMOTEFETCHER_DIR}/scripts/generate-message-receiver.py
        DEPENDS
            ${REMOTEFETCHER_DIR}/scripts/generator/__init__.py
            ${REMOTEFETCHER_DIR}/scripts/generator/messages.py
            ${REMOTEFETCHER_DIR}/scripts/generator/model.py
            ${REMOTEFETCHER_DIR}/scripts/generator/parser.py
            ${_input_files}
            COMMAND ${PYTHON_EXECUTABLE} ${REMOTEFETCHER_DIR}/scripts/generate-message-receiver.py ${REMOTEFETCHER_DIR} ${_inputs}
            COMMAND ${CMAKE_COMMAND} -E copy ${message_header_file} ${Messages_DERIVED_SOURCES_DIR}/
            COMMAND ${CMAKE_COMMAND} -E copy ${message_source_file} ${Messages_DERIVED_SOURCES_DIR}/
        WORKING_DIRECTORY ${Messages_DERIVED_SOURCES_DIR}
        VERBATIM
    )
endmacro()

