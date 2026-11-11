#ifndef MOON_SOUND_FRAMEWORK_V1_H
#define MOON_SOUND_FRAMEWORK_V1_H
#define MA_ABI 1u
#define MA_CUES 64u
#define MA_VOICES 16u
/* Register immutable mono PCM16 during ModStart. Provider memory must outlive
 * the process. All playback, emitter and lifecycle calls belong to Unity's
 * main thread. No scene pointers cross this SDK boundary. Zero means failure. */
typedef struct {
    unsigned abi,size;
    const char *key;
    const short *pcm;
    unsigned samples,rate;
    float gain,min_distance,max_distance,cooldown;
} MoonSoundCue;
typedef struct {
    unsigned abi,size;
    unsigned (*register_cue)(const MoonSoundCue *cue);
    int (*play)(unsigned cue,unsigned long long emitter,const float position[3],float pitch,float gain);
    void (*move)(unsigned long long emitter,const float position[3]);
    void (*stop)(unsigned long long emitter);
    int (*on_update)(void (*callback)(void *walker)); /* Optional shared provider dispatch. */
} MoonSounds;
typedef const MoonSounds *(*MoonSoundsGet)(unsigned abi,unsigned size);
#endif
