#ifndef MOONLAB_SLIME_SOUND_EVENTS_H
#define MOONLAB_SLIME_SOUND_EVENTS_H
/* The built-in shared audio service also delivers these optional notifications.
 * All notifications occur on the Unity main thread. A sink must return promptly,
 * copy what it retains, and must not reenter the mod or mutate game state.
 * JUMP_CYCLE marks the authored animation loop boundary, not a physics landing. */
enum {
    SP_AUDIO_SPAWN=1,SP_AUDIO_DESPAWN,SP_AUDIO_FOLLOW_START,SP_AUDIO_FOLLOW_STOP,
    SP_AUDIO_SLEEP,SP_AUDIO_WAKE,SP_AUDIO_JUMP_CYCLE,SP_AUDIO_ADOPT,SP_AUDIO_RECALL,SP_AUDIO_PICKUP,SP_AUDIO_DROP,SP_AUDIO_LAND,SP_AUDIO_TAKEOFF
};
typedef struct {
    unsigned size,version,event,owner_lo,owner_hi,variant,sequence;
    float position[3];
} SPSoundEvent;
typedef void (*SPSoundSink)(const SPSoundEvent *);
static SPSoundSink sp_sound_sink;
static unsigned sp_sound_sequence,sp_sound_notifying;
static struct {unsigned active,mode,cycle,frame,clip;} sp_sound_states[SP_PLAYERS];
__declspec(dllexport) void MoonLab_SlimeSoundSink_v1(SPSoundSink sink){sp_sound_sink=sink;}
static inline void sp_sound_emit(unsigned kind,const SPPet *pet){
    if(!pet||sp_sound_notifying)return;
    switch(kind){
        case SP_AUDIO_TAKEOFF:sa_pet(sa_hop,pet,1);break;
        case SP_AUDIO_LAND:sa_pet(sa_land,pet,1);break;
        case SP_AUDIO_ADOPT:case SP_AUDIO_RECALL:sa_pet(sa_adopt,pet,1);break;
        case SP_AUDIO_PICKUP:case SP_AUDIO_WAKE:sa_pet(sa_pickup,pet,kind==SP_AUDIO_WAKE?.45f:1.f);break;
        case SP_AUDIO_DROP:sa_pet(sa_drop,pet,1);break;
        case SP_AUDIO_SLEEP:sa_pet(sa_sleep,pet,1);break;
        default:break;
    }
    if(!sp_sound_sink)return;
    SPSoundEvent e={sizeof(SPSoundEvent),1,kind,pet->id.lo,pet->id.hi,pet->variant,++sp_sound_sequence,{pet->position.x,pet->position.y,pet->position.z}};
    sp_sound_notifying=1;sp_sound_sink(&e);sp_sound_notifying=0;
}
static inline void sp_sound_visual(unsigned i,const SPPet *pet,const SVPet *visual){
    unsigned mode=pet->mode,clip=visual->last_clip,cycle=0;
    if(clip<sp_slime_asset.clip_count){const SPClip *c=&sp_slime_asset.clips[clip];if(c->frame_count)cycle=(unsigned)(visual->animation_time*c->sample_rate)/c->frame_count;}
    if(!sp_sound_states[i].active)sp_sound_emit(SP_AUDIO_SPAWN,pet);
    else if(sp_sound_states[i].mode!=mode){
        if(sp_sound_states[i].mode==SP_SLEEP)sp_sound_emit(SP_AUDIO_WAKE,pet);
        if(sp_sound_states[i].mode==SP_FOLLOW)sp_sound_emit(SP_AUDIO_FOLLOW_STOP,pet);
    }
    if((!sp_sound_states[i].active||sp_sound_states[i].mode!=mode)&&mode==SP_SLEEP)sp_sound_emit(SP_AUDIO_SLEEP,pet);
    if((!sp_sound_states[i].active||sp_sound_states[i].mode!=mode)&&mode==SP_FOLLOW)sp_sound_emit(SP_AUDIO_FOLLOW_START,pet);
    unsigned jump=spa_clip(&sp_slime_asset,SP_FOLLOW);
    unsigned landing=sp_slime_asset.clips[jump].land_frame,takeoff=sp_slime_asset.clips[jump].takeoff_frame;
    if(clip==jump&&(!sp_sound_states[i].active||sp_sound_states[i].clip!=jump||sp_sound_states[i].cycle!=cycle))sp_sound_emit(SP_AUDIO_JUMP_CYCLE,pet);
    if(clip==jump&&visual->last_frame>=takeoff&&
       (!sp_sound_states[i].active||sp_sound_states[i].clip!=jump||sp_sound_states[i].cycle!=cycle||sp_sound_states[i].frame<takeoff))sp_sound_emit(SP_AUDIO_TAKEOFF,pet);
    if(sp_sound_states[i].clip==jump&&sp_sound_states[i].frame>=takeoff&&sp_sound_states[i].frame<landing&&
       ((clip==jump&&visual->last_frame>=landing&&sp_sound_states[i].frame<landing)||
        (clip!=jump&&mode!=SP_HELD)||(clip==jump&&sp_sound_states[i].cycle!=cycle)))sp_sound_emit(SP_AUDIO_LAND,pet);
    if(sp_sound_states[i].active&&mode!=sp_sound_states[i].mode){
        if(mode==SP_HELD)sp_sound_emit(SP_AUDIO_PICKUP,pet);
        else if(sp_sound_states[i].mode==SP_HELD)sp_sound_emit(SP_AUDIO_DROP,pet);
    }
    sp_sound_states[i].active=1;sp_sound_states[i].mode=mode;sp_sound_states[i].cycle=cycle;
    sp_sound_states[i].frame=visual->last_frame;sp_sound_states[i].clip=clip;
}
#endif
