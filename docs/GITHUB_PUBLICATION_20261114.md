# GitHub publication scope — 2026-11-14

Approved destination: <https://github.com/Gadorach/AnykaOpenCam>, **public**.
The owner requested project source, original `.bin` firmware acquisitions and
extraction tools for each camera, excluding a working built current-native image.
After disclosure of the actual shared factory RSA private key in KM01 and QC07F,
the owner explicitly chose **Keep originals public**. No sanitization is claimed.

## Included

- Curated reviewed source/history, source/license notices and development records.
- Exactly three original8MiB dump copies under `firmware/original/`, one per device,
  byte-identical to each device's three retained original reads and mirror copies.
- Whole-dump SHA256SUMS/provenance/warnings, plus standalone offline per-device
  partition/container/filesystem extraction tools and documented host dependencies.

## Excluded

- Every working built current-native image/kernel/package/ELF/module and build tree.
- Later modified credential-bearing dumps, private host/operator keys and provisioning.
- Whole local acquisition mirrors, generated extracts/manifests, UART captures,
  photographs, footage and unrelated unreviewed/staged local work.
- Extracted factory private-key/password files as separate repository files; their
  embedded bytes remain intentionally present in the owner-approved original dumps.

## Checks performed before upload

All nine original8MiB reads and all nine mirror copies matched their acquisition
hashes. The three public copies were streamed into fresh files and reread/hash-
verified without altering originals. Parent reran each documented extractor against
the published filenames into protected temporary directories; all three completed.
Detailed normal extraction coverage is in `firmware/tools/TESTING.md`.

Reachable pre-publication Git history was audited:3095 blobs/101174691 bytes, no
ELF/uImage/SquashFS binary payload, native-image filename, photograph or media file.
Heuristic key/token/PSK findings were source comments/expressions and an explicit
uppercase parent-supplied private-passphrase marker, not key material/live tokens.
This is a bounded heuristic audit, not proof of every possible secret's absence.
Known factory key findings in original firmware were disclosed, not suppressed.
Final staged-tree checks must limit `.bin` blobs to the exact three approved hashes
and reject generated native-image/build artifacts before the push.

The existing ordinary staging index is user work and must remain byte-identical;
curation uses a temporary index initialized from HEAD. No blanket add/reset/cleanup,
history rewrite, force push, remote visibility change or hardware operation is
authorized by this file. Firmware/source retain their original license obligations;
neither ownership of the cameras nor publication assigns a global firmware license.
