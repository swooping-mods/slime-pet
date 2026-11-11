#ifndef SLIME_ANIMATION_CONTROL_H
#define SLIME_ANIMATION_CONTROL_H
/* Consume the imported source names rather than assuming shortened aliases. */
static inline int spa_name(const char *a,const char *b){
    if(!a||!b)return 0;while(*a&&*a==*b){a++;b++;}return *a==*b;
}
static inline unsigned spa_clip(const SPAsset *asset,unsigned mode){
    if(!asset||!asset->clip_count)return ~0u;
    const char *wanted=mode==SP_FOLLOW?"FollowHop":mode==SP_SLEEP?"Sleep":"Idle";unsigned idle=0;
    for(unsigned i=0;i<asset->clip_count;i++){
        if(spa_name(asset->clips[i].name,wanted))return i;
        if(spa_name(asset->clips[i].name,"Idle"))idle=i;
    }
    return idle;
}
#endif
