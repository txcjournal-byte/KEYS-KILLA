CHROME VAULT preset banks. Every `*.cvb` file here (format CVB1 v2, see docs/VAULT.md) is embedded into the
ChromeVault plugin in file-name order. With no `.cvb` file the plugin uses its starter bank (EVOLVE factory sounds).

`factory.cvb` is the v2 factory bank, assembled from the preset recipes in `tools/vault/recipes/` and checked by the v2
bank gate (`tools/vault/recipes/RecipeGate.h`). It is GENERATED - do not edit it by hand. Regenerate it from the repo root
(the sample library must be installed: the gate renders the sampled parts):

    cmake --build build-vault --target VaultBankGen
    CHROME_VAULT_LIBRARY=/path/to/cv-library build-vault/VaultBankGen_artefacts/Release/VaultBankGen -recipe all -qa \
        -cache /tmp/cv-cache -out resources/vault/factory.cvb -report /tmp/cv-full-report.md
    python3 tools/vault/bank_report_doc.py /tmp/cv-full-report.md docs/VAULT_BANK_REPORT.md

The assembly is deterministic (same recipes, same library, same compiler = same bank). Preset ids
(`cv.<category slug>.<nnnn>`) are given by the recipes and saved in host projects: after a release, never change what an
existing id sounds like - append new presets with new numbers instead.
