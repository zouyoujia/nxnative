# DRAGON QUEST BUILDERS 2 (`01001C500E72E000`)

## Current status

The registered profile is for the Hong Kong release, update 1.7.3 (`v262144`), SDK 5.4.0, Koei Tecmo's KTGL on NVN. Its main build ID is `173d12a38e9628133f2336d6c9e4b44b`. It loads `main`, `subsdk0` (GLSLC), `subsdk1` (SDK movie library) and `sdk`.

Mac gameplay is verified on an M4 Pro: character creation and appearance changes, the opening Hargon scene, the wake-up dialogue in the ship, then movement and camera input in the first cell. Characters, NPCs, scene textures, lighting and effects render with the complete shader set; this route has no missing-shader reports. The system-settings save commits successfully. iOS, gameplay save/reload and sustained progression remain open.

The supplied XCI carries both Application and Patch content. The base control data says 1.6.1; the patch says 1.7.3. The current NSP script reads only the Application content from a base XCI, so its embedded Patch content was exported into a separate update NSP for this build. The merged extraction passed its integrity checks. The original and exported game packages remain outside the repository.

## What it needs

| Setting | Where | Why |
|---|---|---|
| `modules main subsdk0 subsdk1 sdk` | [game.tsv](game.tsv) | main uses the runtime GLSLC compiler and SDK movie library |
| 4 GB layout, `app_pool 0xCD500000`, `sys_resource 0` | `game.tsv` | NPDM declares no system resource; startup requests a 3.02 GB heap within the 3.25 GB region |
| `pad_layout full` | `game.tsv` | retain both sticks for movement and the camera |
| asset and runtime shader groups | `game.tsv` | complete shader packages are in LINKDATA; six built-in outputs are compiled during startup |

## Rendering

- **Resolved: missing character previews and 3D scenes while UI and dialogue draw.** The provisional split had 130 shaders. User logs at character selection and in the opening dialogue reported 76 distinct missing keys, including missing vertex and compute stages. The actual archive has 42,870 index records in `LINKDATA.IDX`; `LINKDATA.BIN` stores plain or chunked-zlib members. Most blocks are 64 KiB, but the missed shader packages use 128 KiB and 256 KiB; other resources use 2 MiB. All nonempty members now decode to their indexed length. Character creation, the opening scene and the first ship cell now render. See [SPKG packages in KTGL LINKDATA](../../../docs/reference/shaders.md#spkg-packages-in-ktgl-linkdata).
- The complete map has 2,107 shaders: 517 vertex, 1,558 pixel and 32 compute. Sources: LINKDATA 2,068, main 33, runtime 6. Every one of the 76 observed missing keys is present. All 2,107 Mac libraries compile, all bindings decode, and the offline archive builds 517 pipeline bases. No new translation-chain version is needed: the reader changes profile inputs, not shader lowering.

## Startup

- **Resolved: initialization fails, then cleanup dereferences a null object.** Probes on the initialization stages found the first false return at `main+0x84ea0`, from `main+0x42cd40`. Its resource engine calls `main+0x858510`, which creates an SSL context at `main+0x8585f8`; the host returned `0x2a2`. Initialization then unwinds, and cleanup calls `main+0x267190` with null, faulting at `main+0x2673c8` on field `+0x60db0`. The shared host now allocates SSL contexts independently of network availability. No cleanup call or initialization check is skipped. See [SSL contexts before network connections](../../../docs/reference/services.md#ssl-contexts-before-network-connections).

## Found on this game

- [SSL contexts before network connections](../../../docs/reference/services.md#ssl-contexts-before-network-connections): a context must exist even when the game will subsequently play offline.
- [SPKG packages in KTGL LINKDATA](../../../docs/reference/shaders.md#spkg-packages-in-ktgl-linkdata): same-stem BIN/IDX indexes and header-selected zlib block sizes.
- [Switch Pro Controller face buttons are swapped](../../../docs/reference/services.md#switch-pro-controller-face-buttons-are-swapped): Nintendo-labeled GameController elements must not be exchanged again.

## Controller

The physical Pro Controller was recognized, but the old common input mapping exchanged its A/B and X/Y letters. The shared Mac/iOS adapter now preserves the Pro Controller's Nintendo letters. Device enumeration confirmed the `Switch Pro Controller` category, matching A/B/X/Y labels and no system remapping; physical presses in the rebuilt game still need confirmation. See [the shared input finding](../../../docs/reference/services.md#switch-pro-controller-face-buttons-are-swapped).

## Night-scene brightness (open)

The opening ship deck was reported to look too dark. An isolated Mac run reached the same skeleton NPC on deck. Its captured frame has 620 draws and 58 attachment bindings, no missing shader reports or placeholder texture bindings, populated material/normal targets and active lighting/post-processing. Torch lighting in the cell is visible. The final attachment uses the game's `RGBA8_SRGB` view (`0x38`, Metal format 71); the present path reads its stored bytes without a second sRGB decode. The real-Metal present tests passed all 88 checks.

No host lighting defect is established by these checks. Low-resolution frames of the original opening show a similar dark night scene, but are insufficient for a quantitative brightness comparison. A matched original Switch frame, with the same camera, dialogue and display settings, remains needed before changing exposure or gamma. No brightness override was added.

## Open

- Validate gameplay save/reload, the rest of the ship tutorial, later islands and sustained gameplay.
- Validate iOS separately.
