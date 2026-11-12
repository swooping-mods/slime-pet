#ifndef SLIME_PET_OBSERVE_H
#define SLIME_PET_OBSERVE_H
/* Pure admission policy shared by the passive observer and its host check.
 * Observation is independent of scene-mutating test tools and physical input. */
static inline int spo_enabled(const char *value,unsigned count){return value&&count==1&&value[0]=='1';}
static inline int spo_jump_sample(unsigned observe,unsigned tools,unsigned frame,unsigned clip,unsigned hop){
    return clip==hop&&(observe?frame%2==0:tools&&frame%6==0);
}
#endif
