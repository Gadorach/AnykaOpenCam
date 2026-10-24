# Real authenticated span server — contract before implementation

2026-10-24. RP-09/B07; bounded STREAM-01/02/03/07, CAP-01/05/07 source connection.
Only the two assigned fresh directories are writable. No execution of any
camera/server/backend/callback/test, hardware, Git/index, or frozen builder.
Independent ordinary review and parent adoption are mandatory before use.

## Selected derivative

Copy the actual `qc07f-owned-runtime/server/rtsp_server_candidate.c` body into
one new C unit, with a new public source header/export. Reuse original real
candidate/auth/prestarted/listener/owned-runtime headers and implementations,
accepted `span-hub-v2` ONLY and accepted `rtp-span-candidate`. No substitute
backend, old-hub cast, changed candidate_config, old retention callback with
NULL/fake hub, serve-time fork/init/destroy, or synthetic provider.

The caller supplies a persistent BORROWED actual v2 hub, freshly initialized
AFTER the last verifier fork and required native ownership ordering. It is ACTIVE
and pristine: no publication/generation/tokens/subscribers/pending/acquired/history;
eight canonical disjoint nonnull chunk ranges and matching allocation history.
These checks are shape/refusal checks, NOT proof of true allocation, provenance,
fork ordering, prefault, residency, global memory fit or lifetime. Trusted caller
holds hub/chunks, prestarted owner, config, source/runtime, external media-process
owner and typed retention context through all refusals, return and HOLD. No hub
init/dispose/allocation/reinit/free occurs in the server. Reject nonnull legacy
options.hub_storage. Preserve one slot/1MiB NAL, fixed native profile, loopback,
original deadline/control/SSRC/channel/auth/source/owned-listener contracts.

Configure an explicit stable typed sink before entry, accepting the actual new
hub, exact demand and consumer index (4 denotes whole-server/source uncertainty).
It must retain the entire process/stack/cursors and external source owner; not
merely copy pointers. No sink implementation/noop/default is provided. Local
uncertainty latches source HOLD and enters a nonreturning path: invoke typed sink,
existing owned-runtime cleanup (which can itself quarantine), then repeated
SIGSTOP if the sink returns. Preserve native cleanup before any stack/FD cleanup.
No exit/abort/free/FD close/END retry on that path. This is process-local retention,
NOT recovery, survival against process death or external-owner implementation.
Original retain_verifier and frame notification remain real and unchanged.

## Data and lease path

Actual producer contiguous packet -> at most eight borrowed <=128KiB input spans
-> actual v2 publish/shared CPU copy -> actual producer release -> exact common
END -> four independent v2 consumer-tagged views/tokens -> span hub packet wrapper
-> accepted span RTP packetizer -> one 1404-byte pending packet per peer. No
source pointer survives release. Contiguous packetizer is used only for original
bounded SPS/PPS (<=256B combined). No full-NAL concatenation/per-peer NAL copy.
Fresh auth, exact consumer lease and clock/deadline checks precede each packet
build AND each partial-send continuation. Commit sequence/offset only on complete
packet write; failures close that connection rather than skip an incomplete AU.
Release exact v2 token/generation; failed release retains the still-active cursor
before clearing it. All views finish before next source call. STOP and genuine
drained/zero-token checks precede clean return. Borrowed backing is never disposed.

## Evidence and explicit gaps

Offline HOST + retained-musl-ARM production compilation only. Inspect retained
owned-runtime build02 roles/defines/object pins; real REL composition may reuse
archive members EXCLUDING the old server and old packet hub, with actual new hub
and span packetizer. No old combined REL/whole-archive, final ELF or stub closure.
Record exact argv/tools/dependencies/artifacts/logs and source delta in fresh
refuse-overwrite outputs. No runtime/negative/adversarial/mutation/fuzz testing.

Native use is BLOCKED. Existing nh_native_memory_fixed_buffers(void *hub) and
paid[1] accept one contiguous1048576-byte extent, not these chunks. Existing
producer output may still malloc1MiB; raw/output allocation cap, real chunk
prefault/ledger, process2MiB/combined-load/backing/external-owner/recovery/main,
native ABI/image fit, generalized multi-NAL AU and asynchronous capture remain
OPEN. No ready Boolean, callback success or API shape closes any of these gates.
