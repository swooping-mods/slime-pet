#ifndef MOON_SOUND_ASSETS_V1_H
#define MOON_SOUND_ASSETS_V1_H
#include "sound.h"
#define MA_ASSET_ABI 1u
#define MA_ASSETS 16u
#define MA_TOTAL_CUES (MA_CUES+MA_ASSETS)
/* Main-thread extension for bounded caller-owned PCM loaded after startup.
 * PCM must stay immutable until release returns success. Handles include a
 * generation and cannot alias a later asset. Existing Sounds.play/move/stop
 * accept these handles. Release stops voices and destroys the framework clip.
 * Rolloff: 0 logarithmic, 1 linear. No file paths cross this API. */
typedef struct {
    unsigned abi,size;
    unsigned (*register_pcm)(const MoonSoundCue *,unsigned rolloff);
    int (*release)(unsigned handle);
    int (*playing)(unsigned long long emitter);
} MoonSoundAssets;
typedef const MoonSoundAssets *(*MoonSoundAssetsGet)(unsigned,unsigned);
#endif
