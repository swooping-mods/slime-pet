#ifndef SLIME_UNIT_REPLICA_H
#define SLIME_UNIT_REPLICA_H
#include "pet_protocol.h"
#include "../unit-runtime/sdk.h"
#define SPUR_MAX (32u + SP_PLAYERS * 48u)
_Static_assert(SPUR_MAX <= MU_PAYLOAD_MAX,"all retained slime owners fit one shared snapshot");
typedef struct {SPReplica previous,current;float age;unsigned valid,empty_mask;} SPUnitReplica;
static inline void spur_copy(void *out,const void *in,unsigned n){
    unsigned char *a=out;const unsigned char *b=in;for(unsigned i=0;i<n;i++)a[i]=b[i];
}
static inline unsigned spur_write(const SPCore *core,unsigned empty_mask,unsigned char *out,unsigned cap){
    unsigned char original[SP_SNAPSHOT_MAX];
    if(!core||!out||(empty_mask&~31u))return 0;
    unsigned n=sp_snapshot_write(core,original,sizeof(original));if(!n)return 0;
    unsigned count=sp_r32(original+28),size=32+count*48;if(cap<size)return 0;
    /* Reject an oversized enum rather than truncating it. The old decoder
     * retains semantic validation; float/identity bytes remain unchanged. */
    for(unsigned i=0;i<count;i++)for(unsigned j=0;j<4;j++)if(sp_r32(original+32+i*64+44+j*4)>255)return 0;
    spur_copy(out,original,32);sp_w32(out,0x31525553u);sp_w32(out+4,1u|(empty_mask<<8));
    for(unsigned i=0;i<count;i++){
        unsigned char *to=out+32+i*48;const unsigned char *from=original+32+i*64;
        spur_copy(to,from,44);for(unsigned j=0;j<4;j++)to[44+j]=(unsigned char)sp_r32(from+44+j*4);
    }return size;
}
/* Only call from the shared service's authenticated, increasing outer snapshot
 * callback. Equal simulation ticks are valid refreshes while the host is paused. */
static inline int spur_receive(SPUnitReplica *r,SPU64 session,const unsigned char *bytes,unsigned n){
    if(!r||!bytes||!session||n<32||sp_r32(bytes)!=0x31525553u)return 0;
    unsigned flags=sp_r32(bytes+4),count=sp_r32(bytes+28);
    SPU64 world=sp_r64(bytes+8);
    if((flags&~0x1fffu)||(flags&255u)!=1||!world||sp_r64(bytes+16)!=session||count>SP_PLAYERS||n!=32+count*48)return 0;
    int same=r->valid&&r->current.session_id==session;
    if(same){
        if(r->current.world_id!=world)return 0;
        unsigned difference=sp_r32(bytes+24)-r->current.tick;
        if(difference>=0x80000000u)return 0;
    }
    unsigned char original[SP_SNAPSHOT_MAX];spur_copy(original,bytes,32);
    sp_w32(original,0x31305353u);sp_w32(original+4,1);
    for(unsigned i=0;i<count;i++){
        unsigned char *to=original+32+i*64;const unsigned char *from=bytes+32+i*48;
        spur_copy(to,from,44);for(unsigned j=0;j<4;j++)sp_w32(to+44+j*4,from[44+j]);sp_w32(to+60,0);
    }
    SPReplica pending={0};
    if(!sp_snapshot_read(&pending,world,session,original,32+count*64))return 0;
    r->previous=same&&r->age<=2?r->current:pending;r->current=pending;
    r->age=0;r->valid=1;r->empty_mask=(flags>>8)&31u;return 1;
}
static inline void spur_present(SPUnitReplica *r,SPReplica *out,float elapsed){
    if(!r||!out||!r->valid||!sp_finite(elapsed)||elapsed<0)return;
    r->age=elapsed>3?3:r->age+elapsed;if(r->age>3)r->age=3;*out=r->current;
    if(r->age>2){out->count=0;return;}
    float alpha=r->age/.1f;if(alpha>1)alpha=1;
    for(unsigned i=0;i<out->count;i++){
        SPReplicaPet *p=&out->pets[i];if(p->mode==SP_HELD)continue;
        for(unsigned j=0;j<r->previous.count;j++){
            const SPReplicaPet *old=&r->previous.pets[j];
            if(!sp_id_eq(p->id,old->id)||old->mode==SP_HELD)continue;
            SPVec3 delta=sp_sub(p->position,old->position);if(sp_dot(delta,delta)<=16)p->position=sp_add(old->position,sp_mul(delta,alpha));break;
        }
    }
}
/* Guest display slots are separate from durable host ownership. Reclaim absent
 * IDs before adding newcomers, including an unowned local player, so all 16
 * received pets fit. Surviving IDs retain their visual/owner-pose slot. */
static inline int spur_apply_guest(SPCore *core,const SPReplica *view,void (*retire)(void *,unsigned),void *ctx){
    if(!core||!view||!view->world_id||!view->session_id||view->count>SP_PLAYERS)return 0;
    for(unsigned i=0;i<view->count;i++){
        SPId id=view->pets[i].id;if(!id.lo&&!id.hi)return 0;
        for(unsigned j=0;j<i;j++)if(sp_id_eq(id,view->pets[j].id))return 0;
    }
    unsigned kept=0;int slots[SP_PLAYERS];
    for(unsigned i=0;i<view->count;i++){
        slots[i]=-1;for(unsigned j=0;j<SP_PLAYERS;j++)if(!(kept&(1u<<j))&&core->players[j].used&&sp_id_eq(core->players[j].id,view->pets[i].id)){
            slots[i]=(int)j;kept|=1u<<j;break;
        }
    }
    for(unsigned j=0;j<SP_PLAYERS;j++)if(!(kept&(1u<<j))){
        SPPet *p=&core->players[j];if((p->used||p->active)&&retire)retire(ctx,j);
        unsigned char *bytes=(unsigned char*)p;for(unsigned k=0;k<sizeof(*p);k++)bytes[k]=0;
    }
    for(unsigned i=0;i<view->count;i++){
        if(slots[i]<0)for(unsigned j=0;j<SP_PLAYERS;j++)if(!(kept&(1u<<j))){slots[i]=(int)j;kept|=1u<<j;break;}
        SPPet *p=&core->players[slots[i]];const SPReplicaPet *from=&view->pets[i];
        p->used=p->active=p->owned=1;p->id=from->id;p->position=from->position;p->up=from->up;p->heading=from->heading;
        p->variant=from->variant;p->mode=from->mode;p->emotion=from->emotion;p->blink=from->blink;
    }
    core->world_id=view->world_id;core->session_id=view->session_id;core->tick=view->tick;
    for(unsigned i=0;i<SP_STATIONS;i++)core->stations[i].world_id=view->world_id;return 1;
}
#endif
