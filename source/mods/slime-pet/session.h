#ifndef SLIME_SESSION_H
#define SLIME_SESSION_H
/* Main-thread authority coordinator. Transport authenticates the sender; host
 * derives all owner poses and wallet names from vanilla's peer objects. */
static struct {
    char root[1024];SPU64 nonce,host;float network_time,retry_time,save_time;
    unsigned established,pending,dirty,save_failed,authoritative,suspended;SPCommand command;SPReplica replica;
    unsigned char bytes[SS_MAX_BYTES];
} sm;
/* A guest never gains write authority merely because its connection closed. */
static int sm_host(void){return sm.authoritative&&!sm.suspended;}
#include "session_store.h"
static void sm_begin(const char *root){
    su_zero(&sm,sizeof(sm));cat(sm.root,root);sm.nonce=sp_core.session_id^sw_object_id(su_get(sw.world));if(!sm.nonce)sm.nonce=1;
    sp_empty_mask=0;sm.host=st_host_id();U64 local=st_local_id();
    sm.authoritative=!st_joined()||(sm.host&&local&&sm.host==local);
    if(sm_host()){
        unsigned n=0;if(ss_load(root,sm.bytes,sizeof(sm.bytes),&n)){
            if(n<8||sp_r32(sm.bytes)!=0x31564d53u||(sp_r32(sm.bytes+4)&~31u)||!sp_persist_read(&sp_core,sm.bytes+8,n-8))sp_event("pet-save-invalid");
            else{sp_empty_mask=sp_r32(sm.bytes+4);sp_event("pet-save-loaded");}
        }else if(ss.last_error!=SS_NOT_FOUND)sp_event("pet-save-read-failed");
        SPPet *local=sp_player(&sp_core,sp_local);if(local)sp_txn=local->last_txn;
        sm.established=1;sp_event("authority-host");
    }else sp_event("authority-guest");
}
static void sm_end(void){if(sp_world_ready&&sm_host())sm_save();su_zero(&sm,sizeof(sm));}
static PTR sm_wallet_name(U64 peer){int ok=1;void *a[]={&peer};return call(method("MoonCleaners","MoonCrew","PlayerId",1),0,a,&ok);}
static void sm_remote_owners(PTR world){
    PTR list=0;su.static_field(st.shown,&list);if(!list)return;int count=sw_list_count(list);if(count<0||count>64)return;
    unsigned seen[SP_PLAYERS];su_zero(seen,sizeof(seen));SPPet *local=sp_player(&sp_core,sp_local);if(local)seen[(unsigned)(local-sp_core.players)]=1;
    for(int i=0;i<count;i++){
        PTR remote=sw_list_item(list,i);if(!su_alive(remote))continue;U64 peer=st_u64(st.remote_id,remote);if(!peer||peer==st_local_id()||!st_member(peer))continue;
        SPId id=sp_id(peer);SPPet *present=sp_player(&sp_core,id);
        if(present){seen[(unsigned)(present-sp_core.players)]=1;present->owner_valid=0;}
        SPVec3 pos=su_vector(su.tr_get_position,sw_transform(remote)),up;SPGround ground;
        if(!sw_up(&sw,pos,&up)||!sw_ground(&sw,su_add(pos,su_mul(up,.1f)),up,3,&ground))continue;
        int ok=1;SPU32 balance=sw_balance(world,sm_wallet_name(peer),&ok);if(!ok||sp_join(&sp_core,id,balance)!=SP_OK)continue;
        SPOwner owner={sp_core.world_id,ground.point,up,su_vector(su.tr_forward,sw_transform(remote))};sp_owner(&sp_core,id,&owner);
        SPPet *p=sp_player(&sp_core,id);if(p)seen[(unsigned)(p-sp_core.players)]=1;
    }
    for(unsigned i=0;i<SP_PLAYERS;i++)if(sp_core.players[i].used&&!seen[i])sp_leave(&sp_core,sp_core.players[i].id);
}
static void sm_welcome(U64 peer,U64 nonce){unsigned char b[40];su_zero(b,sizeof(b));sp_w32(b,0x31574853u);sp_w32(b+4,SP_SESSION_VERSION);sp_w64(b+8,nonce);sp_w64(b+16,sp_core.world_id);sp_w64(b+24,sp_core.session_id);sp_w32(b+32,sp_empty_mask);st_send(peer,b,sizeof(b));}
static void sm_send_snapshot(U64 peer){unsigned char b[SP_SNAPSHOT_MAX];SPU32 n=sp_snapshot_write(&sp_core,b,sizeof(b));if(n)st_send(peer,b,n);}
static void sm_receive(U64 peer,const unsigned char *bytes,unsigned length,void *ctx){
    PTR world=ctx;if(length<4)return;SPU32 magic=sp_r32(bytes);
    if(sm_host()){
        if(length==24&&magic==0x31484853u&&sp_r32(bytes+4)==SP_SESSION_VERSION&&sp_r64(bytes+8)&&!sp_r64(bytes+16)){sm_welcome(peer,sp_r64(bytes+8));sm_send_snapshot(peer);return;}
        SPCommand command;if(!sp_command_read(&command,bytes,length))return;
        SPResult result=sp_execute(world,sp_id(peer),sm_wallet_name(peer),&command);unsigned char response[SP_RESULT_BYTES];
        SPU32 n=sp_result_write(&result,sp_core.world_id,sp_core.session_id,response,sizeof(response));if(n)st_send(peer,response,n);
        sm_welcome(peer,0);sm_send_snapshot(peer);return;
    }
    if(peer!=st_host_id()||peer!=sm.host)return;
    if(length==40&&magic==0x31574853u&&sp_r32(bytes+4)==SP_SESSION_VERSION&&!sp_r32(bytes+36)){
        U64 nonce=sp_r64(bytes+8),world_id=sp_r64(bytes+16),session_id=sp_r64(bytes+24);unsigned mask=sp_r32(bytes+32);
        if(!world_id||!session_id||(mask&~31u))return;
        if(!sm.established){
            if(nonce!=sm.nonce)return;sp_core.world_id=world_id;sp_core.session_id=session_id;sm.established=1;su_zero(&sm.replica,sizeof(sm.replica));sp_event("host-handshake");
            for(unsigned i=0;i<5;i++)sp_core.stations[i].world_id=world_id;
        }else if(world_id!=sp_core.world_id||session_id!=sp_core.session_id)return;
        sp_empty_mask=mask;return;
    }
    if(!sm.established)return;
    SPResult result;if(sp_result_read(&result,sp_core.world_id,sp_core.session_id,bytes,length)){
        if(!sm.pending||result.txn_id!=sm.command.txn_id)return;
        if(result.watermark>sp_txn)sp_txn=result.watermark;
        if(result.code==SP_OLD_TRANSACTION){sm.command.txn_id=++sp_txn;sm.retry_time=0;return;}
        sp_feedback(sm.command.kind,sm.command.station,result);sm.pending=0;return;
    }
    if(sp_snapshot_read(&sm.replica,sp_core.world_id,sp_core.session_id,bytes,length)){
        for(unsigned i=0;i<SP_PLAYERS;i++)sp_core.players[i].active=0;
        for(unsigned i=0;i<sm.replica.count;i++){
            SPReplicaPet *r=&sm.replica.pets[i];if(sp_join(&sp_core,r->id,0)!=SP_OK)continue;SPPet *p=sp_player(&sp_core,r->id);if(!p)continue;
            p->active=1;p->owned=1;p->position=r->position;p->up=r->up;p->heading=r->heading;p->variant=r->variant;p->mode=r->mode;p->emotion=r->emotion;p->blink=r->blink;
        }
    }
}
static void sp_command_local(PTR world,unsigned kind,unsigned station){
    if(sm.suspended){if(station<5)sv_label_text(&sp_shops[station].label,"REJOIN WORLD");return;}
    if(sm.pending){if(station<5)sv_label_text(&sp_shops[station].label,"PLEASE WAIT");return;}
    SPCommand command={sp_core.world_id,sp_core.session_id,++sp_txn,kind,station};
    if(sm_host()){SPResult r=sp_execute(world,sp_local,sp_player_name(),&command);sp_feedback(kind,station,r);return;}
    if(!sm.established){if(station<5)sv_label_text(&sp_shops[station].label,"HOST MOD\nREQUIRED");sp_event("host-not-ready");return;}
    sm.command=command;sm.pending=1;sm.retry_time=0;
}
static void sm_tick(PTR world,float dt){
    U64 host=st_host_id(),local=st_local_id();if(sm.suspended)return;
    int changed=sm.authoritative?(st_joined()&&host&&host!=local):(!st_joined()||!host||(sm.host&&host!=sm.host));
    if(changed){
        sm.suspended=1;sm.pending=0;sm.established=0;
        for(unsigned i=0;i<SP_PLAYERS;i++)sp_core.players[i].active=0;
        sp_event("authority-changed-rejoin-required");return;
    }
    if(!sm.authoritative&&!sm.host)sm.host=host;
    if(sm_host())sm_remote_owners(world);st_receive(sm_receive,world);
    sm.network_time-=dt;sm.retry_time-=dt;sm.save_time-=dt;
    if(sm_host()){
        SPWorld callbacks=sw_callbacks();sp_tick(&sp_core,dt,&callbacks);
        if(sm.network_time<=0){U64 peers[SP_PLAYERS];unsigned n=st_peers(peers,SP_PLAYERS);for(unsigned i=0;i<n;i++){sm_send_snapshot(peers[i]);sm_welcome(peers[i],0);}sm.network_time=.15f;}
        if(sm.dirty&&sm.save_time<=0){sm_save();sm.save_time=1;}
    }else{
        if(!sm.established&&sm.network_time<=0){unsigned char hello[24];su_zero(hello,sizeof(hello));sp_w32(hello,0x31484853u);sp_w32(hello+4,SP_SESSION_VERSION);sp_w64(hello+8,sm.nonce);st_send(sm.host,hello,sizeof(hello));sm.network_time=1;}
        if(sm.pending&&sm.retry_time<=0){unsigned char request[SP_COMMAND_BYTES];unsigned n=sp_command_write(&sm.command,request,sizeof(request));if(n)st_send(sm.host,request,n);sm.retry_time=.5f;}
    }
    for(unsigned i=0;i<5;i++)if(sp_shops[i].made){SVShop *s=&sp_shops[i];s->empty=(sp_empty_mask>>i)&1u;
        su_set_vector(su.tr_scale,su_get(s->display.transform),s->empty?su_v(0,0,0):su_v(.29f,.29f,.29f));
        if(s->status_time<=0)sv_label_text(&s->label,sp_labels[i]);
    }
}
#endif
