# Collection errors retained

Two initial read-tool lookups used guessed filenames rather than the actual
inventory and returned `tool.execution: File not found`:

- `apps/rtsp-auth/owned-listener-candidate/owned_listener_v1.c`
- `apps/rtsp-auth/owned-listener-candidate/rtsp_server_owned.h`

The directory inventory resolved the names to `listener_custody.c/.h` and the
actual public `owned_listener_v1.h`. No file was created at the guessed paths,
no candidate was run, and neither error was a policy rejection. Subsequent
static collection errors, if any, remain in fresh `STATIC_*.json` receipts.
