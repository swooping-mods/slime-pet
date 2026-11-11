#ifndef SP_INTERACTIONS_H
#define SP_INTERACTIONS_H
static unsigned sp_interactions[2];
enum {SP_INTERACT_PENDING=1,SP_INTERACT_DROP,SP_INTERACT_PICKUP,SP_INTERACT_SHOP=16};
static int sp_interaction_query(void *context,void *actor,MoonInteractionTarget *target){
    PTR walker=actor;unsigned alternate=(unsigned)(U64)context;
    if(!sp_world_ready||sp_failed||!moon_interaction_unity_gate(walker,su_get(sw.world),sw.world_field,sw.restoring,sw.blocked,sw.typing,sw.firstperson,sp_focus))return 0;
    DWORD pid=0;sp_window_pid(sp_foreground(),&pid);if(pid!=GetCurrentProcessId())return 0;
    SPPet *pet=sp_player(&sp_core,sp_local);
    U64 identity=pet?sp_visuals[(unsigned)(pet-sp_core.players)].root:sc_walker;if(!identity)identity=sc_walker;
    int held_pet=pet&&pet->active&&pet->mode==SP_HELD,pending=(sm.pending&&sm.pending_kind==SP_PICKUP)||sm.pickup_accepted;
    if(held_pet||pending){if(alternate||!identity)return 0;*target=(MoonInteractionTarget){identity,0,MOON_INTERACTION_CARRY_PRIORITY,held_pet?SP_INTERACT_DROP:SP_INTERACT_PENDING};return 1;}
    if(!alternate&&pet&&pet->active&&!sc_hands->occupied(walker)&&sp_target_pet(walker,pet)){
        MoonInteractionPoint origin,direction;if(!moon_interaction_unity_view(walker,&origin,&direction))return 0;
        *target=(MoonInteractionTarget){identity,su_sqrt(moon_interaction_distance2(origin,moon_interaction_from_sp(pet->position))),0,SP_INTERACT_PICKUP};return identity!=0;
    }
    int shop=sp_target_shop(walker);if(shop<0)return 0;
    MoonInteractionPoint origin,direction;if(!moon_interaction_unity_view(walker,&origin,&direction))return 0;
    SPVec3 point=su_vector(su.tr_get_position,sw_transform(su_get(sp_shops[shop].button)));
    *target=(MoonInteractionTarget){sp_shops[shop].button,su_sqrt(moon_interaction_distance2(origin,moon_interaction_from_sp(point))),0,SP_INTERACT_SHOP+(unsigned)shop};return 1;
}
static int sp_interactions_bind(PTR domain,PTR game){
    sp_hints=sp_hints_connect(domain,game);if(!sp_hints)return 0;
    const char *actions[]={"slime-pet.interact","slime-pet.buy"},*bindings[]={"E","LeftMouse"};
    for(unsigned i=0;i<2;i++)if(!(sp_interactions[i]=moon_interaction_register(actions[i],bindings[i],MOON_INTERACTION_EXCLUSIVE,sp_interaction_query,(void *)(U64)i)))return 0;return 1;
}
static void sp_interaction_hint(PTR walker,MoonInteractionState state){
    if(!sp_hints)return;
    if(state.selected){char text[128];const char *suffix=state.token==SP_INTERACT_DROP?": Put down slime":state.token==SP_INTERACT_PICKUP?": Pick up slime":state.token==SP_INTERACT_PENDING?": Waiting for host":": Adopt slime";
        if(moon_interaction_binding_hint(text,sizeof(text),"","slime-pet.interact","E",suffix)){sp_hints->publish("slime-pet",walker,text);return;}}
    sp_hints->clear("slime-pet");
}
#endif
