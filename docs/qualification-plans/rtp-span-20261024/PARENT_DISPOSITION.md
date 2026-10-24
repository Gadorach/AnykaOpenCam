# Borrowed-span RTP packetization — parent source/HOST closeout

2026-10-24; RP-09/B07, STREAM-01/03/05, CAP-01/05/07.
Accept **narrow conditional SOURCE/API/bounds/wire-semantics sufficiency** after
independent ordinary review. No source correction required. Cited ONE normal
HOST observation passed; not native RTSP, valid decoder media or cap completion.

The real function builds one RTP/interleaved packet directly from1..16 borrowed
spans, each<=128KiB, exact checked total<=existing1MiB NAL envelope. Descriptor,
sum, session, header and offset refusals precede output writes. No allocator,
whole-NAL temporary, callback, cached iterator or retained pointer. It preserves
the original single-NAL/FU-A wire and offset rules, writing<=1404 bytes. Session
is unchanged; caller commits sequence/offset only after complete transmission.
Valid stable serialized disjoint memory is essential; every future source read
still needs its actual lease. Pending packet independence is not source-release,
END/ACK/drain or DMA synchronization authority.

## Evidence checked, not rerun

- C: `a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e`.
- Header: `41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91`.
- Review: `9df72854face237bbbb4b24f575b044a18c18f83ebe6757b5277e523991ae52b`.

Parent read full production C/header/harness/support/contract/handback and review.
Freshly verified169 overlapping records:17 delivery,14 selected inputs,46 consumed
dependency paths,27 tool/link inputs,54 artifacts and11 pre-call pins, plus20
command logs. See PARENT_VERIFICATION.json. Author3 C compiles/20 commands PASS:
production HOST and retained musl ARMv5TE objects plus HOST harness compile/link.
Production exports only the builder and needs memcpy; no main/allocator. Not full
compiler closure, final ARM linking, native ABI or transitive stack qualification.

ONE normal HOST process packetized174080 synthetic bytes in126 packets, checking
every header/body byte, two inside-payload span crossings and sequence wrap.
Separate65536/65536/43008 source objects, one1404-byte packet; no full-NAL object,
payload logs/hashes, sockets or decoder. No second call/case by parent/reviewer.
Small-NAL/last0/refusal cases are source-reviewed, not executed. Further negative/
adversarial/mutation/fuzz work stays deferred, not passed.

HANDOFF line11's “User-reported” commit wording is a nonblocking provenance error:
the source author received PARENT context. Parent's actual retirement source
commit is `a3d2b75`; no material-retirement operation occurred. Preserve the sealed
handback and this correction rather than rewriting review inputs.

## Integration remains real work

Existing native output malloc/prefault1MiB, copied1MiB hub, contiguous packet/cursor
and old packetizer calls are unchanged. Security75's actual STREAM ioctl requires
output_capacity>=0x100000 and copies to one user output pointer; small views do not
change that backing contract. No cap exception, clipping or lower output limit.
Native raw/output ownership, prefault ledger, release/END/drain/external recovery,
2MiB process and40MiB/CMA/TLS combined load, multi-VCL/RTCP/main remain open.

Next bounded feature is an actual shared chunk-backed CPU hub with<=128KiB
allocations and explicit per-consumer leases, connected to this packetizer in a
normal HOST path before any runtime adoption. It must not release producer/DMA
ownership on chunk metadata alone or free held backing. No old runtime edit,
new diagnostic/build/clock framework or native operation selected by closeout.
Camera last controller OFF after finalized23h; RAM75 recovery/C1 remains blocked.
Source/notices/public closeout metadata may be committed locally; ignored run
objects/ELF/logs remain local evidence. Nothing pushed or published.
