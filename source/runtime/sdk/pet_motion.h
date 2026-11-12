#ifndef MOON_PET_MOTION_H
#define MOON_PET_MOTION_H
#include "pose_motion.h"

/* Scene-free presentation for fixed-step pet movement. Navigation, gravity,
 * collision, replication and ownership remain authoritative in the consumer.
 * candidate() never changes state: collision-check it, then commit the accepted
 * result. Reset on spawn, recall, world/owner change and carry transitions. */
typedef struct {MoonPose pose,target;unsigned valid;} MoonPetMotion;
typedef struct {MoonPoseTuning response;float discontinuity_distance;} MoonPetMotionTuning;
enum {MOON_PET_MOTION_REJECT=0,MOON_PET_MOTION_EASE=1,MOON_PET_MOTION_RESET=2};

static inline float moon_pet_distance_squared(MoonPoseVector a,MoonPoseVector b){
    float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;
}
static inline int moon_pet_motion_candidate(const MoonPetMotion *state,MoonPose target,
    MoonPetMotionTuning tuning,float dt,int reset,MoonPose *out){
    if(!state||!out||!moon_pose_valid(&target)||!moon_pose_finite(dt)||dt<0||dt>.1f||
       !moon_pose_finite(tuning.discontinuity_distance)||tuning.discontinuity_distance<=0||
       !moon_pose_finite(tuning.response.position_response)||tuning.response.position_response<=0||tuning.response.position_response>100||
       !moon_pose_finite(tuning.response.rotation_response)||tuning.response.rotation_response<=0||tuning.response.rotation_response>100)return 0;
    if(reset||!state->valid||moon_pet_distance_squared(target.position,state->target.position)>
        tuning.discontinuity_distance*tuning.discontinuity_distance){*out=target;return MOON_PET_MOTION_RESET;}
    MoonPose current=state->pose;if(!moon_pose_valid(&current))return 0;
    if(dt==0){*out=current;return MOON_PET_MOTION_EASE;}
    return moon_pose_step(current,target,tuning.response,dt,out)?MOON_PET_MOTION_EASE:MOON_PET_MOTION_REJECT;
}
static inline int moon_pet_motion_commit(MoonPetMotion *state,MoonPose target,MoonPose accepted){
    if(!state||!moon_pose_valid(&target)||!moon_pose_valid(&accepted))return 0;
    state->pose=accepted;state->target=target;state->valid=1;return 1;
}
static inline float moon_pet_motion_speed(const MoonPetMotion *state,MoonPose accepted,float dt){
    if(!state||!state->valid||!moon_pose_finite(dt)||dt<=0||dt>.1f||!moon_pose_valid(&accepted))return 0;
    float distance=moon_pet_distance_squared(state->pose.position,accepted.position);
    return distance>1e-12f?moon_pose_sqrt(distance)/dt:0;
}

/* Imported hops own mesh airtime; the logical collision foot stays on its
 * measured support. Brief FOLLOW/IDLE changes finish the same hop, and a move
 * resuming during its landing never restarts the airborne source pose. */
typedef struct {float time;unsigned active,cycle;} MoonPetHop;
static inline int moon_pet_hop_step(MoonPetHop *state,int moving,int carried,
    float dt,float duration,float cadence){
    if(!state||!moon_pose_finite(dt)||dt<0||dt>.1f||!moon_pose_finite(duration)||duration<.01f||duration>60||
       !moon_pose_finite(cadence)||cadence<=0||cadence>10||
       !moon_pose_finite(state->time)||state->time<0||state->time>=duration)return 0;
    if(carried){state->active=0;state->time=0;state->cycle=0;return 1;}
    if(!state->active){if(moving){state->active=1;state->time=0;state->cycle=0;}return 1;}
    float next=state->time+dt*cadence;
    if(!moving&&next>=duration){state->active=0;state->time=0;state->cycle=0;}
    else if(next>=duration){
        unsigned cycles=(unsigned)(next/duration);state->time=next-(float)cycles*duration;state->cycle+=cycles;
    }else state->time=next;
    return 1;
}
#endif
