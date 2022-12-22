add_subdirectory(PurC)
add_subdirectory(CSSEng)
add_subdirectory(DOMRuler)

PURC_COPY_FILES(ScriptToRunSamples
    DESTINATION ${CMAKE_BINARY_DIR}/
    FILES run_all_samples.sh
)

set(sample_HVML_FILES
    "hvml/hello.hvml"
    "hvml/hello-except.hvml"
    "hvml/您好.hvml"
    "hvml/hello-10.hvml"
    "hvml/hello-html.hvml"
    "hvml/hello-html-timer.hvml"
    "hvml/hello-html-timer-1.hvml"
    "hvml/hello-html-timer-2.hvml"
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
    "hvml/hello-world-d.hvml"
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
    "hvml/sys-sleep.hvml"
    "hvml/prime-number-sieve.hvml"
    "hvml/fetch-with-param-0.hvml"
    "hvml/fetch-with-param-1.hvml"
    "hvml/fetch-with-param-2.hvml"
    "hvml/fetch-with-param-3.hvml"
    "hvml/fetch-with-param-4.hvml"
    "hvml/fetch-with-param-5.hvml"
    "hvml/fetch-with-param-6.hvml"
    "hvml/fetch-with-param-7.hvml"
    "hvml/fetch-with-param-8.hvml"
    "hvml/fetch-with-param-9.hvml"
    "hvml/fetch-with-param-a.hvml"
    "hvml/fetch-with-param-b.hvml"
    "hvml/foil-layouts.hvml"
    "hvml/assets/foil-layouts-displayBlock.css"
    "hvml/assets/foil-layouts-quotes.css"
    "hvml/assets/foil-layouts-counters.css"
    "hvml/assets/foil-layouts-layeredBoxes.css"
    "hvml/assets/messages-foil-layouts-zh.json"
)

PURC_COPY_FILES(SampleHVMLFiles
    DESTINATION "${CMAKE_BINARY_DIR}"
    FILES ${sample_HVML_FILES}
)

