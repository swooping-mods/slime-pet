#ifndef SLIME_SESSION_STORE_H
#define SLIME_SESSION_STORE_H
/* Shared durable sidecar writer; both coordinators keep the original schema. */
static void sm_save(void){
    if(!sm_host()||!sp_world_ready||!sm.root[0])return;
    sp_w32(sm.bytes,0x31564d53u);sp_w32(sm.bytes+4,sp_empty_mask);
    SPU32 n=sp_persist_write(&sp_core,sm.bytes+8,SS_MAX_BYTES-8);
    if(!n||!ss_save(sm.root,sm.bytes,n+8)){sm.dirty=1;if(!sm.save_failed){
        sp_event("pet-save-failed");char b[200],*p=cat(b,"{\"event\":\"store-error\",\"code\":");p=num(p,ss.last_error);p=cat(p,",\"native_error\":");p=num(p,ss.native_error);p=cat(p,",\"stage\":");p=num(p,ss.native_stage);p=cat(p,"}\n");DWORD written=0;WriteFile(sp_log,b,(DWORD)(p-b),&written,0);FlushFileBuffers(sp_log);sm.save_failed=1;}return;}
    sm.dirty=0;if(sm.save_failed)sp_event("pet-save-recovered");sm.save_failed=0;
}
#endif
