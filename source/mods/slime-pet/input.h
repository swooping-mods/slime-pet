#ifndef SLIME_INPUT_H
#define SLIME_INPUT_H
#include "../../runtime/native/interaction.h"
typedef struct {int summon,interact,buy;} SPControls;
/* Preserve the owning mod's gate/edge machine. Each logical action is sampled
 * once per frame; pet interaction retains priority over the alternate BUY key. */
static inline SPControls sp_controls(void){
    SPControls controls;
    controls.summon=moon_input_action("slime-pet.summon","F6",1).held;
    controls.interact=moon_input_action("slime-pet.interact","E",1).held;
    int buy=moon_input_action("slime-pet.buy","LeftMouse",1).held;
    controls.buy=controls.interact||buy;return controls;
}
static inline int sp_controls_guide(char *out,unsigned cap){
    if(!moon_interaction_binding_hint(out,cap,"SLIME ADOPTION\nVisit a slime terminal at a cardinal base.\nAim at BUY and press ","slime-pet.interact","E"," or "))return 0;
    unsigned n=0;while(out[n])n++;
    if(!moon_interaction_binding_hint(out+n,cap-n,"","slime-pet.buy","LeftMouse",".\n"))return 0;
    while(out[n])n++;return moon_interaction_binding_hint(out+n,cap-n,"","slime-pet.summon","F6"," recalls your companion.");
}
#endif
