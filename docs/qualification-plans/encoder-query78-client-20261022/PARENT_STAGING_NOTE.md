# Exact frozen patch context retained during local staging

2026-10-22 selection; build/review dates remain their actual Oct23 receipts.
The first isolated-index `git diff --cached --check` returned exit2 on exactly
two space-only **context** lines inside the added frozen unified-diff artifacts:
BUILDER_DELTA.diff line4 and CLIENT_DELTA.diff line5. Each stored file line is one
ASCII space followed by newline, as required to encode an empty context line in
a unified diff. These are not trailing spaces in the client/builder source.

No frozen diff, seal, source or ordinary index was changed to normalize this.
Parent checked the exact two bytes/line locations and checked all other staged
paths separately. The exact diff artifacts remain because review regenerated
and pinned their bytes. This staging diagnostic is not a candidate build/runtime
failure, policy refusal or permission to hide other whitespace findings. No Git
configuration or global whitespace policy was changed.
