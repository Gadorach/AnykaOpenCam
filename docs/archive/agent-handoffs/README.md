# Archived agent handoffs

## Pre-cleanup snapshot

`20261021-pre-cleanup.md` is the complete, unmodified working-tree `AGENTS.md`
captured for the cleanup requested Oct21 and completed after interruption Oct22.
The filename identifies its last execution-evidence date, not a new hardware run.

- Size: **71,974 bytes;1,021 lines** (UTF-8).
- SHA256: `3ebf8407dbfda1b02b29791337cb46337f8e10720ecb7cce81245d821b30dac3`.
- Execution HEAD before cleanup: `71f03a93fc179dd438e3c09422411762f08507fe`.
- Includes existing working-tree edits; it was not reconstructed from HEAD or the
  user's staged version. The ordinary Git index is preserved separately by leaving
  it unchanged.
- All original history, source pins, test counts, operation IDs, dated observations,
  restrictions and reference paths remain in the snapshot. It contains contradictory
  “latest” entries from different dates: these are **history, not current authority**.

The root `AGENTS.md` is now the concise current-state/standing-rules entry point.
Subsequent conversation steering included hardware approval and retry/resume; that
steering is recorded in the root summary, not retroactively inserted into this copy.
No implementation, test or hardware result was produced by this documentation cleanup.

## Find detailed context on demand

Paths below are relative to the workspace root. The full snapshot also retains
every earlier reference, including transient operation/recorder details.

| Topic | Detailed reference |
| --- | --- |
| Current P1 interruption and partial disposition | `docs/qualification-plans/kernel76-transport-20261021/P1_TOOL_BLOCK.md` |
| Accepted station derivatives and remaining source work | `docs/qualification-plans/kernel76-transport-20261021/STATION76_ATTACH_REMAINING_SOURCE_PLAN.md` |
| Execution chronology and product task ledger | `docs/plan/camera-foundation/EXECUTION_TASKS_20261021.md` |
| Remaining product requirements/defects/gates | `docs/plan/camera-foundation/REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md` |
| RAM76 package/build/native history | `docs/plan/camera-foundation/KERNEL76_TASKS_20261020.md` |
| Stock SD/L2, Wi-Fi, video, motor, audio, AP/NOR source findings | `docs/qualification-plans/stock-reference-20261013/closeout-parent01/NEXT_BUILD_INPUTS.md` |
| Stock closeout task ledger | `docs/plan/camera-foundation/STOCK_REFERENCE_TASKS_20261020.md` |
| Optical feed and GUI entry | `docs/plan/camera-foundation/OPTICAL_IRCUT_LIVE_20261016.md` |
| White light, motor controls and optical mapping | `docs/plan/camera-foundation/AIM_WHITE_MOTOR_20261020.md` |
| IR-cut/speaker physical isolation and bounded experiments | `docs/plan/camera-foundation/IRCUT_SPEAKER_CAMPAIGN_20261016.md` |
| Reset button input investigation | `docs/plan/camera-foundation/RESET_BUTTON_TEST_20261016.md` |
| Persistent SSH-only ROOT installation result | `docs/qualification-plans/stock-reference-20261013/persistent-ssh01/INSTALL_RESULT_20261016.md` |
| Original sensor140 versus controlled139 correction | `docs/qualification-plans/stock-reference-20261013/feed-entry/SENSOR_LENGTH_CORRECTION.md` |
| Earlier parallel foundation reviews/native-vs-fixture distinctions | `docs/plan/camera-foundation/PARALLEL_WORK_20261012.md` |
| Workspace/acquisition preservation | `docs/PROJECT_LAYOUT.md` |
| Curated local commits and ordinary-index preservation | `GIT_WORKFLOW.md` |

## Maintenance

Keep the snapshot immutable. Store future detailed outcomes in the applicable
task/evidence documents and update the short root checkpoint in place. Consult
archived sections selectively; do not load this entire snapshot by default or
treat an old owner/PID/IP/timer/approval as current runtime evidence.
