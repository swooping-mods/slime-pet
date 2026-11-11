/* Slime companion native consumer. Scene operations run on the game thread. */
#include "../../runtime/native/gameplay.h"
#include "../unit-runtime/client.h"
#include "inspect.h"
#include "unity.h"
static void sp_nav_trace(const char *,SPVec3,SPVec3);
#define SP_TRACE_EDGE(stage,from,to) sp_nav_trace(stage,from,to)
#define SP_IMPLEMENTATION
#include "pet_core.h"
#define SP_PROTOCOL_IMPLEMENTATION
#include "pet_protocol.h"
#include "world.h"
#include "../../runtime/native/interaction_unity.h"
#include "../../runtime/native/interaction_client.h"
#define mh_connect sp_hints_connect
#include "../hints-framework/sdk/client.h"
#undef mh_connect
static const MoonHints *sp_hints;
#include "input.h"
#include "pet_store.h"
#include "visual.h"
#include "sound.h"
#include "sound_events.h"
#include "../pickup-hands/sdk/client.h"
#include "carry_view.h"
#include "shop_signal.h"
int _fltused=0;
void *memcpy(void *to,const void *from,U64 n){su_copy(to,from,n);return to;}
void *memset(void *to,int value,U64 n){unsigned char *bytes=to;for(U64 i=0;i<n;i++)bytes[i]=(unsigned char)value;return to;}
static const MoonUnits *sp_units;
static const MoonUnitContacts *sp_contacts;
#include "unit_world.h"
static PTR sp_log,sp_frame,sp_focus,sp_real_delta;
static SURoot sp_view_camera;
static int sp_ticks,sp_thread,sp_ready_frames,sp_world_ready,sp_failed;
static SPCore sp_core;
static SVPet sp_visuals[SP_PLAYERS];
static SVShop sp_shops[SP_STATIONS];
static SPId sp_local;
static unsigned sp_seen[5],sp_empty_mask;
static unsigned sp_initialized,sp_deferred_cue,sp_deferred_variant;
static float sp_deferred_age;
static void sm_save(void);
static int sm_host(void);
static void sm_begin(const char *);
static void sm_tick(PTR,float);
static void sm_end(void);
static void sp_command_local(PTR,unsigned,unsigned);
static int sm_publish_pet(unsigned,const SPPet *);
static void sr_drop(void);
static void sr_tick(float);
static short (*sp_key)(int);
static PTR (*sp_foreground)(void);
static DWORD (*sp_window_pid)(PTR,DWORD *);
static int sp_was_summon;
static SURoot sp_shop_hint_root;
static float sp_shop_hint_time;
static char sp_shop_hint_caption[384];
static int sp_test_tools,sp_was_funds,sp_was_view,sp_was_inspect,sp_was_material,sp_was_routes;static unsigned sp_test_station,sp_test_cull,sp_test_aim_station,sp_test_aim_frames,sp_test_aim_attempts;
static const SPU32 sp_prices[5]={125,250,300,225,275};
static const char *sp_labels[5]={"BLUE\n$125","GOLD\n$250","EMERALD\n$300","AMETHYST\n$225","PEARL\n$275"};
static void sp_event(const char *event){
    char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,event);p=cat(p,"\",\"mod\":\"slime-pet\",\"version\":\"0.4.1\",\"ticks\":");p=num(p,(U64)sp_ticks);p=cat(p,"}\n");
    DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(sp_log);
}
static char *sp_fixed(char *p,float f){if(f<0){*p++='-';f=-f;}U64 n=(U64)(f*1000+.5f);p=num(p,n/1000);*p++='.';*p++=(char)('0'+n/100%10);*p++=(char)('0'+n/10%10);*p++=(char)('0'+n%10);*p=0;return p;}
static char *sp_vector_text(char *p,SPVec3 v){*p++='[';p=sp_fixed(p,v.x);*p++=',';p=sp_fixed(p,v.y);*p++=',';p=sp_fixed(p,v.z);*p++=']';*p=0;return p;}
static void sp_station_event(const char *name,unsigned index,SPVec3 point){char b[320],*p=cat(b,"{\"event\":\"station\",\"name\":");p=sp_quote(p,name);p=cat(p,",\"index\":");p=num(p,index);p=cat(p,",\"position\":");p=sp_vector_text(p,point);p=cat(p,"}\n");DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(sp_log);}
static void sp_material_probe(PTR material,const char *role,unsigned variant){
    int ok=1;SUColor color={0};void *query[]={su.string("_BaseColor")};
    PTR value=call(su_method("UnityEngine","Material","GetColor","System.String"),material,query,&ok);
    if(!ok||!value){sp_event("material-probe-unavailable");return;}su_copy(&color,unbox(value),sizeof(color));
    PTR shader=call(su_method("UnityEngine","Material","get_shader",""),material,0,&ok);
    PTR name=shader?call(su_method("UnityEngine","Object","get_name",""),shader,0,&ok):0;char shader_name[96]={0};
    if(name)su_string(name,shader_name,sizeof(shader_name));
    char b[1536],*p=cat(b,"{\"event\":\"material-state\",\"role\":");p=sp_quote(p,role);p=cat(p,",\"variant\":");p=num(p,variant);
    p=cat(p,",\"instance\":");p=num(p,sw_object_id(material));p=cat(p,",\"shader\":");p=sp_quote(p,shader_name);
    p=cat(p,",\"color\":");p=sp_vector_text(p,su_v(color.r,color.g,color.b));p=cat(p,",\"alpha\":");p=sp_fixed(p,color.a);
    static const char *properties[]={"_Metallic","_Smoothness","_Surface","_Cull","_SrcBlend","_DstBlend","_ZWrite"};
    PTR getter=su_method("UnityEngine","Material","GetFloat","System.String");
    for(unsigned i=0;i<7;i++){
        void *a[]={su.string(properties[i])};value=call(getter,material,a,&ok);float f=ok&&value?*(float *)unbox(value):-1;
        p=cat(p,",");p=sp_quote(p,properties[i]);p=cat(p,":");p=sp_fixed(p,sp_finite(f)?f:-1);
    }
    p=cat(p,"}\n");DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(sp_log);
}
static void sp_material_probes(void){
    for(unsigned i=0;i<5;i++)sp_material_probe(su_get(sv_pet_materials[i]),"root",i);
    for(unsigned i=0;i<5;i++)if(sp_shops[i].made){
        PTR renderer=su_component(su_get(sp_shops[i].display.body.go),"Renderer");int ok=1;
        PTR material=call(su_method("UnityEngine","Renderer","get_sharedMaterial",""),renderer,0,&ok);
        if(ok&&material)sp_material_probe(material,"display",i);
    }
}
static void sp_winding_probe(PTR mesh,const char *role){
    const char *names[]={"get_vertices","get_normals","get_triangles"};SURoot arrays[3]={0};int ok=1;
    for(unsigned i=0;i<3;i++){PTR a=call(su_method("UnityEngine","Mesh",names[i],""),mesh,0,&ok);if(!ok||!a)goto done;arrays[i]=su_hold(a);}
    unsigned nv=(unsigned)su.array_length(su_get(arrays[0])),nn=(unsigned)su.array_length(su_get(arrays[1])),ni=(unsigned)su.array_length(su_get(arrays[2]));
    if(!nv||nv>SP_JELLY_VERTICES||nn!=nv||ni>49152||ni%3)goto done;
    SPVec3 *v=su.array_address(su_get(arrays[0]),sizeof(SPVec3),0),*n=su.array_address(su_get(arrays[1]),sizeof(SPVec3),0);int *indices=su.array_address(su_get(arrays[2]),sizeof(int),0);
    unsigned positive=0,negative=0,zero=0;
    for(unsigned i=0;i<ni;i+=3){
        unsigned a=(unsigned)indices[i],b=(unsigned)indices[i+1],c=(unsigned)indices[i+2];if(a>=nv||b>=nv||c>=nv)goto done;
        float sign=su_dot(su_cross(su_sub(v[b],v[a]),su_sub(v[c],v[a])),su_add(n[a],su_add(n[b],n[c])));
        if(sign>0)positive++;else if(sign<0)negative++;else zero++;
    }
    char b[384],*p=cat(b,"{\"event\":\"winding-state\",\"role\":");p=sp_quote(p,role);p=cat(p,",\"positive\":");p=num(p,positive);p=cat(p,",\"negative\":");p=num(p,negative);p=cat(p,",\"zero\":");p=num(p,zero);p=cat(p,"}\n");DWORD written=0;WriteFile(sp_log,b,(DWORD)(p-b),&written,0);FlushFileBuffers(sp_log);
done:
    for(unsigned i=0;i<3;i++)su_drop(&arrays[i]);
}
static void sp_winding_probes(void){
    int shape=3,ok=1;void *args[]={&shape};PTR go=call(su.primitive,0,args,&ok);if(!ok||!go)return;
    su_bool(method("UnityEngine","GameObject","SetActive",1),go,0);
    PTR filter=su_component(go,"MeshFilter"),mesh=call(su_method("UnityEngine","MeshFilter","get_sharedMesh",""),filter,0,&ok);
    if(ok&&mesh)sp_winding_probe(mesh,"unity-cube");su_destroy(go);
    if(sp_shops[0].made){sp_winding_probe(su_get(sp_shops[0].display.body.mesh),"slime");sp_winding_probe(su_get(sp_shops[0].bodies[0].mesh),"console");}
}
static SPId sp_id(SPU64 value){return (SPId){(SPU32)value,(SPU32)(value>>32)};}
static int sp_support_in_use(SPU64 id){for(unsigned i=0;i<SP_PLAYERS;i++)if(sp_core.players[i].active&&sp_core.players[i].support_id==id)return 1;return 0;}
static PTR sp_player_name(void){int ok=1;return call(method("MoonCleaners","MoonCrew","get_LocalPlayerId",0),0,0,&ok);}
static SPU64 sp_local_value(void){int ok=1;PTR m=method("Steamworks","SteamUser","GetSteamID",0),v=call(m,0,0,&ok);if(ok&&v){SPU64 id=*(SPU64 *)unbox(v);if(id)return id;}return 1;}
static void sp_world_drop(void){
    if(sp_hints)sp_hints->clear("slime-pet");
    sm_end();sa_end();sr_drop();sc_carry_end();
    su_drop(&sp_view_camera);su_destroy(su_get(sp_shop_hint_root));su_drop(&sp_shop_hint_root);sp_shop_hint_time=0;sp_shop_hint_caption[0]=0;sp_deferred_cue=0;su_zero(sp_sound_states,sizeof(sp_sound_states));
    for(unsigned i=0;i<SP_PLAYERS;i++)if(sp_visuals[i].made)sv_pet_drop(&sp_visuals[i]);
    for(unsigned i=0;i<SP_STATIONS;i++)if(sp_shops[i].made)sv_shop_drop(&sp_shops[i]);
    sv_materials_drop();for(unsigned i=0;i<sw.support_count;i++)su_drop(&sw.supports[i].transform);sw.support_count=0;su_drop(&sw.world);
    su_zero(sp_seen,sizeof(sp_seen));sp_world_ready=0;sp_ready_frames=0;sp_event("world-teardown");
}
static int sp_owner_update(PTR walker,PTR world){
    PTR tr=sw_transform(walker);SPVec3 pos=su_vector(su.tr_get_position,tr),up;SPGround ground;
    if(!sw_up(&sw,pos,&up))return 0;SPWorld callbacks=sw_callbacks();
    if(!callbacks.ground(&sw,su_add(pos,su_mul(up,.1f)),up,3.f,&ground))return 0;
    SPOwner owner={sp_core.world_id,ground.point,up,su_vector(su.tr_forward,tr)};
    if(sm_host()){
        int ok=1;SPU32 balance=sw_balance(world,sp_player_name(),&ok);if(!ok)return 0;
        if(sp_join(&sp_core,sp_local,balance)!=SP_OK)return 0;sp_balance(&sp_core,sp_local,balance);
    }
    return sp_owner(&sp_core,sp_local,&owner)==SP_OK;
}
static int sp_shop_position(PTR collector,unsigned variant,SPVec3 *point,SPVec3 *forward,SPVec3 *up){
    PTR tr=sw_transform(collector);SPVec3 origin=su_vector(su.tr_get_position,tr),right=su_vector(su.tr_right,tr);*up=su_vector(su.tr_up,tr);*forward=su_vector(su.tr_forward,tr);
    /* Two stations share vanilla's North/starter lobby. Try separate floor
     * anchors and require real floor and full standing clearance at each. */
    static const float spots[6][2]={{2.5f,-2.0f},{-2.5f,-2.0f},{3.5f,0},{-3.5f,0},{2.5f,2},{-2.5f,2}};
    for(unsigned j=0;j<6;j++){
        unsigned k=(j+(variant==1?1:0))%6;SPVec3 q=su_add(origin,su_add(su_mul(right,spots[k][0]),su_mul(*forward,spots[k][1])));SPGround g;
        if(!sw_ground(&sw,su_add(q,su_mul(*up,1.65f)),*up,3.f,&g)||su_dot(g.normal,*up)<.9f)continue;
        if(su_dot(su_sub(g.point,q),*up)>.8f||su_dot(su_sub(q,g.point),*up)>1.2f)continue;
        int occupied=0;for(unsigned n=0;n<SP_STATIONS;n++)if(sp_shops[n].made&&su_sqrt(su_dot(su_sub(sp_shops[n].position,g.point),su_sub(sp_shops[n].position,g.point)))<1.2f)occupied=1;
        if(occupied||sw_overlap(su_add(g.point,su_mul(*up,.76f)),*up,.45f,1.5f))continue;
        *point=su_add(g.point,su_mul(*up,.008f));return 1;
    }return 0;
}
static void sp_shops_update(PTR world){
    int ok=1;PTR list=call(sw.stations,world,0,&ok);int count=sw_list_count(list);if(count<1||count>16)return;
    for(int i=0;i<count;i++){
        PTR collector=sw_list_item(list,i);if(!su_alive(collector))continue;void *a[]={collector};PTR id=call(sw.station_id,0,a,&ok);char name[80];su_string(id,name,sizeof(name));
        unsigned variant=su_eq(name,"station/north")?0:su_eq(name,"station/east")?2:su_eq(name,"station/south")?3:su_eq(name,"station/west")?4:99;
        if(variant>4)continue;
        for(unsigned attempt=0;attempt<(variant==0?2u:1u);attempt++){
            unsigned n=variant==0?attempt:variant;if(sp_shops[n].made)continue;SPVec3 point,forward,up;
            if(!sp_seen[n]){sp_station_event(name,n,su_vector(su.tr_get_position,sw_transform(collector)));sp_seen[n]=1;}
            if(!sp_shop_position(collector,n,&point,&forward,&up))continue;
            if(!sv_shop_make(&sp_shops[n],point,forward,up,n,sp_labels[n])){sp_event("shop-create-failed");sp_failed=1;return;}
            SPStation station={1,n,sp_prices[n],sp_core.world_id,point};sp_station(&sp_core,n,&station);sp_station_event("terminal-ready",n,point);
        }
    }
}
static void sp_world_begin(PTR world){
    sw.world=su_hold(world);sw.center=su_vector(su.tr_get_position,sw_transform(world));
    int ok=1;PTR path=call(sw.save_root,world,0,&ok);char text[1024];if(!ok||!su_string(path,text,sizeof(text))){sp_event("save-root-missing");sp_failed=1;return;}
    sp_init(&sp_core,sw_hash(text),(SPU64)GetCurrentProcessId()*1000003ull+(SPU64)sp_ticks);sp_local=sp_id(sp_local_value());
    if(!sv_materials()){sp_event("material-create-failed");sp_failed=1;return;}sp_world_ready=1;if(!sa_begin())sp_event("click-audio-unavailable");sm_begin(text);sp_event("world-ready");sp_shops_update(world);if(sp_test_tools){sp_material_probes();sp_winding_probes();}
}
static void sp_jump_sample(SVPet *v){
    /* Test-only readback from Unity's uploaded mesh, not the source pose table. */
    int ok=1;PTR value=call(su_method("UnityEngine","Mesh","get_vertices",""),su_get(v->body.mesh),0,&ok);
    if(!ok||!value)return;SURoot root=su_hold(value);U64 count=su.array_length(su_get(root));
    if(count&&count<=SP_JELLY_VERTICES){
        SPVec3 *vertices=su.array_address(su_get(root),sizeof(SPVec3),0);float bottom=vertices[0].y;
        for(U64 i=1;i<count;i++)if(vertices[i].y<bottom)bottom=vertices[i].y;
        char b[256],*p=cat(b,"{\"event\":\"jump-pose\",\"clip\":");p=num(p,v->last_clip);p=cat(p,",\"frame\":");p=num(p,v->last_frame);p=cat(p,",\"bottom\":");p=sp_fixed(p,bottom);p=cat(p,"}\n");DWORD written=0;WriteFile(sp_log,b,(DWORD)(p-b),&written,0);
    }
    su_drop(&root);
}
static void sp_visuals_update(float dt){
    for(unsigned i=0;i<SP_PLAYERS;i++){
        SPPet *pet=&sp_core.players[i];SVPet *v=&sp_visuals[i];
        sa_follow(i,pet,pet->position);
        if(!pet->active){if(v->made){sp_sound_emit(SP_AUDIO_DESPAWN,pet);sv_pet_drop(v);}sp_sound_states[i].active=0;continue;}
        if(v->made&&v->variant!=pet->variant){sv_pet_drop(v);sp_sound_states[i].active=0;}
        if(!v->made&&!sv_pet_make(v,0,pet->variant,1)){sp_event("pet-create-failed");sp_failed=1;return;}
        su_set_vector(su.tr_position,su_get(v->transform),su_sub(pet->position,su_mul(pet->up,SP_HALF_HEIGHT)));
        SPGround floor;SPVec3 visual_up=pet->up;
        if(pet->mode!=SP_HELD&&sw_ground(&sw,su_add(pet->position,su_mul(pet->up,.1f)),pet->up,.6f,&floor)&&su_dot(floor.normal,pet->up)>=SP_SLOPE_COS)visual_up=su_normal(floor.normal);
        SPVec3 visual_heading=su_normal(su_sub(pet->heading,su_mul(visual_up,su_dot(pet->heading,visual_up))));
        int local_held=pet->mode==SP_HELD&&sp_id_eq(pet->id,sp_local)&&su_alive(su_get(sp_view_camera))&&pet->owner_valid&&sp_length(sp_sub(pet->position,sp_hold_point(pet)))<.25f;
        if(pet->mode==SP_HELD){visual_up=pet->up;visual_heading=su_normal(su_add(pet->heading,su_mul(pet->up,.3f)));}
        if(local_held){
            PTR camera=sw_transform(su_get(sp_view_camera));SPVec3 eye=su_vector(su.tr_get_position,camera),forward=su_vector(su.tr_forward,camera),up=su_vector(su.tr_up,camera);
            SPVec3 held=su_add(eye,su_add(su_mul(forward,1.05f),su_mul(up,-.36f)));
            SPVec3 visual_center=su_add(held,su_mul(up,SP_HALF_HEIGHT));
            local_held=sw_clear(&sw,pet->position,visual_center,pet->up,SP_RADIUS,SP_HEIGHT,0);
            if(local_held){su_set_vector(su.tr_position,su_get(v->transform),held);
            /* Tilt the face toward the owner's eyes, softly cradled below the
             * crosshair. The shared world pose still belongs to host policy. */
            visual_heading=su_normal(su_sub(eye,su_add(held,su_mul(up,.18f))));visual_up=up;
            sa_follow(i,pet,su_add(held,su_mul(up,.18f)));}
        }
        su_set_vector(su.tr_scale,su_get(v->transform),local_held?su_v(.72f,.72f,.72f):su_v(1,1,1));
        sv_cradle(v,local_held);
        su_set_quat(su.tr_rotation,su_get(v->transform),su_rotation(visual_heading,visual_up));sv_pet_pose(v,pet->mode,pet->blink,pet->emotion,dt);sp_sound_visual(i,pet,v);
        if(sp_test_tools&&sp_ticks%6==0&&pet->mode==SP_FOLLOW&&sp_id_eq(pet->id,sp_local))sp_jump_sample(v);
    }
    for(unsigned i=0;i<SP_STATIONS;i++)sv_shop_tick(&sp_shops[i],dt);
}
static void sp_interaction_show(void *context,const char *message){sv_label_text(context,message);}
static void sp_feedback(unsigned kind,unsigned station,SPResult r){
    if(r.code==SP_OK&&(kind==SP_PURCHASE||kind==SP_SUMMON)){
        unsigned cue=kind==SP_PURCHASE?SP_AUDIO_ADOPT:SP_AUDIO_RECALL;SPPet *pet=sp_player(&sp_core,sp_local);
        if(!sm_host()&&(!pet||!pet->active||pet->variant!=r.variant)){sp_deferred_cue=cue;sp_deferred_variant=r.variant;sp_deferred_age=0;}
        else sp_sound_emit(cue,pet);
    }
    char b[320],*p=cat(b,"{\"event\":\"command\",\"kind\":");p=num(p,kind);p=cat(p,",\"station\":");p=num(p,station);p=cat(p,",\"result\":");p=num(p,r.code);p=cat(p,",\"debit\":");p=num(p,r.debit);p=cat(p,"}\n");DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(sp_log);
    if(kind==SP_PURCHASE&&station<5){SVShop *s=&sp_shops[station];
        const char *message=r.code==SP_OK?"ADOPTED\nSTILL BUY":r.code==SP_ALREADY_OWNED?"OWNED":r.code==SP_NO_FUNDS?"NEED FUNDS":r.code==SP_NO_SPACE?"NO SPACE":r.code==SP_FULL?"UNIT LIMIT":"TRY AGAIN";
        moon_interaction_feedback((MoonInteractionFeedback){&s->label,sp_interaction_show},message);s->status_time=3;
    }
}
#include "unit_execute.h"
static void sp_retire_guest(void *ctx,unsigned slot){
    (void)ctx;SPPet *pet=&sp_core.players[slot];
    if(sp_visuals[slot].made){sp_sound_emit(SP_AUDIO_DESPAWN,pet);sv_pet_drop(&sp_visuals[slot]);}
    SPPet absent=*pet;absent.active=0;sa_follow(slot,&absent,pet->position);su_zero(&sp_sound_states[slot],sizeof(sp_sound_states[slot]));
}
#include "unit_session.h"
static int sp_target_pet(PTR walker,const SPPet *pet){
    if(!moon_interaction_reach(moon_interaction_from_sp(pet->owner.position),moon_interaction_from_sp(pet->position),2.5f))return 0;
    MoonInteractionPoint origin,direction;if(!moon_interaction_unity_view(walker,&origin,&direction))return 0;
    const SVPet *v=&sp_visuals[(unsigned)(pet-sp_core.players)];
    SPVec3 center=su_add(pet->position,su_mul(pet->up,-.08f));
    if(v->made&&v->last_clip<sp_slime_asset.clip_count){
        const SPFaceFrame *face=&sp_slime_asset.clips[v->last_clip].face_frames[v->last_frame];
        center=su_add(su_vector(su.tr_get_position,su_get(v->transform)),su_mul(pet->up,(face->left_eye.y+face->right_eye.y)*.5f));
    }
    return moon_interaction_aim(moon_interaction_unity_ray,0,origin,direction,3,moon_interaction_from_sp(center),.43f,.3f);
}
static int sp_target_shop(PTR walker){
    unsigned long long objects[5]={0};for(unsigned i=0;i<5;i++)if(sp_shops[i].made)objects[i]=sw_object_id(su_get(sp_shops[i].button));
    return moon_interaction_unity_target(walker,3.5f,objects,5);
}
#include "interaction.h"
#include "test_routes.h"
#include "test_scene.h"
static void sp_nav_trace(const char *stage,SPVec3 from,SPVec3 to){
    static unsigned count;if(!sp_test_tools||!sr.active||sr.index!=10||count++>=12)return;
    SPVec3 up;sw_up(&sw,from,&up);char b[1024],*p=cat(b,"{\"event\":\"nav-diagnostic\",\"stage\":");p=sp_quote(p,stage);
    p=cat(p,",\"from\":");p=sp_vector_text(p,from);p=cat(p,",\"to\":");p=sp_vector_text(p,to);
    p=cat(p,",\"from_clear\":");p=num(p,sw_clear(&sw,from,from,up,SP_RADIUS,SP_HEIGHT,0));
    p=cat(p,",\"to_clear\":");p=num(p,sw_clear(&sw,to,to,up,SP_RADIUS,SP_HEIGHT,0));
    p=cat(p,",\"lift\":");p=sp_fixed(p,sw_crease_lift(to,up,SP_RADIUS,SP_HEIGHT,sw_slope_lift(to,up,SP_RADIUS,SP_HEIGHT)));p=cat(p,"}\n");
    DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);
}

static void sp_test_input(PTR walker,PTR world){
    if(!sp_test_tools||!sm_host())return;
    if(sp_test_aim_station){
        unsigned station=sp_test_aim_station-1;SVShop *s=&sp_shops[station];
        if(sp_test_aim_frames)sp_test_aim_frames--;
        else if(sp_test_aim_attempts<3){
            PTR camera=0;field_value(walker,sw.camera_field,&camera);SPVec3 origin=su_vector(su.tr_get_position,sw_transform(camera));
            SPVec3 target=su_add(su_vector(su.tr_get_position,su_get(s->button_transform)),su_mul(s->forward,.041f)),delta=su_sub(target,origin);
            float vertical=-su_dot(delta,s->up),horizontal=su_sqrt(su_dot(delta,delta)-vertical*vertical);int ok=1;
            void *args[]={&vertical,&horizontal};PTR angle=call(su_method("UnityEngine","Mathf","Atan2","System.Single;System.Single"),0,args,&ok);
            if(ok&&angle){
                float pitch=*(float *)unbox(angle)*57.2957795f;SPVec3 facing=su_normal(su_add(delta,su_mul(s->up,vertical))),pos=su_vector(su.tr_get_position,sw_transform(walker));
                void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);
            }
            sp_test_aim_attempts++;sp_test_aim_frames=12;
        }else{
            sp_station_event(sp_target_shop(walker)==(int)station?"test-aim-ready":"test-aim-missed",station,s->position);sp_test_aim_station=0;
        }
    }
    int funds=(sp_key(0x76)&0x8000)!=0,view=(sp_key(0x77)&0x8000)!=0,inspect=(sp_key(0x78)&0x8000)!=0,material=(sp_key(0x79)&0x8000)!=0,routes=(sp_key(0x7a)&0x8000)!=0;
    if(routes&&!sp_was_routes)sr_start();sp_was_routes=routes;
    if(funds&&!sp_was_funds){
        int value=2000,ok=1;unsigned char decimal[16];su_zero(decimal,sizeof(decimal));void *ctor[]={&value};call(sw.decimal_int,decimal,ctor,&ok);
        void *args[]={decimal};call(method("MoonCleaners","MoonWorld","SetTeamDollars",1),world,args,&ok);sp_event(ok?"test-funds-ready":"test-funds-failed");
    }
    if(view&&!sp_was_view){
        unsigned station=sp_test_station++%5;SVShop *s=&sp_shops[station];if(s->made){
            SPVec3 pos=su_add(s->position,su_add(su_mul(s->forward,1.6f),su_add(su_mul(su_cross(s->up,s->forward),.29f),su_mul(s->up,.95f)))),facing=su_mul(s->forward,-1);float pitch=31;int ok=1;unsigned char refresh=1;
            void *teleport[]={&pos,&refresh};call(method("MoonCleaners","MoonWalker","Teleport",2),walker,teleport,&ok);
            void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);
            sp_test_aim_station=station+1;sp_test_aim_frames=18;sp_test_aim_attempts=0;
            sp_station_event(ok?"test-terminal-view":"test-terminal-view-failed",station,pos);
        }
    }
    if(inspect&&!sp_was_inspect){
        SPPet *pet=sp_player(&sp_core,sp_local);if(pet&&pet->active){
            SPVec3 center=su_add(pet->position,su_mul(pet->heading,1.5f));SPGround g;
            if(sw_ground(&sw,su_add(center,su_mul(pet->up,.4f)),pet->up,2,&g)){
                SPVec3 pos=su_add(g.point,su_mul(pet->up,.95f)),facing=su_mul(pet->heading,-1);float pitch=43;int ok=1;
                if(sw_clear(&sw,pos,pos,pet->up,.4f,1.8f,g.support_id)){void *place[]={&pos,&facing,&pitch};call(method("MoonCleaners","MoonWalker","Place",3),walker,place,&ok);sp_event(ok?"test-pet-view":"test-pet-view-failed");}
            }
        }
    }
    if(material&&!sp_was_material){
        static const unsigned cull[]={2,1,0};sp_test_cull=(sp_test_cull+1)%3;
        for(unsigned i=0;i<5;i++)sv_integer(su_get(sv_pet_materials[i]),"_Cull",(int)(sp_test_cull?cull[sp_test_cull]:i==2?0:2));
        sp_material_probes();
    }
    sp_was_funds=funds;sp_was_view=view;sp_was_inspect=inspect;sp_was_material=material;
}
static void sp_sample(void){
    SPPet *pet=sp_player(&sp_core,sp_local);if(!pet)return;char b[512],*p=cat(b,"{\"event\":\"pet-sample\",\"owned\":");p=num(p,pet->owned);p=cat(p,",\"active\":");p=num(p,pet->active);p=cat(p,",\"variant\":");p=num(p,pet->variant);p=cat(p,",\"mode\":");p=num(p,pet->mode);p=cat(p,",\"clip\":");SVPet *v=&sp_visuals[(unsigned)(pet-sp_core.players)];p=num(p,v->made?v->last_clip:~0u);p=cat(p,",\"balance\":");p=num(p,pet->balance);p=cat(p,",\"owner\":");p=sp_vector_text(p,pet->owner.position);p=cat(p,",\"pet\":");p=sp_vector_text(p,pet->position);p=cat(p,",\"casts\":");p=num(p,sw.raycasts);p=cat(p,",\"sweeps\":");p=num(p,sw.sweeps);p=cat(p,"}\n");DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(sp_log);
}
static int sp_before(int id,Context *c){
    (void)id;if(!__atomic_load_n(&sp_initialized,__ATOMIC_ACQUIRE)||sp_failed)return 0;
    if(!sp_thread){sp_thread=(int)GetCurrentThreadId();sp_event("main-thread");}if((DWORD)sp_thread!=GetCurrentThreadId())return 0;
    sp_ticks++;PTR walker=(PTR)c->rcx,world=0;field_value(walker,sw.world_field,&world);
    if(sw.world&&(!su_alive(su_get(sw.world))||su_get(sw.world)!=world))sp_world_drop();if(!world)return 0;
    int ok=1;if(boolean(sw.restoring,world,&ok)||!ok){sp_ready_frames=0;return 0;}
    if(!sp_world_ready){if(++sp_ready_frames<120)return 0;sp_world_begin(world);if(!sp_world_ready||sp_failed)return 0;}
    PTR view_camera=0;field_value(walker,sw.camera_field,&view_camera);
    if(view_camera!=su_get(sp_view_camera)){su_drop(&sp_view_camera);sp_view_camera=su_hold(view_camera);}
    if(sm_host()&&!sp_owner_update(walker,world)){SPPet *local=sp_player(&sp_core,sp_local);if(local)local->owner_valid=0;}
    float dt=su_number(su.time_delta,0);if(!sp_finite(dt)||dt<0)dt=0;if(dt>.1f)dt=.1f;
    float real_dt=su_number(sp_real_delta,0);if(!sp_finite(real_dt)||real_dt<0)real_dt=0;
    if(sp_ticks%120==0)sp_shops_update(world);sm_tick(world,real_dt);
    if(!sm_host()&&!sp_owner_update(walker,world)){SPPet *local=sp_player(&sp_core,sp_local);if(local)local->owner_valid=0;}
    if(sp_deferred_cue){
        sp_deferred_age+=real_dt;SPPet *pet=sp_player(&sp_core,sp_local);
        if(pet&&pet->active&&pet->variant==sp_deferred_variant){sp_sound_emit(sp_deferred_cue,pet);sp_deferred_cue=0;}
        else if(sp_deferred_age>2||sm.suspended)sp_deferred_cue=0;
    }
    SPPet *carried=sp_player(&sp_core,sp_local);sc_carry_view(walker,(carried&&carried->active&&carried->mode==SP_HELD)||(sm.pending&&sm.pending_kind==SP_PICKUP)||sm.pickup_accepted);
    sp_visuals_update(dt);sr_tick(dt);
    DWORD pid=0;sp_window_pid(sp_foreground(),&pid);int focused=pid==GetCurrentProcessId()&&boolean(sp_focus,0,&ok);
    int allow=focused&&!boolean(sw.blocked,walker,&ok)&&!boolean(sw.typing,0,&ok)&&boolean(sw.firstperson,walker,&ok)&&ok;
    if(sp_test_tools&&sm_host()&&focused&&!boolean(sw.typing,0,&ok)&&ok)sc_input(walker,allow);
    int summon_control=moon_input_action("slime-pet.summon","F6",1).held;
    int shop_request=moon_shop_poll();
    if(shop_request&&allow){
        su_destroy(su_get(sp_shop_hint_root));su_drop(&sp_shop_hint_root);
        PTR camera=0;field_value(walker,sw.camera_field,&camera);PTR go=su_gameobject("MoonLab slime adoption guide",sw_transform(camera));
        sp_shop_hint_root=su_hold(go);su_set_vector(su.tr_local_position,su_transform(go),su_v(-.55f,.3f,1.5f));
        PTR text=su_add_component(go,"TextMesh");su_int(su_method("UnityEngine","TextMesh","set_fontSize","System.Int32"),text,48);
        su_float(su_method("UnityEngine","TextMesh","set_characterSize","System.Single"),text,.04f);
        char guide[384];if(!sp_controls_guide(guide,sizeof(guide)))cat(guide,"SLIME ADOPTION\nInput service unavailable.");
        cat(sp_shop_hint_caption,guide);int text_ok=1;void *caption[]={su.string(guide)};
        call(su_method("UnityEngine","TextMesh","set_text","System.String"),text,caption,&text_ok);sp_shop_hint_time=8;
    }
    if(sp_shop_hint_time>0){
        char guide[384];if(sp_controls_guide(guide,sizeof(guide))&&!su_eq(guide,sp_shop_hint_caption)){
            int text_ok=1;void *caption[]={su.string(guide)};PTR text=su_component(su_get(sp_shop_hint_root),"TextMesh");
            call(su_method("UnityEngine","TextMesh","set_text","System.String"),text,caption,&text_ok);if(text_ok)cat(sp_shop_hint_caption,guide);
        }
        sp_shop_hint_time-=dt;if(sp_shop_hint_time<=0){su_destroy(su_get(sp_shop_hint_root));su_drop(&sp_shop_hint_root);}}
    if(allow)sp_test_input(walker,world);if(sp_ticks%120==0)sp_sample();
    int summon=summon_control;
    int target=allow?sp_target_shop(walker):-1;
    for(unsigned i=0;i<5;i++)if(sp_shops[i].made)su_bool(method("UnityEngine","GameObject","SetActive",1),su_get(sp_shops[i].focus),target==(int)i||sp_shops[i].control.grace_remaining>0);
    if(allow&&summon&&!sp_was_summon)sp_command_local(world,SP_SUMMON,0);
    MoonInteractionState interaction=moon_interaction_sample(sp_interactions[0]),alternate=moon_interaction_sample(sp_interactions[1]);sp_interaction_hint(walker,interaction);
    /* One offer wins a physical key; the local pet also takes precedence over
     * BUY on another key in the same frame. A failed carry never buys behind it. */
    MoonInteractionState action=interaction.input.pressed?interaction:alternate;
    if(interaction.input.pressed)(void)moon_interaction_take(sp_interactions[1],walker);
    if(allow&&action.input.pressed&&moon_interaction_take(sp_interactions[interaction.input.pressed?0:1],walker)){
        MoonInteractionTarget current={0};void *context=interaction.input.pressed?0:(void *)1;
        if(sp_interaction_query(context,walker,&current)&&current.identity==action.target&&current.token==action.token){
            if(action.token==SP_INTERACT_DROP)sp_command_local(world,SP_DROP,0);
            else if(action.token==SP_INTERACT_PICKUP){if(sc_carry_claim(walker))sp_command_local(world,SP_PICKUP,0);}
            else if(action.token>=SP_INTERACT_SHOP&&action.token<SP_INTERACT_SHOP+5){unsigned shop=action.token-SP_INTERACT_SHOP;ms_shop_control_press(&sp_shops[shop].control);sa_click(su_get(sp_shops[shop].button_transform));sp_command_local(world,SP_PURCHASE,shop);}
        }
    }
    sp_was_summon=summon;return 0;
}
static void sp_unit_update(PTR walker){Context context={0};context.rcx=(U64)walker;sp_before(0,&context);}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    char folder[1000],path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,1000);if(!n||n>=1000)return 0;
    cat(cat(path,folder),"\\slime-pet.jsonl");sp_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(sp_log==(PTR)(U64)-1)return 0;
    (void)prepare_hook;(void)install_hooks;
    if(!bind_runtime(domain,game)||!sp_inspect(game,folder)){sp_event("bind-failed");return 0;}
    if(!su_bind(game)){sp_event("unity-bind-failed");sp_event(su.failed?su.failed:"unknown");return 0;}sp_event("unity-bound");
    if(!sw_bind()){sp_event("world-bind-failed");return 0;}
    sw.support_in_use=sp_support_in_use;
    if(!sa_setup(domain,game)){sp_event("sound-framework-unavailable");return 0;}
    sc_hands=mh_connect(domain,game);if(!sc_hands){sp_event("hands-framework-unavailable");return 0;}
    sp_units=mu_connect(domain,game);sp_contacts=mu_connect_contacts();if(!sp_units||!ss_bind()){sp_event("session-bind-failed");return 0;}
    sp_event(sp_contacts?"shared-contacts-ready":"shared-contacts-fallback");
    char test[8];DWORD test_n=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test,sizeof(test));sp_test_tools=test_n==1&&test[0]=='1';
    test_n=GetEnvironmentVariableA("MOONLAB_SLIME_TEST_TOOLS",test,sizeof(test));if(test_n)sp_test_tools=test_n==1&&test[0]=='1';
    PTR user=LoadLibraryA("user32.dll");if(!user)return 0;sp_key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");sp_foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");sp_window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");if(!sp_key||!sp_foreground||!sp_window_pid)return 0;
    sp_frame=method("UnityEngine","Time","get_frameCount",0);sp_focus=method("UnityEngine","Application","get_isFocused",0);sp_real_delta=method("UnityEngine","Time","get_unscaledDeltaTime",0);
    if(!sp_interactions_bind(domain,game)){sp_event("interaction-framework-unavailable");return 0;}
    if(!sp_frame||!sp_focus||!sp_real_delta){sp_event("update-registration-failed");return 0;}
    __atomic_store_n(&sp_initialized,1,__ATOMIC_RELEASE);
    MoonUnitProvider provider={sizeof(provider),3,sm_snapshot,sm_receive,sm_reset,sp_unit_update,sm_command,sm_result};
    if(!sp_units->register_provider(&provider)){sp_event("update-registration-failed");return 0;}
    moon_shop_listen("slime-pet");sp_event("ready");return 1;
}
