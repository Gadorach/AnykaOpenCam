# Metadata inspection correction (not a candidate failure/test)

The first post-build inline Python report inspection stopped at its required-API
count assertion. It incorrectly looked for source macro names
`rtsp_provisioner_begin/observe/bind` in nm output. The unchanged real public
header maps these to `qc07f_rtsp_provisioner_{begin,observe,bind}_v1` in production
(and different names in the separately excluded PUBLICDUMMY class).

Read-only inspection confirmed all three actual production names defined once in
each REL. The corrected metadata report uses the real exported names and parses
ELF header fields without fixed padding assumptions. No source/recipe/object was
changed, no compiler/linker was repeated and no candidate/test/function executed.
No prior report file was created/overwritten by the stopped assertion. The tool
transcript retains the failed assertion; it is not a runtime or negative test.
