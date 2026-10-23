# Cold-entry23 v2 — bounded ESPHome R1 correction

Fresh derivative of frozen `qc07f-cold-entry23/cold_entry.py`. Read that v1
README and the new v2 CONTRACT before use. V1 was never invoked; its negative
review stays preserved. This v2 is SOURCE/AST/syntax only until separate review
and actual parent selection. No implicit invocation or retry follows.

Only the HTTP acknowledgement/state distinction changes:

- GET retains strict JSON Content-Type, duplicate-key, exact id, Boolean and
  consistent ON/OFF checks. It is the only controller-state observation.
- POST permits only turn_on/turn_off, still requires HTTP200, no Location/redirect,
  timeout and bounded full acknowledgement read. Empty body/no Content-Type is
  accepted as the observed API acknowledgement, not manufactured state.
- After the one ON acknowledgement, an independent actual GET must report ON.
  The one OFF attempt is still followed by its independent GET even after error;
  all errors remain sticky. Initial OFF is GET-only.

Every other v1 source byte is preserved: exact original source/classifier pins,
private explicit port/config/roots, read-only historical roles, Popen/peer/argv
custody, first360-byte profile PLUS every controlled echo byte, separate quiet
observation, failure OFF, known-child stop/wait/final hashes and independent
failure postchecks. No old source/global or classifier is patched.

Use the same explicit argument shape with this script path, isolated interpreter
and a fresh foundation part. Original radio/reset/23c failures remain unchanged.
No image, boot, native-clock/C1 waiver, GPIO/motor/NOR/media effect or new framework.
Nominal host timeouts are not a hard deadline or host-hang/rail recovery guarantee.
