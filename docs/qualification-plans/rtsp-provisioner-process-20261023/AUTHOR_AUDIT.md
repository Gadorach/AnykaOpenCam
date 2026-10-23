# Ordinary author audit before invocation

2026-10-23. Read actual original constructor/handoff, backend config/prefix checks,
verifier material ABI/launch/isolation checks and real prestarted lifecycle. Read
all new C/harness/recipe after writing. This is self-inspection, NOT independent
review or a failure/negative/adversarial test suite. CONTRACT.md was added first.

- Native hardening is a real original helper call in child only, before reads;
  original create repeats it. HOST PUBLICDUMMY preprocessor class excludes it and
  has distinct process/handoff symbols. No production native hardening execution.
- Parent moves canonical ingress, does not read it, closes after actual fork;
  fork/preflight failures retain slot in stable owner. Default SIGCHLD is checked;
  no external reaper/aliases/concurrency is a caller contract. Only OWN fork PID
  enters waitpid; no kill/wait on selectors or retry after unknown/ECHILD.
- Actual child h object constructs/writes/moves; parent never reconstructs it.
  SCM_RIGHTS send is a duplicate. Child clears/closes config alias once and socket/
  directory aliases, wipes entire owned secret scratch, then exits0; every failure
  wipes then _exit(nonzero), never resumes parent code. No destructor or unlink.
- Receive uses MSG_CMSG_CLOEXEC, bounded kernel ancillary buffer (max8 delivered
  rights) and stores delivered rights before rejecting truncation/malformed/extra
  metadata. Exactly one correct FD required. No secrets in public envelope.
- Parent bind requires actual wait0, known closes, unchanged original public
  snapshot/owner, linked inode identity/type/owner0600/nlink1/size1536/read-only/
  CLOEXEC/offset0. No material payload or prefix is read in new parent path.
- Nominal clock/poll limit does not prove finite scheduler/syscall duration or
  recovery; HOLD remains sticky, with explicit own-child observer and stable
  retained namespace/FD/queue state. Unknown close numbers are never reused.
- Explicit zero envelope tail avoids uninitialized padding on wire. Public cfg
  snapshots use memcpy so exact comparison does not depend on struct-assignment
  padding. Immutable graph/registry/address and exclusive namespace are mandatory
  external whole-lifetime conditions, not permission booleans.
- Recipe invokes only compilers/linkers/readelf/nm; produces HOST/ARM native-shaped
  REL, separate dummy executable, no app/kernel/image/main/native verifier launch.
  Existing parser warning exceptions are isolated to unchanged rtsp_input.c.
  Native-shaped process/handoff objects are excluded from the dummy executable.
- Harness only uses explicit PUBLICDUMMY credentials and a fresh0700 /tmp/opencode
  directory, local pipe/socketpair/constructor/file/SCM_RIGHTS/wait/bind; no network
  call (URI literals are parser inputs), payload read/hash or private fixture.
  Parent-held dummy constants explicitly do NOT prove product secret-free fork.
  Test cleanup only follows known complete success, with single closes/one inode-
  matched unlink/rmdir. Failure does no cleanup and keeps a static owner alive
  indefinitely if child retirement remains unknown; no automatic kill or retry.

Only ONE normal functional invocation is authorized after successful build/symbol
inspection. No original builders/helpers/controllers/generators or native effects
are invoked. Author changes remain confined to the assigned app/docs directories.
