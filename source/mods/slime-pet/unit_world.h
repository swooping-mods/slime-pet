#ifndef SLIME_UNIT_WORLD_H
#define SLIME_UNIT_WORLD_H
#include "../unit-runtime/contact.h"
static MUCContactCache sp_unit_contacts;
static int sp_unit_clear(void *ctx,SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height,SPU64 support){
    if(!sw_clear(ctx,from,to,up,radius,height,support))return 0;
    return muc_capsule_clear(&sp_unit_contacts,sp_units,sp_contacts,3,(MoonUnitVector){from.x,from.y,from.z},(MoonUnitVector){to.x,to.y,to.z},(MoonUnitVector){up.x,up.y,up.z},radius,height);
}
static SPWorld sp_unit_world(void){muc_invalidate(&sp_unit_contacts);SPWorld w=sw_callbacks();w.clear=sp_unit_clear;return w;}
#endif
