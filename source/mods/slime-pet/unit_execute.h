#ifndef SLIME_UNIT_EXECUTE_H
#define SLIME_UNIT_EXECUTE_H
static SPResult sp_execute(PTR world,SPId owner,PTR player_name,const SPCommand *command){
    SPWorld callbacks=sp_unit_world();int ok=1;SPResult failure={0};failure.txn_id=command->txn_id;failure.code=SP_BAD_INPUT;
    if(!sm_host())return failure;SPU32 balance=sw_balance(world,player_name,&ok);
    if(!ok)return failure;sp_balance(&sp_core,owner,balance);
    static SPCore pending;su_copy(&pending,&sp_core,sizeof(pending));SPResult r=sp_command(&pending,owner,command,&callbacks);
    SPPet *changed=sp_player(&pending,owner);unsigned slot=changed?(unsigned)(changed-pending.players):SP_PLAYERS;
    /* Reserve the shared marker before paying for a newly placed actor. A full
     * registry cannot produce a charged but invisible-to-contacts purchase. */
    if(r.code==SP_OK&&slot<SP_PLAYERS&&!sm_publish_pet(slot,changed)){failure.code=SP_FULL;sp_event("unit-publish-failed");return failure;}
    if(r.code==SP_OK&&r.debit&&!sw_debit(world,player_name,r.debit)){
        if(slot<SP_PLAYERS)sm_publish_pet(slot,&sp_core.players[slot]);sp_event("debit-failed");return failure;
    }
    su_copy(&sp_core,&pending,sizeof(sp_core));
    if(command->kind==SP_PURCHASE&&r.code==SP_OK)sp_empty_mask|=1u<<command->station;
    sm_save();if(r.code==SP_OK&&r.debit){call(sw.request_save,world,0,&ok);}
    return r;
}
#endif
