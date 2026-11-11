# Publishing on the PRG32 Cartridge Store

Release 1.0.0 ships one Store bundle:

| Bundle | Store id | Contents |
|---|---|---|
| `dist/pompeii-1.0.0-store.zip` | `org.riscv-prg32.pompeii-the-last-song-of-vesuvius` | `manifest.json`, `icon.png`, `splash.png`, `preview.mp4`, `pompeii-esp32c6.prg32`, `pompeii-qemu.prg32` |

The bundle was validated with CartridgeStore's own ingestion code, at CartridgeStore commit `156368b`. Two functions were used: `_prepare_bundle()`, which implements `POST /api/publish/bundle`, and `_bundle_preview()`, which reads the preview video. The bundle is **ready for submission** but has **not** been submitted: publishing is an authenticated, human-approved step. Checksums are in `dist/SHA256SUMS`.

## Bundle format

```text
manifest.json     prg32-metadata-1.0 object + assets + architectures + colophon
icon.png          64x64 palettised PNG (1,112 bytes)
splash.png        320x200 Waterworks frame captured from the PRG32 QEMU firmware
preview.mp4       60 s, 640x400 H.264 + AAC, real QEMU frames and audio (964 KB)
*.prg32           one file per architecture: esp32c6 and qemu
```

`scripts/pack-store-bundle.sh` generates the manifest from `metadata/metadata.json` and `metadata/colophon.json`:
- `abi` is `prg32-metadata-1.0`.
- `runtime` is removed; the Store writes `runtime.architecture` for each variant.
- `assets` is `{"icon": "icon.png", "splash": "splash.png", "preview": "preview.mp4"}`.
- `architectures` lists the `esp32c6` and `qemu` files.
- `colophon` is embedded as an object.

The Store rebuilds every cartridge from the manifest, icon, splash and colophon. Each rebuilt image is about 19.1 KB, within the 64 KiB slot (about 46.4 KB free). At run time the cartridge needs 108,484 bytes of executable RAM, which is the 128 KiB profile used by the default PRG32 v1.8 firmware for both QEMU and ESP32-C6.

## Rebuild, check, pack

```sh
make test
make cartridge PRG32_ROOT=../PRG32
make qemu PRG32_ROOT=../PRG32
make preview PRG32_ROOT=../PRG32
python3 tools/make_store_art.py
make cartridge store PRG32_ROOT=../PRG32
```

The steps do the following:
1. `make test` runs the host tests and the full playthrough.
2. The first `make cartridge` builds both variants.
3. `make qemu` checks that the bot reaches the ending on QEMU.
4. `make preview` records `build/qemu-preview/preview.mp4`. Copy it to `assets/store/` when it should replace the current one.
5. `make_store_art.py` regenerates the icon and screenshot from `build/qemu-preview`.
6. The final command re-attaches the art, then packs and validates the bundle.

Expected output from the last command:

```text
preview  preview.mp4                        963862 bytes OK [store-ingest]
esp32c6  pompeii-esp32c6.prg32               19134 bytes (+46402 free) OK [store-ingest]
qemu     pompeii-qemu.prg32                  19132 bytes (+46404 free) OK [store-ingest]
pompeii-1.0.0-store.zip: READY for submission (store-ingest)
```

The ZIP timestamps are pinned to `updated_at`, so the bundle is byte-reproducible.

## Before submitting

- [x] `make test`: all suites pass, including the playthrough
- [x] `scripts/build.sh`: both cartridges built, budgets respected, position independence proven
- [x] `make qemu`: ending reached on PRG32 QEMU (`docs/qemu/smoke-run.json`)
- [x] `scripts/pack-store-bundle.sh`: `READY for submission (store-ingest)`
- [x] `cd dist && shasum -a 256 -c SHA256SUMS`
- [ ] hardware acceptance on an ESP32-C6 recorded
- [ ] metadata reviewed by the project owner: Store id, authors, credits, license, description
- [ ] human approval of the release

## Submitting (authenticated, human-controlled)

No script in this repository contacts a Store, and no credentials are stored here. A maintainer with a Store account creates a personal Bearer token in the Store (`/auth/tokens`). They then submit the bundle from the PRG32 checkout:

```sh
python3 -m prg32 store publish-bundle ../Pompeii-The-Last-Song-of-Vesuvius/dist/pompeii-1.0.0-store.zip --store-url https://store.prg32.uniparthenope.it/ --token "$PRG32_STORE_TOKEN"
```

The Store replies `{"status": "pending", "review_required": true, ...}`. The cartridge appears in the catalog only after an editor approves the submission.

## Releasing a new version

1. Bump `version` (and `updated_at`, `release_date`) in `metadata/metadata.json` and `metadata/colophon.json`, and add a `CHANGELOG.md` entry.
2. Re-run the steps above. The bundle file is named after the version.
3. Update the sizes quoted in `README.md` and in this file.
