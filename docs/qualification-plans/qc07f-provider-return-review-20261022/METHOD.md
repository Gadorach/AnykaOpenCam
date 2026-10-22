# Static review evidence method

Only this directory is writable. Standard read-only jq, sha256sum, stat, cmp,
diff, sed, sort, xxd and ar inspection are used for public metadata/byte evidence.
No candidate, helper, generator, test, compiler or saved review script is executed.
ar is used only with `t`/`p` (list/read member bytes), never create/link/extract.
No current external compiler/header installation or private acquisition is read.
No P1 or query77 path is selected. No target/network operation is performed.

Inputs are selected from the candidate's public sealed app/report/evidence/prior
pin maps. Paths must be workspace-relative apps/rtsp-auth/, apps/qualification/ or
docs/qualification-plans/, and exclude P1/station-attach/transport/query77. Pin maps
are retained as JSON and sha256sum checklists. The source aggregate uses sorted
UTF-8 path + NUL + raw32-byte SHA256. Input size/hash checks are repeated at close.

Dependency metadata is parsed from actual .d files, not guessed from object names.
Each reused object/.d/.su is compared against the original adopted build02 copy;
consumed source/header pins and captured closure bytes are compared to that
build's recorded closure. Every project dependency is matched to the new generated
source. Changed consumers are verified against the new closure. Archive member
bodies are hashed directly from `ar p` and compared with actual .o bodies.

App/generated deltas are independently diffed from retained baseline bytes using
the frozen patch labels. Applied patch ordering/labels follow the read-only builder
metadata. No patch is applied and no overlay is imported or regenerated.

Tool-call commands and their exit statuses are the execution record; retained
metadata reports identify origins/captured copies, command receipts, compile logs,
object dependencies and artifacts. Author compile results are cited, not rerun.
The final review seal pins all fresh report/evidence outputs; its hash is returned
to the parent. There is no automatic subsequent implementation/test/build queue.
