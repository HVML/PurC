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
    "hvml/hello-html-timer.hvml"
    "hvml/hello-world-0.hvml"
    "hvml/hello-world-1.hvml"
    "hvml/hello-world-2.hvml"
    "hvml/hello-world-3.hvml"
    "hvml/hello-world-4.hvml"
    "hvml/hello-world-5.hvml"
    "hvml/hello-world-6.hvml"
    "hvml/hello-world-7.hvml"
    "hvml/hello-world-8.hvml"
    "hvml/hello-world-9.hvml"
    "hvml/hello-world-a.hvml"
    "hvml/hello-world-b.hvml"
    "hvml/hello-world-c.hvml"
    "hvml/hello-world.json"
    "hvml/fibonacci-void-temp.hvml"
    "hvml/fibonacci-html-temp.hvml"
    "hvml/fibonacci-html-temp-rdr.hvml"
    "hvml/calculator-bc.hvml"
    "hvml/planetary-resonance-lines.hvml"
    "hvml/planetary-resonance.hvml"
    "hvml/greatest-common-divisor.hvml"
    "hvml/execute-in-place-void.hvml"
    "hvml/execute-in-place-html.hvml"
    "hvml/load-string-hvml.hvml"
    "hvml/load-another-body.hvml"
    "hvml/call-concurrently.hvml"
)

PURC_COPY_FILES(SampleHVMLFiles
    DESTINATION "${CMAKE_BINARY_DIR}"
    FILES ${sample_HVML_FILES}
)

