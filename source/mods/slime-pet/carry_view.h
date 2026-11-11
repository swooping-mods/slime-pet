#ifndef SLIME_CARRY_VIEW_H
#define SLIME_CARRY_VIEW_H
#include "../pickup-hands/sdk/hands.h"
/* Selection belongs to Hands. Keep the reservation through an authenticated
 * guest pickup request, and retain a failed release so restoration can retry. */
static const MoonHands *sc_hands;
static SURoot sc_walker;
static int sc_carry_claim(PTR walker){
    if(!sc_hands||!walker)return 0;
    if(sc_walker&&su_get(sc_walker)!=walker)return 0;
    if(!sc_walker&&sc_hands->occupied(walker))return 0;
    int fresh=!sc_walker;
    if(fresh){sc_walker=su_hold(walker);if(!sc_walker)return 0;}
    if(!sc_hands->claim("slime-pet",walker)){if(fresh)su_drop(&sc_walker);return 0;}
    return 1;
}
static int sc_carry_release(void){
    if(!sc_walker)return 1;
    PTR walker=su_get(sc_walker);
    if(su_alive(walker)&&!sc_hands->release("slime-pet",walker))return 0;
    su_drop(&sc_walker);return 1;
}
/* World teardown must not retain an obsolete walker if its world is already
 * gone. Hands owns its context and resets that snapshot on the next world. */
static void sc_carry_end(void){sc_carry_release();su_drop(&sc_walker);}
static void sc_carry_view(PTR walker,int carrying){
    if(carrying)sc_carry_claim(walker);else sc_carry_release();
}
#endif
