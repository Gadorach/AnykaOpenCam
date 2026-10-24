# RP-11 / CTRL-02 — independent ordinary PTZ OFF-retain review

2026-10-24. **Narrow conditional SOURCE/API/bounds/ownership/lifetime
sufficiency; no source correction required within this scope.** This is not
adoption, hardware readiness, a physical STOP/deadline, thermal safety, mechanical
qualification, zeroing or native GO. Parent disposition is required before use.

Reviewed all235 candidate C lines, all160 accepted counter-baseline lines, the
contract/handback/reasoning/build result, both literal patches, manifests, actual
saved argv/dependencies/logs/object metadata and the retained kernel contracts
below. Only this fresh review directory is written. No compiler, linker, test,
helper/project import, candidate function, GPIO/motor, device/private evidence,
UART/network/outlet or Git/index operation was performed. Inline host work only
read/hashed/compared public artifacts and interpreted saved metadata/patch text.

## Exact scope and custody

- Baseline `bringup/qc07f/ptz-counter-candidate/ptz.c`:5248B, SHA256
  `b4a30cc783e7af5b2b0cb8eb53dc34ff24c80f159cd3c8bfe59db9abb11d24a0`
  (accepted counter-only source increment8ec07ff).
- Candidate `bringup/qc07f/ptz-off-retain-candidate/ptz.c`:7551B, SHA256
  `a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`.
- Independently checked421 SOURCE_INPUTS records,6 ADDITIONAL_INPUTS records,
  15 delivery pins,394 compile-dependency records and2 tool pins: all match.
  These sets overlap; they are not independent unique-file totals. Author docs
  directory membership is exactly15 files including DELIVERY itself; its15
  payload pins additionally include the candidate outside that directory.
- Both regenerated unified patches exactly equal saved bytes; literal inverse
  application in memory reconstructs the accepted baseline. Four-line counter
  guard and tail beginning `static DEVICE_ATTR_RO(state);` are byte-identical.
  Supplemental reference pins/check details are in `VERIFICATION.json`.

## Findings in the actual candidate

1. **Real terminal owner, not a flag after put (19–32,69–88,155–181).** Both axes
   share one mutex and one static owner. Non-NULL original `gpio_descs *` rejects
   every subsequent valid pulse before status/counter mutation, added references
   or GPIO acquisition. The HOLD branch transfers the actual array, consumer
   device and successful self-reference obligation plus scalar axis/masks/error/
   completed issued count/phase; it bypasses array/device/module puts. It stores
   no devm `ak_ptz *`, callback-local address or cleanup pointer. No probe/remove/
   reset path clears it. Consumer unbind/rebind can replace ordinary devm counters
   but cannot clear this static owner or its separately visible fault history.
2. **Reference entry/exit is paired (78–98,168–182).** Self-reference precedes
   consumer `get_device`, both precede acquisition/effects. Failed self get returns
   without GPIO/history effects; failed device get drops the successful self ref.
   Acquisition error drops both added refs, with partial acquisition cleanup
   owned by gpiolib. Wrong count releases without direction/output attempts under
   the restricted ASIS/provider contract below. Verified final cleanup releases
   array then device then self, after logging/private-state accesses. HOLD keeps
   all three obligations; this intentional indefinite retention is not a leak
   on the verified normal path. Built-in THIS_MODULE=NULL is legitimate.
3. **Possibly effectful failed direction is covered (100–107,133–167).** Attempted
   bit is set BEFORE each direction call; initialized bit only AFTER0. Final
   cleanup issues LOW to every attempted line, including the failed call's pin,
   then requires setter0 AND direction0 AND logical read0 for each. Any failure,
   input direction or nonzero/unknown read makes retention sticky. Unattempted
   ASIS lines receive neither cleanup setters nor direction reconfiguration.
   No direction retry is disguised as cleanup. A failed direction that leaves
   gpiolib's output flag unset can make setter return EPERM; that retains safely
   rather than treating a later LOW read alone as release authority.
4. **First error and partial progress remain truthful (39–50,108–174).** The
   per-phase OFF setter failure aborts that phase, and a phase setter failure
   immediately enters final cleanup. LOW helper still attempts all selected
   lines while keeping its first error. Cleanup cannot overwrite an existing
   actuation error. Actual retained gpiolib normalizes invalid positive provider
   setter/direction/config returns to EBADE; negative errors propagate. Positive
   direction/read mismatches become EIO, not success. `completed` advances only
   after all four phase reads match; phase advances only then. Partial phase
   output is not counted as a completed/readback-confirmed iteration, much less
   physical displacement. Full `coils_off_verified` requires both masks15 and
   every final predicate. Verified partial attempted-subset release never claims
   untouched lines OFF. Full OFF may be true after an earlier actuation error;
   it denotes final logical observations, not overall command success. Logging
   snapshots that flag under the lock, avoiding a later pulse replacing it.
5. **Original bounds preserved (62–77,108–130,171,202–235).** Parsed nonzero
   +/-4 becomes1..4 before capacity arithmetic. Under the same mutex, C<=M-s
   and0<=completed<=s imply both retained C+completed and final addition <=U32_MAX.
   The original whole-request reservation remains exact. Zero-initialized phase
   starts0 and only changes modulo4. Board5/four-line/axis0..1 checks, phase
   bytes3,6,12,9,20ms sleep,1–1.5ms break and phase advancement are unchanged.
   These are requested delays, not wall-time bounds. Probe/state request or write
   no GPIO. No homing/zero assignment, wider motion API, broker, timer or clock/
   receipt machinery was added. Status remains scalar/unknown physical position.

## Actual Linux6.18.55 contracts and conditions

Here `K` is
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux`.
These are source contracts, not observed target backing.

- `K/drivers/gpio/gpiolib.c:2432–2538,853–865,1399–1413,5007–5122,5170–5178`:
  exclusive requests pin provider module and gpio_device/descriptor storage;
  heap array remains caller-owned. Put releases requests/refs and frees the
  array, not an electrical OFF action. Keeping the actual array retains these
  storage obligations. `gpiolib.h:223–242` supplies SRCU chip guards.
- `gpiolib.c:2657–2750,2793–2807,2885–3065,3286–3305,3631–3642,3850–3860,
  4212–4227,4321–4327`: modern setter returns int; output may have effects before
  failure. Direction0 is output. Raw get, direction and setter reject absent
  provider chip with ENODEV (or setter EPERM for unset output flag); unsupported
  direction is an error, not cached LOW evidence. Logical reads apply ACTIVE_LOW.
  Generic open-drain direction at453–455 can report output while physically
  input: the direction proof is NOT portable to arbitrary providers/lookups.
- `K/arch/arm/mach-ak3918ev200/gpio.c:48–145,147–170` is the intended direct
  provider: request only reads mux; no free callback; direction reads the actual
  direction register; get reads input register; setter writes latch; output
  writes latch then direction. Unsupported persistence/drive config returns
  ENOTSUPP without writes. `gpiolib.c:2694–2714,3200–3212,4630–4697,4814–4872`
  shows ASIS still requests/configures persistence/flags and may internally put
  partial arrays: acquisition is NOT generally effect-free. Wrong-count/error
  release is justified only for this inert request/config/free path and fixed
  lookup, not arbitrary GPIO acquisition. Prototype DTS107–119 supplies the two
  unchanged board5 pin groups with flags0; nodes are disabled. There was no DT
  activation. Admission must establish exact direct provider/native-GPIO mux,
  unchanged active-high push-pull lookup, no open-drain/source/bias substitution,
  no other privileged reconfiguration and continuing functional power/clock/MMIO
  backing. The driver does not enforce all those native preconditions.
- `K/drivers/base/core.c:3858–3881` holds the device kobject, NOT driver binding
  or devm PTZ memory. `core.c:2919–2956`, `devres.c:496–535`,
  `dd.c:607–631,1322–1441` establish detach/devres cleanup: later-added sysfs
  group is removed/drained before earlier devm PTZ allocation is freed and
  drvdata cleared. HOLD has no reference to that allocation. Remaining per-device
  accesses occur only inside active callbacks, not later retained-owner work.
- **Normal final module_put return is protected by the real exit chain**, not
  self-refcount folklore. `include/linux/module.h:686–725` explicitly permits
  sysfs self-get under kernfs implied protection. `kernel/module/main.c:729–756,
  828–847,907–933` rejects ordinary unload with retained refs; put itself only
  decrements. After the normal final put, an unload may begin, but
  `include/linux/platform_device.h:281–288` / `device/driver.h:257–267` generate
  exit→platform_driver_unregister→driver_unregister→bus_remove_driver→driver_detach
  (`drivers/base/platform.c:874–877`, `driver.c:270–278`, `bus.c:781–797`).
  That synchronously reaches the devm group drain above before module free.
  `fs/kernfs/file.c:311–357` encloses the complete store/return in an active ref;
  `fs/sysfs/file.c:137–147`, `drivers/base/core.c:2495–2509` dispatch to this
  callback. `fs/sysfs/group.c:20–32,304–327` removes its files;
  `fs/kernfs/dir.c:489–527,1472–1535` deactivates/drains them. No active-protection
  break/self-removal exists here. Concurrent consumer unbind cannot finish
  freeing PTZ or removing its driver-list entry before this drain. Thus no
  post-unload callback return is inferred merely from module_put's position.
- **Provider unbind remains outside the guarantee.** `gpiolib.c:1263–1299`
  actually nulls chip/removes backing despite its comment about requested lines.
  `drivers/gpio/gpiolib-devres.c:326–362` invokes this on provider unbind; the
  provider's own devm state/MMIO can then retire. Consumer self/device and GPIO
  request refs preserve consumer code/device and descriptor storage, not live
  provider binding or electrical function. Suspend/rail loss/forced unload and
  other privileged writers are not prevented. No false LOW fallback was found
  in the inspected absent/unsupported-provider paths; continuing electrical
  validity during suspend is nevertheless unproved and requires admission.

## Saved compile evidence only

Saved compile01 records exactly3 successful commands: Clang version23.1.3, ONE
actual candidate-C ARM-object compile, readelf header/attributes. Compile log is
empty. The106-element compile argv is identical to the prior counter argv
except dependency-output index3, object-output104 and actual source105. Emitted
.d names exactly the394 report dependencies and the fresh object/source, not
the counter C. Current candidate matches its consumed dependency pin. Original
query77 C remains byte-identical to security75, not replaced by this object.

Coherent query77 source/generated headers/.config identify6.18.55 and NEW
Clang23.1.3; PTZ=y, MODULES=y, MODULE_UNLOAD=y, FORCE_UNLOAD unset. No MODULE define
was added: `include/linux/init.h:172–177` makes THIS_MODULE NULL. Consequently
this is honestly **built-in object scope**, not a modular-branch compile or module
ABI/runtime result. Saved ELF attributes say ARMv5TE/ARM/Thumb-1; direct raw
header inspection agrees with ELF32 little-endian ARM REL/EABI5/no program
headers. Object8580B SHA256
`6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`.
No tool was rerun. No link/module/kernel/image build, full compiler/transitive
toolchain closure, old75 ABI reproduction, native pairing or deployment is proved.
No failed historical evidence is superseded; no new execution is selected.

## Remaining gates / parent boundary

Retained ownership can leave energized coils indefinitely; bank HOLD does NOT
deenergize, impose current/thermal protection or acknowledge physical STOP.
Mutex/I/O/delays may block indefinitely; OFF cannot preempt a hung operation.
Finite product STOP/queue revocation, provider lifecycle control, physical output/
current/temperature, movement/sign/limits, defensible origin/zeroing, exact native
pairing and rollback/independent recovery remain open. Stock homing is still
NO-GO; no guessed limits or automatic home. Historical23h OFF supplies no fresh
state/lease, RAM-entry recovery/C1 stays blocked, Wi-Fi P1 restrictions unchanged.
Speaker testing remains deferred. This review creates no source-fix/test matrix,
self-adoption, next worker queue or hardware phase.
