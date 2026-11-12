#ifndef MOON_POSE_MOTION_H
#define MOON_POSE_MOTION_H
/* Source-only position/rotation filter; the consumer owns collision/authority and commits
 * only accepted poses. Rejected candidates must never advance hidden state. */
typedef struct {float x,y,z;} MoonPoseVector;
typedef struct {float x,y,z,w;} MoonPoseRotation;
typedef struct {MoonPoseVector position;MoonPoseRotation rotation;} MoonPose;
typedef struct {float position_response,rotation_response;} MoonPoseTuning;

static inline int moon_pose_finite(float x){return x==x&&x>-1e7f&&x<1e7f;}
static inline float moon_pose_sqrt(float x){float n=x>1?x:1;for(int i=0;i<16;i++)n=.5f*(n+x/n);return n;}
static inline int moon_pose_normalize(MoonPoseRotation *q){
    if(!moon_pose_finite(q->x)||!moon_pose_finite(q->y)||!moon_pose_finite(q->z)||!moon_pose_finite(q->w))return 0;
    float n=q->x*q->x+q->y*q->y+q->z*q->z+q->w*q->w;
    if(n<.000001f||n>4)return 0;
    n=1/moon_pose_sqrt(n);q->x*=n;q->y*=n;q->z*=n;q->w*=n;return 1;
}
static inline int moon_pose_valid(MoonPose *p){
    return moon_pose_finite(p->position.x)&&moon_pose_finite(p->position.y)&&moon_pose_finite(p->position.z)&&moon_pose_normalize(&p->rotation);
}
static inline float moon_pose_response(float response,float dt){
    /* exp(-response*dt), without a CRT dependency in freestanding Win64 mods.
     * Range reduction keeps the fifth-order approximation below float error. */
    float x=response*dt;int squares=0;while(x>.125f){x*=.5f;squares++;}
    float decay=1-x*(1-x*(.5f-x*(1.f/6-x*(1.f/24-x/120))));
    while(squares--)decay*=decay;
    return 1-decay;
}
static inline int moon_pose_step(MoonPose current,MoonPose target,MoonPoseTuning tuning,float dt,MoonPose *out){
    if(!out||!moon_pose_finite(dt)||dt<=0||dt>.1f||
       !moon_pose_finite(tuning.position_response)||tuning.position_response<=0||tuning.position_response>100||
       !moon_pose_finite(tuning.rotation_response)||tuning.rotation_response<=0||tuning.rotation_response>100||
       !moon_pose_valid(&current)||!moon_pose_valid(&target))return 0;
    float a=moon_pose_response(tuning.position_response,dt);
    current.position.x+=(target.position.x-current.position.x)*a;
    current.position.y+=(target.position.y-current.position.y)*a;
    current.position.z+=(target.position.z-current.position.z)*a;
    float dot=current.rotation.x*target.rotation.x+current.rotation.y*target.rotation.y+current.rotation.z*target.rotation.z+current.rotation.w*target.rotation.w;
    if(dot<0){target.rotation.x=-target.rotation.x;target.rotation.y=-target.rotation.y;target.rotation.z=-target.rotation.z;target.rotation.w=-target.rotation.w;}
    /* Short normalized blends keep large turns stable across frame rates,
     * preserve unit rotation and avoid flips between equivalent q/-q poses. */
    int count=(int)(dt*240)+1;float turn=moon_pose_response(tuning.rotation_response,dt/count);
    for(int i=0;i<count;i++){
        current.rotation.x+=(target.rotation.x-current.rotation.x)*turn;
        current.rotation.y+=(target.rotation.y-current.rotation.y)*turn;
        current.rotation.z+=(target.rotation.z-current.rotation.z)*turn;
        current.rotation.w+=(target.rotation.w-current.rotation.w)*turn;
        if(!moon_pose_normalize(&current.rotation))return 0;
    }
    *out=current;return 1;
}
#endif
