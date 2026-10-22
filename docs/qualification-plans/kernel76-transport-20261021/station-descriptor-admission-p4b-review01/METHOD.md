# Static review method and boundaries

Only public source reading, full-file hashing, text diff, pinned line excerpts
and existing author metadata inspection are selected. No stored script main,
candidate/helper/metadata script import, compiler, generator, executable, shim or
test is invoked. Inline Python uses only the standard library for those static
operations and exclusively creates evidence files in this new review directory.
`COLLECTION_COMMAND.txt` retains the exact successful collection command as text;
it is not executed as a file. Final seal collection is likewise inline metadata.

The reviewer read the full P4b report, actual two-file delta, helper/probe and
retained public USB-core data ownership/count/normalization/probe APIs. Historical
metadata query/parser code was read only as source; no imports, script functions,
parser or main were executed and no private input was read. Its expectations are
not current native evidence. P1 source/partial review is excluded.

An initial read-only metadata-key exploration printed the baseline and inventory
key lists, then raised KeyError on `preserved_p3_blocks`; the actual inventory key
is `preserved_blocks`. No file had been written or candidate code executed. This
was a reviewer metadata assumption, not a task/policy rejection or source defect.
The collection below uses the actual published schema without modifying inputs.

No target/UART/RPC/status/network/private IO, Git/index, original source/helper,
AGENTS/central-plan, settings/model/provider/safeguard change, compiler/generator,
ordinary runtime or penetration/adversarial/fuzz/mutation campaign is performed.
Existing P3/P4a evidence and both hardware HOLDs remain untouched. Input maps
overlap:90 candidate,595 prior custody,817 earlier reader and204 accepted-source
pins are verified separately, never summed as unique inputs.
