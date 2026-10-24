# Author reasoning — descriptor, device and code lifetime

This is ordinary SOURCE reasoning for the fresh235-line derivative, not an
independent review or runtime result. Source hash:
`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`.
`K` below is the retained full kernel at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux/`.
Its Makefile identifies6.18.55. Exact file pins are in SOURCE_INPUTS.json;
additional reference-only inputs are pinned in ADDITIONAL_INPUTS.json.

## Actual kernel contracts inspected

| Actual retained source | Consequence for this derivative |
| --- | --- |
| `K/drivers/gpio/gpiolib.c:2432–2488,2496–2538` | Successful exclusive request sets REQUESTED, pins the provider module and gpio_device. put clears request/descriptor flags, calls provider free if present, drops those refs. It is not an OFF operation. |
| Same file:853–865,1399–1413,5007–5122,5170–5178 | gpio_descs is a real heap array, separately owned by the caller. Descriptors reside in gpio_device storage freed at its final release. Array put frees the array and releases its descriptors. Keeping the exact original array without put retains those requests/storage refs; a numeric pin list would not. |
| Same file:2901–3065 | Direction-output may configure drive/bias and invoke a provider before returning failure. Failure does not certify no output effect. Mark attempted BEFORE the call, not after success. |
| Same file:432–462,4212–4227,4321–4327 | Direction0 means output; negative direction/read errors are not LOW. Logical getters account for ACTIVE_LOW; modern setters return an error that the baseline ignored. |
| `K/arch/arm/mach-ak3918ev200/gpio.c:48–64,66–110,113–145` | Intended provider request reads mux only; get_direction reads its actual direction register, get reads input, set changes the latch, direction_output changes latch then direction. Unsupported persistence/drive configuration returns ENOTSUPP without writes. |
| `K/drivers/gpio/gpiolib.c:4814–4872,5007–5033` | GPIOD_ASIS skips direction configuration, but still requests/configures flags/persistence, and array acquisition can internally put partial requests. Thus wrong-count/acquisition-failure release is justified by the inspected provider/lookup contract, not a claim that all providers' ASIS acquisition is physically inert. |
| `K/drivers/base/core.c:3858–3881` | get_device holds the actual consumer kobject/device, not its devm private state or binding. put_device releases this added reference on the normal path. |
| `K/include/linux/module.h:686–725`, `K/kernel/module/main.c:729–756,907–933` | sysfs active protection explicitly permits try_module_get at callback entry. Successful self pin prevents ordinary unload while retained; failure returns before GPIO acquisition. module_put decrements, does not synchronously execute module exit. Forced removal is outside this proof. |
| `K/include/linux/init.h:172–177` | Built-in THIS_MODULE is NULL; code/global storage are non-unloadable and module ref operations legitimately do nothing. This compile is built-in scope, NOT modular-runtime validation. |
| `K/drivers/base/core.c:2919–2956`, `K/drivers/base/devres.c:496–535`, `K/drivers/base/dd.c:607–631` | The devm sysfs group is registered after devm ptz allocation; reverse devres release removes/drains that group before freeing ptz on consumer detach. Consumer get_device alone would NOT preserve ptz. The retained owner deliberately contains no ptz pointer. |
| `K/fs/kernfs/file.c:311–357`, `K/fs/sysfs/file.c:137–147`, `K/fs/kernfs/dir.c:489–527` | Active protection encloses the store call; removal drains active callbacks. This protects callback code/private-state use through return, including normal logging and final self put. There is no deliberate active-reference break or self-removal path here. |
| `K/drivers/gpio/gpiolib.c:1263–1299` | Although the comment forbids removal with requested GPIOs, the implementation can numb chip=NULL and proceed. Neither consumer get_device nor provider module pin prevents explicit provider unbind. Retained descriptor storage is not an operational provider lease. |

Direction proof is conditional on the intended direct, push-pull provider/lookup,
not a general GPIO-provider guarantee. `bringup/qc07f/qc07f-prototype.dts:107–119`
records board5 groups with flags0; its nodes remain disabled. Generic gpiolib has
an open-drain emulation exception at453–455 which can report logical output while
physically input. Do NOT adopt this reasoning with modified/open-drain lookup,
another provider, a guessed DT or without actual pairing/admission. No DT/image
was changed, built, enabled or queried here.

## Actual derivative ownership/control flow

- Lines69–88: both axes use the same static mutex/owner. A non-NULL retained
  array rejects a valid future pulse with EIO before counter/status mutation or
  GPIO/ref acquisition. The exact accepted four-line counter guard is unchanged.
  Self pin precedes device pin and acquisition; either refusal is no-effect.
- Lines100–107: attempted records each possibly effectful direction call before
  invoking it; initialized records successful returns separately. Failed direction
  never advances the loop to configure unattempted descriptors.
- Lines39–50,111–129: LOW helper preserves its first setter error while attempting
  LOW on all selected lines. Break-before-make and phase writes now propagate
  modern setter errors into the one existing terminal cleanup path; they do not
  continue issuing motion after a reported failure. No timer/phase/pin expansion.
- Lines133–154: one bounded final cleanup sets only attempted descriptors LOW.
  It does not retry direction_output, touch unattempted ASIS lines, or infer
  output from a failed direction call. Every attempted line must return setter0,
  direction0 AND logical read0; any failed predicate makes known_low false.
  The first existing actuation error wins; otherwise the first cleanup error
  wins. Partial initialization always leaves the full coils_off_verified false,
  even if the attempted subset is subsequently verified LOW.
- Lines155–180: uncertain final cleanup publishes the original array plus actual
  device/self-module ref ownership into one static owner while holding the mutex.
  It bypasses gpiod_put_array and both ref puts. No later writer can replace it:
  every pulse first checks it under that mutex. Static storage is not devm memory.
  No exit/remove/reset/rehome/clear/release hook exists. Only proven final LOW (or
  no output attempt under the stated ASIS contract) reaches normal release.
- The retained scalar phase and issued count include exactly the existing
  completed/readback-confirmed iterations, not unconfirmed partial writes or
  fabricated physical steps. Common issued-step addition still occurs on errors;
  the original capacity reservation bounds it and the retained precomputed sum.
  Error/phase history is not rolled back, overwritten by denied pulses, or lost
  with old devm ptz storage after consumer unbind.
- Lines185–235: existing state fields keep their original meaning and append only
  bank_hold/hold_axis/hold_attempted/hold_initialized/hold_result/
  hold_issued_steps/hold_phase. The hold fields are meaningful with bank_hold=1.
  No pointer or private identity is exposed. A rebound axis has fresh ordinary
  per-device counters as before, but cannot move while the static HOLD persists;
  retained historical counters remain separately visible. Probe/tail after
  DEVICE_ATTR_RO(state) are byte-identical to the baseline.

## Unclosed boundaries

Indefinite ownership retention can leave coils energized. It is NOT a physical
STOP, emergency cutoff, deadline, current/thermal limit or recovery method.
get/set/direction calls and the mutex can block; there is no finite-hang proof.
There is no product STOP/queue-revocation API, native broker or independently
armed rail-cut/reset recovery here. Suspend, provider/consumer lifecycle policy,
privileged reconfiguration and actual native pairing remain gates. Ordinary
consumer unbind drains callbacks but does not clear HOLD; provider explicit unbind
can destroy operational backing despite descriptor retention. No promise otherwise.

Motion/sign/limits/displacement, current/polarity and defensible absolute origin
remain unqualified. Stock homing remains NO-GO. No physical operation, candidate
function invocation, test or synthetic GPIO result was used to support this proof.
Independent ordinary review and parent disposition must precede any use.
