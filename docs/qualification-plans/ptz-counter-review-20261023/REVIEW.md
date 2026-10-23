# CTRL-02 — independent ordinary SOURCE review

2026-10-23. **Narrow conditional SOURCE/API/counter-capacity sufficiency;
no correction required within this scope.** This accepts the five added source
lines only, not deployment, native behavior, mechanical qualification or full
CTRL-02 completion. Parent alone owns hardware; this review selects no build or
native follow-up.

## Source and counter proof

Reviewed all160 lines of `bringup/qc07f/ptz-counter-candidate/ptz.c`, the contract,
exact delta, saved compile command, compile report/logs and relevant consumed
type/limits headers. Read-only local artifact hashing/comparison reconstructed
the baseline in memory; no candidate was imported, compiled, executed or tested.
No target, GPIO, serial, network, private-evidence or Git/index access occurred;
no original source was modified. Only this fresh review document was written.

- Lines42–48 parse into `int`, reject zero and values outside[-4,4], then take
  `abs`. Consequently `steps` is1..4; `INT_MIN` and negative subtraction operands
  cannot reach the new guard.
- The consumed `linux/limits.h:21` defines `U32_MAX` as `((u32)~0U)`;
  `asm-generic/int-ll64.h` and its UAPI header resolve `u32` to `unsigned int`.
  On this ARM baseline, the usual arithmetic conversions convert positive
  `steps` to unsigned int. `U32_MAX - steps` is exactly the nonwrapping unsigned
  value M-s, where M=4294967295 and1<=s<=4. The unsigned comparison against
  `issued_steps` therefore has the intended meaning, not signed-overflow behavior.
- The global mutex is acquired at49 before checking capacity at50–53 and stays
  held through the final addition at104 and unlock at106. Both axes use this
  same lock; the counter has no other mutation in this file apart from initial
  zero allocation. `state_show` also uses the lock.
- For an admitted request, C<=M-s. `completed` starts at0, increments once only
  after all four phase reads succeed, and its loop has at most s iterations.
  Every ordinary early/partial completion path therefore has0<=k<=s. No counter
  mutation intervenes under the lock, so C+k<=C+s<=M: the final addition cannot
  wrap. Equality C=M-s is correctly admitted; the guard conservatively reserves
  the full requested capacity even when a later operation might complete less.
- Refusal unlocks and returns `-EOVERFLOW` before status resets, GPIO acquisition,
  phase changes or the common final-result/addition path. It preserves counter,
  phase, `last_result`, both readback flags and existing GPIO state. This is a
  direct kernel/sysfs check, not reliance on a userspace CLI guard.
- Exact reverse reconstruction and unified-diff comparison confirm only the
  explicit limits include plus the four-line guard were added. Four-step cap,
  signed request interface,20ms delay, break-before-make, phase sequence, OFF/
  readback/release paths and state layout remain unchanged. The intended API
  change is capacity refusal without replacing last-actuation facts.

This is an ordinary valid-object/control-flow capacity proof, including partial
completion; it is not a hostile-memory, lifetime or fault-injection proof.

## Retained compile evidence, not a new build

All4 reported input pins,394 source/header dependency pins,3 artifact pins,
3 log pins and the compiler-file pin match current local bytes. The dependency
file has394 unique paths, exactly the report's dependency set, and targets the
fresh object. The original query77 full-tree `ptz.c` is byte-identical to the
security75 source below. The original saved-command and object pins also match.

The report records three successful commands: Clang version, one real changed
driver ARM-object compile, and readelf inspection. The compile log is empty;
the version log says23.1.3, and `/usr/bin/clang` resolves to the pinned
`/usr/bin/clang-23`. The ELF log describes ELF32 little-endian ARM REL, EABI5,
ARMv5TE; direct object-header bytes agree with its class/data/type/machine/flags.

Exact argv comparison confirms unchanged code-generation/include/define flags
and working directory. Precisely, in addition to source/output redirection and
explicit `/usr/bin/clang`, dependency emission changed from
`-Wp,-MMD,arch/arm/mach-ak3918ev200/.ptz.o.d` to `-MD -MF <fresh ptz.d>`.
Thus “only paths changed” is shorthand, not a literal argv identity claim;
this dependency-only option difference does not invalidate the narrow compile
evidence. No original command was replayed by this reviewer.

This is NEW coherent query77 source/generated-header/Clang23.1.3 object evidence,
not old75 compiler/ABI reproduction, native pairing, linking, Kbuild, module,
kernel/image integration or adoption. No target object/program was executed.

## Exact SHA-256 pins

Paths abbreviated only in this table: `Q` =
`docs/qualification-plans/ptz-counter-20261023`; `R` = `Q/runs/compile01`;
`B` = `work/lightbulb-camera/foundation-20261011T041148Z/analysis/encoder-query77-build02/build/arch/arm/mach-ak3918ev200`.

| Input/artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `bringup/qc07f/variants/security75/mach-ak3918ev200/ptz.c` / reconstructed baseline | 5114 | `c70829527a939bf3533ec0270fba1f8db4186fb3dc61ff1ba506e1900cda50b6` |
| `bringup/qc07f/ptz-counter-candidate/ptz.c` | 5248 | `b4a30cc783e7af5b2b0cb8eb53dc34ff24c80f159cd3c8bfe59db9abb11d24a0` |
| `Q/CONTRACT.md` | 1775 | `25cf69c98d0813779aea378de000d487ceb773d37def683063ca146366d32c12` |
| `Q/SOURCE_DELTA.patch` | 637 | `b66b80958f96676629e8270c13a20bf1dbd8a575059bd041816da9eaf3412a71` |
| `R/REPORT.json` | 117142 | `0ef505b53ef6aca08ff15a42c186071cd45ff97ff5ce5b27abd775e08b761942` |
| `R/compiler-version.txt` | 92 | `515e73398b96d9755cd8e88a2f977a4ac68160148e23fc5f5a63c1290c6576eb` |
| `R/compile.txt` | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `R/ELF.txt` | 1466 | `e79b8a4c2a7a63c64b1df73a73e595b251fb9a6d0fc95944c5a236627b26e952` |
| `B/.ptz.o.cmd` | 86971 | `bd8878e556d5d5299bc6f30febb1c5b7f7a06e8daa925fef0ca361574141d8e0` |
| `B/ptz.o` (original NEW77 object) | 6552 | `7a23811e9ce3e01a169d6fa68910a371b412695f478a7c152d273c59612a70e0` |
| `R/ptz.o` | 6596 | `aa9b242fc4f43a43d8593669707487fcc6fd1da6713842c2118033103868b9b1` |
| `R/ptz.d` | 61161 | `6ea5618c8276c11b1a168fe93de0b07ad06212994c86059ff11ff9953cd40c8b` |
| `/usr/bin/clang-23` | 124752 | `583017067c40b0ff0a9b35805d8ac89d5a66fb014221c78ae13a7c21997dd464` |

## Explicit remaining limits

Native counter behavior/timing, actual movement/sign/travel, recovery and product
acceptance remain unqualified. Existing descriptor release despite failed OFF
readback, lifetime and hang/recovery deficiencies remain unfixed. Preserving
prior status on refusal does not establish current coil safety. This fix creates
no quarantine, STOP/lifetime guarantee, counter-to-position mapping or authority
to raise the four-step cap. Original75/76/77 drivers/images remain separate and
were not modified or adopted by this review. No further test/build/hardware work
is automatically queued.
