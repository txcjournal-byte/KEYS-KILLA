# CHROME VAULT sample library (orphan branch)

This branch holds only the CHROME VAULT sample library: `Library/library.json` + `Library/audio/**.ogg`, built by
`VaultLibBuild` (main branch: `tools/vault/LibBuild.cpp`, `tools/vault/library_map.json`) from the CC0 1.0 recordings
of VSCO 2 Community Edition and the Versilian Community Sample Library (credits: `docs/CHROME_VAULT_CREDITS.md`).
It shares no history with the code. CI (`.github/workflows/build.yml`) checks it out into `./vault-library`, runs
`VaultTests` on it and packs it into the installers. Regenerate with `tools/vault/package_library.py branch`.
