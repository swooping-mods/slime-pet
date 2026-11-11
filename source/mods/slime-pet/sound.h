#ifndef SLIME_SOUND_H
#define SLIME_SOUND_H
#include "../sound-framework/sdk/client.h"
#include "shop_assets/shop_click.h"
#include "audio/slime_sounds.h"
#include "audio/generated/springy_sounds.h"
static const MoonSounds *sa_api;
static unsigned sa_hop,sa_land,sa_adopt,sa_pickup,sa_drop,sa_sleep,sa_button;
static U64 sa_emitters[SP_PLAYERS];
static unsigned sa_register(const char *key,const short *pcm,unsigned samples,unsigned rate,float gain,float cooldown){
    MoonSoundCue cue={MA_ABI,sizeof(MoonSoundCue),key,pcm,samples,rate,gain,.9f,12.f,cooldown};return sa_api->register_cue(&cue);
}
static int sa_setup(PTR domain,PTR game){
    sa_api=ma_connect(domain,game);if(!sa_api)return 0;
#define SA(N,U,G,C) sa_##N=sa_register("slime-pet." #N,ms_##N##_pcm,MS_##U##_SAMPLES,MS_##U##_RATE,G,C);if(!sa_##N)return 0
    sa_hop=sa_register("slime-pet.hop",ms_springy_hop_pcm,MS_SPRINGY_HOP_SAMPLES,MS_SPRINGY_HOP_RATE,.45f,.25f);
    sa_land=sa_register("slime-pet.land",ms_springy_land_pcm,MS_SPRINGY_LAND_SAMPLES,MS_SPRINGY_LAND_RATE,.38f,.25f);
    if(!sa_hop||!sa_land)return 0;
    SA(adopt,ADOPT,.60f,.8f);SA(pickup,PICKUP,.55f,.6f);SA(drop,DROP,.45f,.4f);SA(sleep,SLEEP,.20f,3.f);
#undef SA
    sa_button=sa_register("slime-pet.button",ms_shop_click_pcm,MS_SHOP_CLICK_SAMPLES,MS_SHOP_CLICK_RATE,.5623413f,.04f);
    return sa_button!=0;
}
static U64 sa_id(const SPPet *pet){return ((U64)pet->id.hi<<32)|pet->id.lo;}
static void sa_pet(unsigned cue,const SPPet *pet,float gain){
    if(!sa_api||!pet||!cue)return;float p[]={pet->position.x,pet->position.y,pet->position.z};
    /* Small deterministic variant tuning, never randomized per-frame pitch. */
    sa_api->play(cue,sa_id(pet),p,.94f+(float)pet->variant*.035f,gain);
}
static inline int sa_begin(void){return sa_api!=0;}
static inline void sa_click(PTR cap_transform){
    if(!sa_api)return;SPVec3 p=su_vector(su.tr_get_position,cap_transform);
    float position[]={p.x,p.y,p.z};sa_api->play(sa_button,~(U64)0,position,1,1);
}
static void sa_follow(unsigned i,const SPPet *pet,SPVec3 position){
    if(!sa_api||i>=SP_PLAYERS)return;
    U64 id=sa_id(pet);if(sa_emitters[i]&&(!pet->active||id!=sa_emitters[i]))sa_api->stop(sa_emitters[i]);
    sa_emitters[i]=pet->active?id:0;
    if(pet->active){float p[]={position.x,position.y,position.z};sa_api->move(id,p);}
}
static inline void sa_end(void){
    if(sa_api)for(unsigned i=0;i<SP_PLAYERS;i++)if(sa_emitters[i])sa_api->stop(sa_emitters[i]);
    su_zero(sa_emitters,sizeof(sa_emitters));
}
#endif
