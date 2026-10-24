# Span hub v2 — focused R1 source correction handback

2026-10-24; RP-09/B07, STREAM-01/02/03, CAP-01/05/07.
**Author correction delivered; four production HOST/ARM compiles and two actual
integration REL links PASS. NOT RUN. Independent ordinary review and separate
parent disposition are required before source adoption.** No automatic queue.

## Exact scope and correction

Only fresh `apps/rtsp-auth/span-hub-v2/` and
`docs/qualification-plans/span-hub-v2-20261024/` were written. CONTRACT preceded
source. Frozen V1 source/evidence and its independent R1 review remain unchanged:
FROZEN_INPUTS.json and BUILD_INSPECTION.json verify 113 public pin records and
exact frozen-directory file membership/bytes, including the accepted packetizer
C/header and disposition. No Git/index/AGENTS/central-plan, runtime/hub/encoder,
accepted dependency, private/current-state or hardware change. No subworkers.

Minimal R1 mechanism:

- New `issued_token` counter is separate from publication generation. A successful
  acquire, after all prior validation and checked exhaustion preflight, issues
  counter+1, tags the view with its actual consumer and stores that nonzero token
  in canonical `active_token[consumer]`. Only successful acquire advances it.
- View validation binds owner + actual consumer tag + that consumer's canonical
  active token, retaining all old generation/metadata/span checks. Scalar release
  independently requires that consumer's canonical token and current generation;
  it clears the canonical token BEFORE clearing acquired/pending bits.
- No other consumer's unchanged view/token can satisfy those consumer-specific
  checks. STOP leaves held tokens intact and exact old releases accepted. No
  subscribe/unsubscribe/resync/backlog/publication/STOP path resets or reassigns
  held tokens; the issued counter never resets, including at disposal. Counter
  and publication-generation exhaustion refuse before relevant state/output
  changes, without wrap/reuse. These are SOURCE arguments, not executed cases.
- Existing drained bits imply all canonical slots zero by the acquire/release
  invariant; unpublished INIT_FAILED also has no issued/active tokens. No
  disposal-policy change or fake permission flag was added. Caller still owns
  one true-fresh, one-use, stable-address trusted owner and serialization/valid
  disjoint objects/no aliases/moves/reentry/signals/concurrency/fork. Tokens are
  local identities, not authorization across owners/processes/cameras.

Production C delta: **7 added / 4 removed lines**, confined to acquire, release
and lease validation. Header: **16 added / 4 removed lines**, small fields plus
accurate contract/replacement comments. V1_TO_V2.diff and V2_TO_V1.diff are exact;
SOURCE_DELTA.json records successful forward/inverse text application in memory.
The complete C prefix before acquire and suffix from packet wrapper are byte-
identical: allocation/input/copy/publication/wrapper/STOP/drain/known-CPU disposal
are untouched. Eight separate128KiB requests and one shared copied NAL remain;
no cap increase or native source/DMA/END authority. GPL notices are retained.

**Exclusive source replacement only.** V1 export/type/header-guard names remain,
but changed owner/view layouts are NOT V1 binary ABI compatible. Recompile all
consumers with this header; never link both hubs, reuse old combined RELs, or
include both headers in the same translation unit/guard context. No V1
host_normal.c/host_once.py was copied into v2; no new harness exists.

## Compile/link evidence, not a runtime test

From `/home/gadorach/Desktop/AnykaOpenCam`, one invocation only:

```text
/usr/bin/python3 -B apps/rtsp-auth/span-hub-v2/compile01.py
```

Actual tool interval: 2026-10-24 03:48:16.303829–03:48:17.165157 UTC.
Fresh refuse-overwrite build01; no failure/retry/source correction after build.
HOST GCC16.2.1 and retained musl ARM GCC14.3.0; ARMv5TE/ARM/soft-float flags.
The fixed recipe executes **20 compiler/linker/metadata commands**, all exit0:
four warning-as-error production C compiles (new hub + actual unchanged accepted
span packetizer per architecture), two direct `ld -r` links and compiler/readelf/
nm metadata only. All four compile logs are empty. No executable was built/run,
no old builder/helper/program/test was invoked. Compiler temporaries stayed in
the fresh build tree. All executed tools matched retained V1 tool hashes.

`runs/build01/REPORT.json` contains exact argv/cwd/explicit environment/start/end/
returncode/log records and artifacts. Author read/hash inspection independently
rechecked 19 selected inputs,42 consumed dependency paths,12 tools,59 artifacts,
20 command logs and113 frozen pins (overlapping records, not complete compiler
closure). Link traces contain exactly the new hub and accepted packetizer
objects. REL symbols resolve `camera_rtp_h264_spans_packet_v1`; no main or old
packetizer symbol. HOST is ELF64 REL; ARM is ELF32 REL/EABI5/v5TE, no program
headers. Real malloc/free and ordinary libc/compiler support remain unresolved
in the REL, as expected. This is not an executable link, native ABI/loadability,
transitive-stack, residency/prefault, whole-app allocation or complete cap proof.

## Frozen normal observation and open gates

V1's retained ONE normal HOST PASS (174080B synthetic NAL, four correct cursors,
504 packets, exact releases/STOP/drain/dispose) remains normal correct-cursor
evidence ONLY. Independent review found V1's R1 ownership/API defect; overall V1
source ownership remains unaccepted. Nothing here relabels that review or runs
an R1 failure test. **V2 has zero runtime/functional observations.** No normal
rerun, wrong-consumer/refusal/negative/adversarial/mutation/fuzz test was selected.

Old runtime/provider/hub/encoder,1MiB native output/contiguous-copy/raw ownership,
prefault ledger, source release/END/drain/external retention/recovery,2MiB process
and40MiB/CMA/TLS/headroom qualification remain untouched/open. Generic AU/RTCP,
snapshot/audio/recorder policy and main/native integration are not introduced.
No sockets/threads/fork from production code, RNG/provisioner/verifier/credential/
retirement call or device/native/hardware operation occurred. Parent's last OFF
after23h and blocked RAM75 recovery/C1 remain context, not new observations.

Next handback action: parent-selected independent ordinary source/evidence review
of this focused correction, without a test/program rerun or automatic adoption.

## Exact SHA-256 anchors

| File | SHA-256 |
| --- | --- |
| v2 span_hub.c | `bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1` |
| v2 span_hub.h | `310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454` |
| accepted rtp_h264_spans.c (unchanged) | `a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` |
| accepted rtp_h264_spans.h (unchanged) | `41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91` |
| runs/build01/REPORT.json | `609b7588a343f7814ef75e3587150afc787d7a7a401d594a887a7d34a968d169` |
| HOST production REL | `3760bf130a175a215e0e9223920d529e88b39578d0dc7789d48bb18c227e6dff` |
| ARM production REL | `2637896af43fc03bb6b9cf8328d5959376099135a6382aa70b5381bc4d87288a` |

DELIVERY_PINS.json anchors new source/public docs and the compile reports; ignored
runs retain exact local object/log evidence, not publication or release approval.
