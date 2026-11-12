#ifndef SLIME_PET_PRESENTATION_H
#define SLIME_PET_PRESENTATION_H
/* The root is a visual foot anchor. Capsule checks use the logical center;
 * imported mesh animation adds the hop once, without moving simulation state. */
enum {SVP_RESET_FIRST=1,SVP_RESET_CARRY=2,SVP_RESET_DISTANCE=4,SVP_RESET_SOLID=8,SVP_RESET_COMMAND=16};
static inline SPVec3 svp_vector(MoonPoseVector p){return su_v(p.x,p.y,p.z);}
static inline float svp_apply(SVPet *v,SPVec3 foot,SUQuat rotation,SPVec3 up,float dt,unsigned held){
    MoonPose target={{foot.x,foot.y,foot.z},{rotation.x,rotation.y,rotation.z,rotation.w}},next;
    int reset=v->motion_held!=held;
    v->motion_kind=MOON_PET_MOTION_REJECT;v->motion_reset_reason=0;
    int kind=moon_pet_motion_candidate(&v->motion,target,(MoonPetMotionTuning){{18,20},3.f},dt,reset,&next);
    if(!kind)return 0;
    if(kind==MOON_PET_MOTION_RESET){
        v->motion_reset_reason=v->motion_reset_pending;
        if(!v->motion.valid&&!v->motion_reset_pending)v->motion_reset_reason|=SVP_RESET_FIRST;
        if(reset)v->motion_reset_reason|=SVP_RESET_CARRY;
        if(v->motion.valid&&moon_pet_distance_squared(target.position,v->motion.target.position)>9.f)v->motion_reset_reason|=SVP_RESET_DISTANCE;
        v->hop=(MoonPetHop){0};v->animation_mode=~0u;
    }
    if(kind==MOON_PET_MOTION_EASE&&moon_pet_distance_squared(next.position,v->motion.pose.position)>1e-10f){
        SPVec3 lift=su_mul(up,SP_HALF_HEIGHT);
        if(!sw_clear(&sw,su_add(svp_vector(v->motion.pose.position),lift),su_add(svp_vector(next.position),lift),up,SP_RADIUS,SP_HEIGHT,0)){
            /* A smoothed corner/step must not cut through real solids. The
             * already checked authoritative target is the safe reset pose. */
            next=target;kind=MOON_PET_MOTION_RESET;v->motion_reset_reason|=SVP_RESET_SOLID;
        }
    }
    float speed=kind==MOON_PET_MOTION_EASE?moon_pet_motion_speed(&v->motion,next,dt):0;
    if(!moon_pet_motion_commit(&v->motion,target,next))return 0;
    v->motion_held=held;v->motion_kind=(unsigned)kind;v->motion_reset_pending=0;
    su_set_vector(su.tr_position,su_get(v->transform),svp_vector(next.position));
    su_set_quat(su.tr_rotation,su_get(v->transform),(SUQuat){next.rotation.x,next.rotation.y,next.rotation.z,next.rotation.w});
    return speed;
}
#endif
