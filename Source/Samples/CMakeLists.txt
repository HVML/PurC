add_subdirectory(api)
#add_subdirectory(fetcher)

PURC_COPY_FILES(ScriptToRunSamples
    DESTINATION ${CMAKE_BINARY_DIR}/
    FILES run_all_samples.sh
)

set(sample_HVML_FILES
    "hvml/hello.hvml"
    "hvml/hello-10.hvml"
    "hvml/hello-html.hvml"
    "hvml/hello-html-sleep.hvml"
    "hvml/fibonacci-void-temp.hvml"
    "hvml/calculator-bc.hvml"
    "hvml/planetary-resonance.hvml"
)

PURC_COPY_FILES(SampleHVMLFiles
    DESTINATION "${CMAKE_BINARY_DIR}"
    FILES ${sample_HVML_FILES}
)

