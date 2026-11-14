# Original stock firmware and extraction tools

These are original manufacturer-firmware acquisitions from the three owned cameras,
published byte-for-byte at the owner's explicit request on 2026-11-14. They are NOT
working builds of AnykaOpenCam's current native source. No generated native kernel,
UIM, ELF, module or installable project image is included.

| File under `original/` | Camera / stock software | Bytes | Matching independent reads |
| --- | --- | ---: | ---: |
| `t34-4g-km01.bin` | T34-4G / Anyka KM01; AliOS Things | 8388608 | 3 |
| `qc07f.bin` | QC07F / Anyka V200; Linux 3.4.35 | 8388608 | 3 |
| `fulhan-fh8626v200.bin` | Fulhan FH8626V200; Linux 4.9.129 | 8388608 | 3 |

The exterior label T34-4G does not identify an Ingenic T34 Linux platform here.
These dumps cover the 8 MiB flash main array, not OTP/eFuses or separate LTE-modem
storage. They predate the later QC07F ROOT and Fulhan environment bench changes.
Repeated matching reads do not alone prove electrical correctness or restorability.

## Verify and extract

Run `sha256sum -c firmware/original/SHA256SUMS` from the repository root. The
[manifest](original/manifest.json) records exact hashes, acquisition sessions and
the relationship to the retained independent reads. Three canonical dumps total
24 MiB; redundant original reads and complete acquisition mirrors remain local.

See [tools/README.md](tools/README.md) for one extraction command per device,
dependencies and output formats. Normal extraction was verified against all23
original partition hashes; detailed coverage is in [tools/TESTING.md](tools/TESTING.md).
The tools never execute firmware code or access a camera/programmer. They produce
an offline regular-file view, not a bootable filesystem replica. JFFS2 is carved
only. Generated content/manifests can contain secrets and remain ignored/local.

## Intentional unmodified-data warning

No sanitization has been performed. The owner reported that no Wi-Fi network had
been saved on the original firmware and explicitly confirmed public originals
after disclosure of an actual embedded RSA private key shared by the KM01 and
QC07F originals. Those keys remain recoverable from the `.bin` files. Firmware
can also contain factory passwords, device/cloud identifiers and configuration.
This statement is not a guarantee that every proprietary store is free of secrets.
Do not reuse any embedded credential or key, expose these stock services to the
Internet, or mistake factory defaults for the project's credential policy.

Private host keys, operator credentials, newer modified credential-bearing images,
bench UART captures, device photographs and footage are not published separately.
The full `firmware/stock/` mirror is deliberately not uploaded wholesale.

## Rights and safety

Vendor firmware and bundled third-party components retain their original copyrights
and license obligations; inclusion is not a global open-source grant. Corresponding
source completeness is not asserted. The acquisition/extraction documentation is
research evidence, not vendor endorsement or permission to install firmware on an
unmatched device. Do not pad/truncate/repair an original to fit a guessed flash map.
No flashing, erase, unlock, OTP write, automatic homing or update procedure is
provided by these extraction commands.
