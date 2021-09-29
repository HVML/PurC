include(PurCCommon)
set(MATH_LIBRARY_TYPE SHARED)

set(MATH_PUBLIC_HEADERS                                                          
)

set(MATH_SOURCES                                                                 
    math.c                                                                   
)                                                                               
                                                                                
set(math_PARSERS math)
PURC_FLEX_BISON(MATH parsers "${math_PARSERS}")
unset(math_PARSERS)

set(MATH_PRIVATE_INCLUDE_DIRECTORIES                                             
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)                                                                               
                                                                                
set(MATH_LIBRARIES                                                               
    ${CMAKE_DL_LIBS}                                                            
)                            

link_directories (${CMAKE_BINARY_DIR}/Source/PurC)
list(APPEND MATH_LIBRARIES purc m)
                                                                                
set(MATH_INTERFACE_LIBRARIES MATH)                                                
set(MATH_INTERFACE_INCLUDE_DIRECTORIES ${MATH_FRAMEWORK_HEADERS_DIR})             
set(MATH_INTERFACE_DEPENDENCIES MATH_CopyHeaders)
                                                                                
PURC_FRAMEWORK_DECLARE(MATH)                                                     
PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()

PURC_COPY_FILES(MATH_CopyHeaders                                                 
    DESTINATION ${MATH_FRAMEWORK_HEADERS_DIR}/wtf                                
    FILES ${MATH_PUBLIC_HEADERS}                                                 
)   

set_target_properties(MATH PROPERTIES OUTPUT_NAME "purc-dvobj-MATH")

PURC_WRAP_SOURCELIST(${MATH_SOURCES})                                            
PURC_FRAMEWORK(MATH)                                                             
PURC_FRAMEWORK_TARGET(MATH)                                                      
                                                                                
install(TARGETS MATH DESTINATION "${EXT_DVOBJ_LIB_INSTALL_DIR}/purc-${EXT_DVOBJ_VERSION}")

if (MSVC)                                                                       
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")  
endif ()                                                               
