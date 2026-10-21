# Flash and image map

No target offsets have been established in the handoff. Fill this using the acquired image and corroborating runtime evidence. Use hexadecimal byte offsets relative to the **full main-array dump**, with an exclusive end offset: `length = end - start`. Preserve each source's own coordinate system and show conversions explicitly.

- Target / session: unknown
- Original dump path and SHA-256: unknown
- Confirmed main-array byte size: unknown
- Bootloader partition evidence: unknown
- Device-tree / bootargs evidence: unknown
- `/proc/mtd` capture path: unknown

| Region | Start | End (exclusive) | Length | Candidate type | Evidence path and location | Confidence / corroboration | SHA-256 if separately extracted |
| --- | --- | --- | --- | --- | --- | --- | --- |
| unknown | unknown | unknown | unknown | unknown | unknown | unconfirmed | unknown |

Suggested regions to look for, without assuming their presence or order: ROM-loader header/prefix, bootloader, primary/redundant environment, DTB/FIT, kernel, root filesystem, application filesystem, persistent configuration, factory data, and erased gaps.

## Image-internal objects

| Object | Absolute offset | Enclosing region | Header/payload length | Checks performed | Interpretation limits |
| --- | --- | --- | --- | --- | --- |
| unknown | unknown | unknown | unknown | unknown | unknown |

Header validity and CRC success do not prove authenticity or target compatibility. Signature locations alone do not prove partitions. Record payload length separately from an MTD partition's allocated size.

## Restore implications

- Boot-critical prefixes, redundant copies and environment layout: unknown
- Erase-unit sizes and alignment, from confirmed part data: unknown
- Mutable device-specific configuration and identifiers: unknown
- Main-array data versus separate OTP/security/fuse state: unknown
- Proposed future write ranges and preservation strategy: pending actual evidence and current user authorization
- Restore procedure and what has actually been tested: unknown
