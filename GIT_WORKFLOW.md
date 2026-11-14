# Git workflow

The user requests reviewed increments committed to **main**. On2026-11-14 the user
explicitly requested a GitHub repository and public original stock dumps plus
extraction tools, excluding working built images of the current native source.
The approved destination is **https://github.com/Gadorach/AnykaOpenCam**. The user
also explicitly confirmed public untouched dumps after disclosure of the shared
factory RSA private key in the KM01/QC07F originals. Use the existing Git author.
The earlier2026-10-21 local-commits-only decision is superseded for this scope,
not permission to publish unrelated private material or create other repositories.

## Source and privacy boundaries

- Commit reviewed authored source, tests, plans, source/license notices and safe
  metadata. A source snapshot is not a build, hardware or product acceptance claim.
- Do not commit private provisioning/configuration, operator passwords/host keys,
  private captures/responses/digests, photographs, footage or generated native images.
  The narrow explicit exception is the three byte-exact original stock dumps in
  `firmware/original/`, including their embedded factory material, with warnings,
  checksums/provenance and original vendor/third-party license obligations intact.
  Do not publish later credential-bearing modified dumps or whole acquisition/
  extracted trees. This is not a global license grant or evidence of secret absence.
- Preserve local originals, source notices, experiments and failed evidence. An
  excluded path is not disposable and must not be deleted or rewritten for Git.
- Publication needs a final privacy/license and generated-artifact review of the
  reachable commit tree/history. Other future remotes do not authorize private data.

## Existing index

The workspace was already an initialized non-bare repository on main, without
an initial commit. Its pre-existing index held1993 entries, including acquisition
mirrors and partially staged source. Those staging choices are user work.

Use an **isolated temporary Git index** for a curated initial source baseline and
subsequent selected increments; initialize it from HEAD (or empty for the first
commit), add only the reviewed paths, inspect its staged diff/privacy boundary,
and commit on main. Preserve the ordinary index and all working files. Never use
`git add -A`, blanket staging, forced untracking, reset or checkout to normalize
the pre-existing work. This intentionally leaves its staged/uncommitted state
visible; do not later commit that ordinary index without reviewing it.

Hardware-specific local selectors and historical documents with unreviewed
identifiers may remain local pending a separate configuration/redaction review.
Do not mutate a sealed boot helper merely to make it publishable during a live
hardware operation. Preserve the original and qualify any future config change.

## Completed increments

1. Name the task, reviewed changes, tests and remaining native/product gates.
2. Commit only its appropriate source/docs/tests on main, retaining negative
   results and input provenance without copying private raw data into the commit.
3. Push main only to the user-specified configured destination. Do not force-push,
   erase remote history, alter visibility or assume a remote branch is empty.
4. If no remote or access exists, report **committed locally, not pushed**, keep
   executing independent approved work and request the exact missing destination.

The camera plan's effect/recovery/qualification gates continue to apply; a Git
commit or push is not permission to flash, broadcast, move motors or write cards.
