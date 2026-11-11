#ifndef SLIME_UNIT_SESSION_H
#define SLIME_UNIT_SESSION_H
/* Provider 3 borrows authentication, retries and sequencing from MoonUnits.
 * Its durable transaction ledger and real-physics navigation stay Slime-owned. */
#include "unit_replica.h"
static struct {
    char root[1024];SPU64 session;float save_time;
    unsigned established,pending,dirty,save_failed,authoritative,suspended;
    unsigned pending_kind,pending_station,registry_failed,pickup_accepted;
    SPUnitReplica replica;unsigned char bytes[SS_MAX_BYTES];
} sm;
static int sm_host(void){return sm.authoritative&&!sm.suspended&&sp_units->host()&&sp_units->session()==sm.session;}
#include "session_store.h"
static void sm_remove_units(void){for(unsigned i=0;i<SP_PLAYERS;i++)sp_units->remove(3,i+1);}
static void sm_begin(const char *root){
    su_zero(&sm,sizeof(sm));cat(sm.root,root);sm.session=sp_units->session();
    sm.authoritative=sp_units->host();sp_empty_mask=0;
    if(sm.authoritative){
        if(!sm.session){sm.suspended=1;return;}sp_core.session_id=sm.session;
        unsigned n=0;if(ss_load(root,sm.bytes,sizeof(sm.bytes),&n)){
            if(n<8||sp_r32(sm.bytes)!=0x31564d53u||(sp_r32(sm.bytes+4)&~31u)||!sp_persist_read(&sp_core,sm.bytes+8,n-8)){sm.suspended=1;sp_event("pet-save-invalid");return;}
            else{sp_empty_mask=sp_r32(sm.bytes+4);sp_event("pet-save-loaded");}
        }else if(ss.last_error!=SS_NOT_FOUND){sm.suspended=1;sp_event("pet-save-read-failed");return;}
        sm.established=1;sp_event("authority-host");
    }else sp_event("authority-guest");
}
static void sm_end(void){if(sp_world_ready&&sm_host())sm_save();sm_remove_units();su_zero(&sm,sizeof(sm));}
static PTR sm_wallet_name(SPU64 peer){int ok=1;void *a[]={&peer};PTR name=call(method("MoonCleaners","MoonCrew","PlayerId",1),0,a,&ok);return ok?name:0;}
/* Only retained owners and authenticated request senders enter this ledger.
 * An ungrounded loaded avatar loses its pose, rather than despawning its pet. */
static int sm_remote_owner(PTR world,SPU64 peer,int join){
    SPId id=sp_id(peer);SPPet *p=sp_player(&sp_core,id);if(p)p->owner_valid=0;
    PTR actor=sp_units->peer_actor(peer);SURoot root=su_hold(actor);
    if(!root){if(p)sp_leave(&sp_core,id);return 0;}
    if(!p&&join){
        PTR wallet=sm_wallet_name(peer);int ok=1;SPU32 balance=wallet?sw_balance(world,wallet,&ok):0;
        if(!wallet||!ok||sp_join(&sp_core,id,balance)!=SP_OK){su_drop(&root);return 0;}p=sp_player(&sp_core,id);
    }
    if(p){
        PTR tr=sw_transform(su_get(root));SPVec3 pos=su_vector(su.tr_get_position,tr),up;SPGround g;
        if(sw_up(&sw,pos,&up)&&sw_ground(&sw,su_add(pos,su_mul(up,.1f)),up,3,&g)){
            SPOwner owner={sp_core.world_id,g.point,up,su_vector(su.tr_forward,tr)};sp_owner(&sp_core,id,&owner);
        }
    }
    su_drop(&root);return p!=0;
}
static void sm_remote_owners(PTR world){
    for(unsigned i=0;i<SP_PLAYERS;i++)if(sp_core.players[i].used&&!sp_id_eq(sp_core.players[i].id,sp_local)){
        SPId id=sp_core.players[i].id;sm_remote_owner(world,((SPU64)id.hi<<32)|id.lo,0);
    }
}
static int sm_publish_pet(unsigned slot,const SPPet *p){
    if(!p->active||p->mode==SP_HELD){sp_units->remove(3,slot+1);return 1;}
    return sp_units->publish(3,slot+1,3,(MoonUnitVector){p->position.x,p->position.y,p->position.z},SP_RADIUS);
}
static void sm_publish(void){
    for(unsigned i=0;i<SP_PLAYERS;i++)if(!sm_publish_pet(i,&sp_core.players[i])&&!sm.registry_failed){sm.registry_failed=1;sp_event("unit-publish-failed");}
}
static unsigned sm_snapshot(unsigned char *out,unsigned cap){return sp_world_ready&&sm_host()?spur_write(&sp_core,sp_empty_mask,out,cap):0;}
static int sm_receive(const unsigned char *bytes,unsigned n){
    if(!sp_world_ready||sm.authoritative||sm.suspended||sp_units->host())return 0;
    SPU64 session=sp_units->session();if(!session||(sm.session&&session!=sm.session))return 0;
    if(!spur_receive(&sm.replica,session,bytes,n))return 0;
    /* Results and snapshots use separate runtime channels. A successful pickup
     * must keep hands reserved while presentation still contains FOLLOW. */
    if(sm.pickup_accepted)for(unsigned i=0;i<sm.replica.current.count;i++)if(sp_id_eq(sm.replica.current.pets[i].id,sp_local)&&sm.replica.current.pets[i].mode==SP_HELD){sm.pickup_accepted=0;break;}
    sm.session=session;sm.established=1;
    sp_core.world_id=sm.replica.current.world_id;sp_core.session_id=session;
    sp_empty_mask=sm.replica.empty_mask;return 1;
}
static void sm_reset(void){
    if(!sp_world_ready)return;
    /* On ordinary teardown Unit still exposes host ownership here. An authority
     * change revokes it before this callback, so a guest can never write. */
    if(sm_host())sm_save();sm_remove_units();sm.suspended=1;sm.pending=sm.established=sm.pickup_accepted=0;
    su_zero(&sm.replica,sizeof(sm.replica));
    for(unsigned i=0;i<SP_PLAYERS;i++){sp_core.players[i].active=0;sp_core.players[i].owner_valid=0;}
    sp_event("authority-changed-rejoin-required");
}
static SPResult sm_execute_owner(PTR world,SPId id,PTR wallet,unsigned kind,unsigned station){
    SPPet *p=sp_player(&sp_core,id);SPResult failure={0};failure.txn_id=1;failure.code=SP_BAD_INPUT;
    if(!sm_host()||!p||!wallet||p->last_txn==~(SPU64)0)return failure;
    SPCommand command={sp_core.world_id,sm.session,p->last_txn+1,kind,station};
    return sp_execute(world,id,wallet,&command);
}
static unsigned sm_command(SPU64 peer,const unsigned char *bytes,unsigned n,unsigned char *out,unsigned cap){
    if(!sp_world_ready||!sm_host()||!bytes||n!=12||!out||cap<SP_RESULT_BYTES||sp_r32(bytes)!=1)return 0;
    unsigned kind=sp_r32(bytes+4),station=sp_r32(bytes+8);
    if(kind<SP_PURCHASE||kind>SP_DROP||station>=SP_STATIONS||!peer||sp_id_eq(sp_id(peer),sp_local))return 0;
    if(!sp_player(&sp_core,sp_id(peer))){
        unsigned used=0;for(unsigned i=0;i<SP_PLAYERS;i++)used+=sp_core.players[i].used!=0;
        if(used==SP_PLAYERS){
            /* A bounded owner ledger is a completed rejection, not a transient
             * missing-avatar condition that should retry forever. */
            SPResult full={0};full.txn_id=1;full.code=SP_FULL;
            return sp_result_write(&full,sp_core.world_id,sm.session,out,cap);
        }
    }
    PTR world=su_get(sw.world);if(!world||!sm_remote_owner(world,peer,1))return 0;
    SPResult result=sm_execute_owner(world,sp_id(peer),sm_wallet_name(peer),kind,station);
    return sp_result_write(&result,sp_core.world_id,sm.session,out,cap);
}
static int sm_result(const unsigned char *bytes,unsigned n){
    SPResult result;if(!sp_world_ready||sm.authoritative||sm.suspended||!sm.pending||!sm.established||sp_units->session()!=sm.session||
        !sp_result_read(&result,sp_core.world_id,sm.session,bytes,n))return 0;
    if(sm.pending_kind==SP_PICKUP&&(result.code==SP_OK||result.code==SP_ALREADY_OWNED))sm.pickup_accepted=1;
    else if(result.code==SP_OK)sm.pickup_accepted=0;
    /* A rejected repeat PICKUP cannot revoke an earlier accepted reservation. */
    sp_feedback(sm.pending_kind,sm.pending_station,result);sm.pending=0;return 1;
}
static void sp_command_local(PTR world,unsigned kind,unsigned station){
    if(sm.suspended||!sp_units->session()||(sm.session&&sp_units->session()!=sm.session)){
        if(station<SP_STATIONS)sv_label_text(&sp_shops[station].label,"REJOIN WORLD");return;
    }
    if(sm.pending){if(station<SP_STATIONS)sv_label_text(&sp_shops[station].label,"PLEASE WAIT");return;}
    if(kind<SP_PURCHASE||kind>SP_DROP||station>=SP_STATIONS)return;
    if(sm_host()){SPResult r=sm_execute_owner(world,sp_local,sp_player_name(),kind,station);sp_feedback(kind,station,r);return;}
    if(!sm.established||!sm.replica.valid||sm.replica.age>2){sv_label_text(&sp_shops[station].label,"HOST MOD\nREQUIRED");sp_event("host-not-ready");return;}
    unsigned char request[12];sp_w32(request,1);sp_w32(request+4,kind);sp_w32(request+8,station);
    if(sp_units->command(3,request,sizeof(request))){sm.pending_kind=kind;sm.pending_station=station;sm.pending=1;}
    else sv_label_text(&sp_shops[station].label,"PLEASE WAIT");
}
static void sm_tick(PTR world,float real_dt){
    if(sm.suspended)return;
    SPU64 session=sp_units->session();
    if((sm.session&&session!=sm.session)||(sm.authoritative&&!sp_units->host())||(!sm.authoritative&&sp_units->host())){sm_reset();return;}
    if(!sp_finite(real_dt)||real_dt<0)real_dt=0;
    if(sm_host()){
        sm_remote_owners(world);int ok=1;int frame=integer(sp_frame,0,&ok);
        unsigned steps=ok&&frame>=0?sp_units->steps(3,(SPU64)frame,su_number(su.time_delta,0)):0;
        SPWorld callbacks=sp_unit_world();
        for(unsigned i=0;i<steps;i++){sp_tick(&sp_core,.05f,&callbacks);sm_publish();}
        /* Publish commands and removals even during a paused simulation. */
        if(!steps)sm_publish();
        sm.save_time-=real_dt;if(sm.dirty&&sm.save_time<=0){sm_save();sm.save_time=1;}
    }else if(sm.replica.valid){
        SPReplica view={0};spur_present(&sm.replica,&view,real_dt);
        if(view.world_id)spur_apply_guest(&sp_core,&view,sp_retire_guest,0);
    }
    for(unsigned i=0;i<SP_STATIONS;i++)if(sp_shops[i].made){SVShop *s=&sp_shops[i];s->empty=(sp_empty_mask>>i)&1u;
        su_set_vector(su.tr_scale,su_get(s->display.transform),s->empty?su_v(0,0,0):su_v(.29f,.29f,.29f));
        if(s->status_time<=0)sv_label_text(&s->label,sp_labels[i]);
    }
}
#endif
