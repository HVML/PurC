add_subdirectory(variant)

PURC_COPY_FILES(SAMPLE_Script
    DESTINATION ${CMAKE_BINARY_DIR}/
    FILES run_all_samples.sh
)
