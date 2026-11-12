# Slime Pet: behavior and implementation

Describes the retained payload **0.5.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.5.1** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.5.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Adopt a slime from the starter-area terminals by aiming at BUY and interacting. Each player owns at most one pet; buying a variant replaces it. Unit Runtime authenticates guest requests and supplies host simulation/snapshots. The host validates payment, placement, follow movement and recall. Aim and E picks up a nearby slime; E sets it down and F6 recalls to an admitted clear point. Pickup Hands and exclusive interaction delivery prevent overlapping tool actions; Hints/Sound provide contextual feedback.

## State and lifecycle

A versioned checksummed host sidecar stores pet ownership/variant and transaction state. Floor/contact and moving-support queries use local gravity, swept/occupied capsules and real solid support. Missing floor holds position instead of fabricating a respawn; summon requires clear near-owner support. Carried/visual state is reconstructed per session. Authority change suspends commands and writes until world reentry; teardown saves valid host state and releases unit/scene/audio roots.

## Integration and configuration

Runtime dependencies: [sound-framework](https://github.com/swooping-mods/sound-framework), [unit-runtime](https://github.com/swooping-mods/unit-runtime), [pickup-hands](https://github.com/swooping-mods/pickup-hands), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/slime-pet/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `sound-framework.sound` | 1 | at least 48 bytes |
| Requires `unit-runtime.units` | 1 | at least 80 bytes |
| Requires `unit-runtime.contacts` | 1 | at least 16 bytes |
| Requires `pickup-hands.hands` | 1 | at least 32 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |

## Implementation sources

- [mod.c](../source/mods/slime-pet/mod.c)
- [pet_core.h](../source/mods/slime-pet/pet_core.h)
- [unit_session.h](../source/mods/slime-pet/unit_session.h)
- [unit_execute.h](../source/mods/slime-pet/unit_execute.h)
- [unit_replica.h](../source/mods/slime-pet/unit_replica.h)
- [session_store.h](../source/mods/slime-pet/session_store.h)

## Verification boundary

No host migration without ledger transfer, arbitrary wall-crossing recovery or cross-process wallet/sidecar transaction guarantee. Terrain, carry transitions, visual variants, reload and real co-op require matching game evidence.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
