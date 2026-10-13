CHROME VAULT preset banks. Every `*.cvb` file here (format CVB1, see docs/VAULT.md) is embedded into the
ChromeVault plugin in file-name order. With no `.cvb` file the plugin uses its starter bank (EVOLVE factory sounds).

`factory.cvb` is the 5,000-preset factory bank. It is GENERATED - do not edit it by hand. Regenerate it from the repo root with

    cmake --build build-vault --target VaultBankGen
    build-vault/VaultBankGen_artefacts/Release/VaultBankGen          # writes resources/vault/factory.cvb + docs/VAULT_BANK_REPORT.md

The generator is deterministic (same sources = same bank). Preset ids (`cv.<category>.<nnnn>`) are saved in host projects:
after a release, never regenerate in a way that changes what an existing id sounds like - append new presets instead.
