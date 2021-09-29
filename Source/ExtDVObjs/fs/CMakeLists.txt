set(FS_LIBRARY_TYPE SHARED)

set(FS_PUBLIC_HEADERS                                                          
)

set(FS_SOURCES                                                                 
    fs.c                                                                   
)                                                                               
                                                                                
set(FS_PRIVATE_INCLUDE_DIRECTORIES                                             
    "${CMAKE_BINARY_DIR}"
    "${FORWARDING_HEADERS_DIR}"
    "${PURC_DIR}"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${PURC_DIR}/include"
    "${PURC_DIR}/include/private"
)                                                                               
                                                                                
set(FS_LIBRARIES                                                               
    ${CMAKE_DL_LIBS}                                                            
)                            

link_directories (${CMAKE_BINARY_DIR}/Source/PurC)
list(APPEND FS_LIBRARIES purc)
                                                                                
set(FS_INTERFACE_LIBRARIES FS)                                                
set(FS_INTERFACE_INCLUDE_DIRECTORIES ${FS_FRAMEWORK_HEADERS_DIR})             
set(FS_INTERFACE_DEPENDENCIES FS_CopyHeaders)
                                                                                
PURC_FRAMEWORK_DECLARE(FS)                                                     
PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()

PURC_COPY_FILES(FS_CopyHeaders                                                 
    DESTINATION ${FS_FRAMEWORK_HEADERS_DIR}/wtf                                
    FILES ${FS_PUBLIC_HEADERS}                                                 
)   

set_target_properties(FS PROPERTIES OUTPUT_NAME "purc-dvobj-FS")

PURC_WRAP_SOURCELIST(${FS_SOURCES})                                            
PURC_FRAMEWORK(FS)                                                             
PURC_FRAMEWORK_TARGET(FS)                                                      
                                                                                
install(TARGETS FS DESTINATION "${EXT_DVOBJ_LIB_INSTALL_DIR}/purc-${EXT_DVOBJ_VERSION}")

if (MSVC)                                                                       
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")  
endif ()                                                               
