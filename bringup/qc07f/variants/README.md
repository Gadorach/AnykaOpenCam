# Exact RAM-tested board variants

`lifetime74/` and `security75/` mirror the exact ARM board-port sources used in
the isolated74/75 builds, without duplicating complete upstream Linux trees.
The existing `mainline/` overlay is preserved rather than silently overwritten.

Kernel74 adds reviewed retained-resource/receipt ownership and scoped built-in
codec lifetime policy; all12 full-frame STATIC4 CRC gates and scalar userspace
oracle remain. Kernel75 changes only seccomp/filter configuration, not driver
source. Both are RAM diagnostics, not complete product architecture.

Full pinned Linux source, recipes, failed builds, provider audits and sealed
manifests remain in the historical local `work/` campaigns. Exact configs/symvers
and images are also indexed under `firmware/recovery/lightbulb-camera/`.
