/* Private engine qualification: an independent copy of the production policy
 * runs against the real world adapter and actual Unity colliders. No wallet,
 * ownership ledger or network state is injected into the player's companion.
 * Temporary fixtures are destroyed before world teardown and never saved. */
#ifndef SLIME_TEST_ROUTES_H
#define SLIME_TEST_ROUTES_H
static struct {
    SPCore core;SVPet visual;SURoot boxes[4];SPVec3 base,right,up,forward,start,goal;
    SPVec3 terrain;float time,delay,max_side,max_height,initial_dust;unsigned active,index,phase,failures;
} sr;
static const char *sr_names[]={"starter-interior","dusty-terrain","cleaned-terrain","solid-obstacle-detour","raised-threshold","low-ceiling-recall","moving-rotating-support","removed-sleeping-support","sleeping-dust-cleanup","ramp-ascent","ramp-descent"};
static void sr_log(const char *event,int pass,float error){
    char b[768],*p=cat(b,"{\"event\":");p=sp_quote(p,event);p=cat(p,",\"case\":");p=sp_quote(p,sr_names[sr.index]);
    p=cat(p,",\"index\":");p=num(p,sr.index);p=cat(p,",\"pass\":");p=cat(p,pass?"true":"false");
    p=cat(p,",\"error\":");p=sp_fixed(p,error);p=cat(p,",\"max_side\":");p=sp_fixed(p,sr.max_side);
    p=cat(p,",\"max_height\":");p=sp_fixed(p,sr.max_height);p=cat(p,",\"start\":");p=sp_vector_text(p,sr.start);
    p=cat(p,",\"goal\":");p=sp_vector_text(p,sr.goal);p=cat(p,",\"pet\":");p=sp_vector_text(p,sr.core.players[0].position);
    p=cat(p,",\"support_valid\":");p=num(p,sr.core.players[0].support_valid);
    p=cat(p,",\"support_known\":");p=num(p,sr.core.players[0].support_id!=0);
    p=cat(p,",\"support_origin\":");p=sp_vector_text(p,sr.core.players[0].support_frame.origin);
    p=cat(p,"}\n");DWORD n=0;WriteFile(sp_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(sp_log);
}
static void sr_clear(void){
    if(sr.visual.made)sv_pet_drop(&sr.visual);
    for(unsigned i=0;i<4;i++){su_destroy(su_get(sr.boxes[i]));su_drop(&sr.boxes[i]);}
}
static void sr_drop(void){sr_clear();sr.active=0;}
static SPVec3 sr_point(float x,float y,float z){return su_add(sr.base,su_add(su_mul(sr.right,x),su_add(su_mul(sr.up,y),su_mul(sr.forward,z))));}
static int sr_box(unsigned index,SPVec3 center,SPVec3 size){
    int kind=3,ok=1;void *args[]={&kind};PTR go=call(su.primitive,0,args,&ok);if(!ok||!go)return 0;
    sr.boxes[index]=su_hold(go);PTR tr=su_transform(go);su_set_vector(su.tr_position,tr,center);
    su_set_quat(su.tr_rotation,tr,su_rotation(sr.forward,sr.up));su_set_vector(su.tr_scale,tr,size);
    sv_assign_materials(su_component(go,"Renderer"),&sv_pet_materials[3],1);return 1;
}
static int sr_floor(SPVec3 point,SPGround *g){SPVec3 up;if(!sw_up(&sw,point,&up))return 0;return sw_ground(&sw,su_add(point,su_mul(up,2)),up,6,g);}
static void sr_seed(SPVec3 point,SPVec3 goal,int owner){
    sp_init(&sr.core,1,1);sp_join(&sr.core,(SPId){1,0},1000);SPPet *p=&sr.core.players[0];SPGround g;
    if(!sr_floor(point,&g))return;
    p->owned=p->active=1;p->variant=0;p->position=su_add(g.point,su_mul(sr.up,SP_HALF_HEIGHT));p->up=sr.up;p->heading=sr.right;
    p->owner=(SPOwner){1,goal,sr.up,sr.right};p->owner_valid=(unsigned)owner;p->idle_time=owner?0:9;
    SPWorld w=sw_callbacks();sp_contact(p,&w,&g);sr.start=p->position;sr.goal=goal;
    sv_pet_make(&sr.visual,0,0,1);
}
static int sr_find_terrain(void){
    SVShop *s=&sp_shops[0];SPVec3 right=su_cross(s->up,s->forward);
    for(unsigned ring=0;ring<3;ring++)for(unsigned direction=0;direction<8;direction++){
        static const float x[]={1,1,0,-1,-1,-1,0,1},z[]={0,1,1,1,0,-1,-1,-1};
        float radius=35+(float)ring*20;SPVec3 point=su_add(s->position,su_add(su_mul(right,x[direction]*radius),su_mul(s->forward,z[direction]*radius))),up;SPGround g;
        if(sw_up(&sw,point,&up)&&sw_ground(&sw,su_add(point,su_mul(up,30)),up,65,&g)&&!g.support_id&&su_dot(g.normal,up)>.96f){
            sr.terrain=g.point;return 1;
        }
    }
    return 0;
}
static void sr_setup(void){
    sr_clear();sr.time=sr.delay=sr.max_side=sr.max_height=0;sr.phase=0;
    sr.up=sp_shops[0].up;sr.forward=sp_shops[0].forward;sr.right=su_cross(sr.up,sr.forward);
    sr.base=sr.index==0?su_add(sp_shops[0].position,su_mul(sr.forward,6)):sr.terrain;
    if(sr.index==2)sr.base=su_add(sr.base,su_mul(sr.forward,15));
    if((sr.index>=3&&sr.index<=7)||sr.index>=9){sr.base=su_add(sr.base,su_mul(sr.up,3));sr_box(0,sr_point(0,-.25f,0),su_v(14,.5f,10));}
    if(sr.index>=9){
        sr_box(1,sr_point(0,.65f,0),su_v(6,.2f,5));
        SPVec3 normal=su_normal(su_sub(sr.up,su_mul(sr.right,.2f)));
        su_set_quat(su.tr_rotation,su_transform(su_get(sr.boxes[1])),su_rotation(sr.forward,normal));
        /* Rotation shortens the ramp footprint: the furthest corner is
         * x=2.9614, not 3. Join with overlap, avoiding an unintended 39mm
         * unsupported gap in what is meant to be a continuous-ramp case. */
        sr_box(2,sr_point(3.925f,.66f,0),su_v(2.15f,1.32f,5));
    }
    if(sr.index==3)sr_box(1,sr_point(0,.8f,0),su_v(.6f,1.6f,2.6f));
    if(sr.index==4)sr_box(1,sr_point(2,.125f,0),su_v(5,.25f,5));
    if(sr.index==5)sr_box(1,sr_point(1,.65f,0),su_v(9,.3f,7));
    if(sr.index==6||sr.index==7)sr_box(1,sr_point(0,.5f,0),su_v(5,1,5));
    if(sr.index==2){int ok=1;float radius=9,strength=100;SPVec3 p=sr.base;void *a[]={&p,&radius,&strength};call(su_method("MoonCleaners","MoonWorld","CleanAt","UnityEngine.Vector3;System.Single;System.Single"),su_get(sw.world),a,&ok);}
    sr.delay=1;sr_log("route-setup",1,0);
}
static void sr_start(void){
    sr_drop();sr.index=0;sr.failures=0;sr.active=1;
    if(!sr_find_terrain()){sr_log("route-no-loaded-exterior",0,0);sr.active=0;return;}
    sr_setup();
}
static void sr_finish(int pass,float error){
    sr_log("route-result",pass,error);if(!pass)sr.failures++;
    if(++sr.index>=sizeof(sr_names)/sizeof(*sr_names)){sr.index--;sr_log("route-suite-complete",sr.failures==0,(float)sr.failures);sr_drop();return;}
    sr_setup();
}
static void sr_tick(float dt){
    if(!sp_test_tools||!sr.active)return;
    int ok=1;call(su_method("UnityEngine","Physics","SyncTransforms",""),0,0,&ok);
    if(sr.delay>0){sr.delay-=dt;if(sr.delay>0)return;
        SPVec3 start=sr_point(-3,0,0),goal=sr_point(4,0,0);
        if(sr.index==4)goal=sr_point(4,.25f,0);
        if(sr.index>=6&&sr.index<=8){start=sr.index<=7?sr_point(.6f,1,0):sr.base;goal=start;}
        if(sr.index==9){start=sr_point(-4,0,0);goal=sr_point(6,1.32f,0);}
        if(sr.index==10){start=sr_point(4,1.32f,0);goal=sr_point(-6,0,0);}
        sr_seed(start,goal,sr.index<5||sr.index>=9);if(!sr.core.players[0].active){sr_finish(0,99);return;}
        sr.initial_dust=0;
        if(sr.index==8){void *a[]={&start};PTR v=call(sw.dust_height,su_get(sw.world),a,&ok);sr.initial_dust=ok&&v?*(float *)unbox(v):0;}
        sr_log("route-start",1,0);
    }
    sr.time+=dt;SPPet *p=&sr.core.players[0];SPWorld w=sw_callbacks();
    if(sr.index==5){
        p->owner=(SPOwner){1,sr_point(1,0,0),sr.up,sr.right};p->owner_valid=1;SPCommand c={1,1,1,SP_SUMMON,0};SPResult r=sp_command(&sr.core,p->id,&c,&w);
        int occupied=!sw_clear(&sw,sr_point(1,.35f,0),sr_point(1,.35f,0),sr.up,SP_RADIUS,SP_HEIGHT,0);
        sr_finish(occupied&&r.code==SP_NO_SPACE,(float)r.code);return;
    }
    if(sr.index==6){
        float f=sr.time/5;if(f>1)f=1;SPVec3 forward=su_normal(su_add(su_mul(sr.forward,1-f),su_mul(sr.right,f)));
        PTR tr=su_transform(su_get(sr.boxes[1]));su_set_vector(su.tr_position,tr,sr_point(f*.8f,.5f,0));su_set_quat(su.tr_rotation,tr,su_rotation(forward,sr.up));
        call(su_method("UnityEngine","Physics","SyncTransforms",""),0,0,&ok);
    }
    if(sr.index==7&&!sr.phase&&sr.time>1){su_destroy(su_get(sr.boxes[1]));su_drop(&sr.boxes[1]);sr.phase=1;}
    if(sr.index==8&&!sr.phase&&sr.time>1){float radius=6,strength=100;SPVec3 point=sr.base;void *a[]={&point,&radius,&strength};call(su_method("MoonCleaners","MoonWorld","CleanAt","UnityEngine.Vector3;System.Single;System.Single"),su_get(sw.world),a,&ok);sr.phase=1;}
    sp_tick(&sr.core,dt,&w);
    float side=sp_abs(su_dot(su_sub(p->position,sr.base),sr.forward)),height=su_dot(su_sub(p->position,sr.start),sr.up);
    if(side>sr.max_side)sr.max_side=side;if(height>sr.max_height)sr.max_height=height;
    su_set_vector(su.tr_position,su_get(sr.visual.transform),su_sub(p->position,su_mul(p->up,SP_HALF_HEIGHT)));
    su_set_quat(su.tr_rotation,su_get(sr.visual.transform),su_rotation(p->heading,p->up));sv_pet_pose(&sr.visual,p->mode,p->blink,p->emotion,dt);
    if(sr.index<5||sr.index>=9){float error=sp_length(su_sub(p->position,sr.goal));
        if(error<2){sr_finish(sr.index!=3||sr.max_side>1.4f,error);return;}
        if(sr.time>12){sr_finish(0,error);return;}
    }else if(sr.time>6){
        SPVec3 expected=sr.index==6?sr_point(.8f,1.35f,-.6f):sr_point(.6f,.35f,0);float error=sp_length(su_sub(p->position,expected));int pass=error<.1f;
        if(sr.index==8){SPGround g;void *a[]={&sr.base};PTR v=call(sw.dust_height,su_get(sw.world),a,&ok);float dust=ok&&v?*(float *)unbox(v):999;
            pass=sr.initial_dust>.05f&&dust<sr.initial_dust-.02f&&sr_floor(sr.base,&g);error=pass?sp_length(su_sub(p->position,su_add(g.point,su_mul(p->up,.35f)))):999;pass=pass&&error<.08f;}
        sr_finish(pass,error);
    }
}
#endif
