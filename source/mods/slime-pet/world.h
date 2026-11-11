#ifndef SLIME_WORLD_H
#define SLIME_WORLD_H
typedef struct {SURoot transform;SPU64 id,object_id,stamp;} SWSupport;
typedef struct {
    SURoot world,hit_array;SPVec3 center;
    PTR component_transform,instance_id,hit_transform,world_field,camera_field;
    PTR stations,station_id,find_objects,wallet,charge,decimal_int,decimal_long,save_root,request_save;
    PTR restoring,blocked,typing,firstperson,is_terrain,bedrock,dust_height;
    SWSupport supports[128];unsigned support_count;SPU64 support_clock,support_serial;int (*support_in_use)(SPU64);SPU32 raycasts,sweeps,rejected;
} SWContext;
static SWContext sw;
static inline PTR sw_transform(PTR component){int ok=1;return call(sw.component_transform,component,0,&ok);}
static inline SPU64 sw_object_id(PTR obj){int ok=1;int id=integer(sw.instance_id,obj,&ok);return ok?(SPU64)(unsigned)id:0;}
static inline int sw_up(void *ctx,SPVec3 point,SPVec3 *up){SWContext *w=ctx;return sp_unit(sp_sub(point,w->center),up);}
static inline SPU64 sw_support_register(PTR transform){
    SPU64 object_id=sw_object_id(transform);if(!object_id)return 0;
    /* Unity can reuse a destroyed object's instance ID. A pet must never bind
     * that old contact to a different platform or retain its destroyed wrapper. */
    for(unsigned i=0;i<sw.support_count;i++)if(sw.supports[i].object_id==object_id&&su_alive(su_get(sw.supports[i].transform))){sw.supports[i].stamp=++sw.support_clock;return sw.supports[i].id;}
    if(sw.support_serial==~(SPU64)0)return 0;
    SURoot held=su_hold(transform);if(!held)return 0;
    unsigned slot=sw.support_count;
    if(slot>=128){
        SPU64 oldest=~(SPU64)0;slot=128;
        for(unsigned i=0;i<128;i++)if((!sw.support_in_use||!sw.support_in_use(sw.supports[i].id))&&sw.supports[i].stamp<oldest){slot=i;oldest=sw.supports[i].stamp;}
        if(slot==128){su_drop(&held);return 0;}su_drop(&sw.supports[slot].transform);
    }else sw.support_count++;
    SWSupport *s=&sw.supports[slot];s->id=++sw.support_serial;s->object_id=object_id;s->stamp=++sw.support_clock;s->transform=held;return s->id;
}
static inline int sw_ray(SPVec3 origin,SPVec3 direction,float distance,unsigned char *hit){
    int ok=1,mask=~(1<<2),triggers=1;void *args[]={&origin,&direction,hit,&distance,&mask,&triggers};sw.raycasts++;
    PTR result=call(su.physics_ray,0,args,&ok);return ok&&result&&*(unsigned char *)unbox(result);
}
static inline int sw_terrain_contact(SPVec3 origin,SPVec3 up,float drop,SPGround *out,float *probe){
    int ok=1;SPVec3 normal={0},bare;void *sample[]={&origin,&normal};PTR stone=call(sw.bedrock,su_get(sw.world),sample,&ok);
    void *dust_sample[]={&origin};PTR dust=call(sw.dust_height,su_get(sw.world),dust_sample,&ok);
    if(!ok||!stone||!dust)return 0;su_copy(&bare,unbox(stone),sizeof(bare));float height=*(float *)unbox(dust);
    if(!sp_vec_valid(bare)||!sp_unit(normal,&normal)||!sp_finite(height)||height<0||height>20)return 0;
    float slope=su_dot(normal,up);if(slope<SP_SLOPE_COS)return 0;
    float offset=height/slope,distance=su_dot(su_sub(origin,bare),up)-offset;
    if(!sp_finite(distance)||distance<0||distance>drop)return 0;
    out->point=su_sub(origin,su_mul(up,distance));out->normal=normal;out->support_id=0;
    *probe=drop+offset;return sp_vec_valid(out->point);
}
static inline int sw_ground(void *ctx,SPVec3 origin,SPVec3 up,float drop,SPGround *out){
    (void)ctx;union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));
    if(!out||!sp_vec_valid(origin)||!sp_vec_valid(up)||!sp_finite(drop)||drop<=0)return 0;
    SPGround candidate;float probe=drop;int extended=0;
    if(!sw_ray(origin,su_mul(up,-1),drop,hit.bytes)){
        /* Dust can hide bedrock beyond the requested visible-floor range.
         * Extend only by measured thickness and still require an actual
         * terrain collider. A height field alone never proves loaded floor. */
        if(!sw_terrain_contact(origin,up,drop,&candidate,&probe)||probe<=drop||!sw_ray(origin,su_mul(up,-1),probe,hit.bytes))return 0;
        extended=1;
    }
    int terrain_ok=1;PTR collider=call(su.hit_collider,hit.bytes,0,&terrain_ok);void *terrain_args[]={collider};
    PTR terrain=call(sw.is_terrain,su_get(sw.world),terrain_args,&terrain_ok);
    if(!terrain_ok||!terrain)return 0;
    if(*(unsigned char *)unbox(terrain)){
        if(extended){*out=candidate;return 1;}
        return sw_terrain_contact(origin,up,drop,out,&probe);
    }
    /* An extended probe must not turn a distant building into a nearby floor. */
    if(extended)return 0;
    su_copy(&out->point,hit.bytes+su.hit_point,sizeof(SPVec3));su_copy(&out->normal,hit.bytes+su.hit_normal,sizeof(SPVec3));
    int ok=1;PTR tr=call(sw.hit_transform,hit.bytes,0,&ok);out->support_id=ok&&tr?sw_support_register(tr):0;
    return sp_vec_valid(out->point)&&sp_vec_valid(out->normal);
}
static inline int sw_overlap(SPVec3 center,SPVec3 up,float radius,float height){
    /* A 12mm skin permits floor contact while retaining walls/ceilings. */
    float r=radius-.012f;float half=height*.5f-radius;
    SPVec3 a=su_add(center,su_mul(up,half)),b=su_sub(center,su_mul(up,half));int ok=1,mask=~(1<<2),triggers=1;
    void *args[]={&a,&b,&r,&mask,&triggers};PTR result=call(su.physics_capsule,0,args,&ok);return !ok||!result||*(unsigned char *)unbox(result);
}
static inline float sw_slope_lift(SPVec3 center,SPVec3 up,float radius,float height){
    SPGround ground;if(!sw_ground(&sw,su_add(center,su_mul(up,.1f)),up,height*.5f+1,&ground))return 0;
    if(su_dot(su_sub(center,ground.point),up)>height*.5f+.12f)return 0;
    float cosine=su_dot(ground.normal,up);if(cosine<SP_SLOPE_COS||cosine>1)return 0;
    /* The logical foot remains on the ray. Raising the collision capsule by
     * r(1/cos-1) gives its lower sphere the promised slope clearance. */
    return radius*(1/cosine-1);
}
static inline int sw_cast_clear(SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height){
    if(sw_overlap(to,up,radius,height))return 0;
    SPVec3 delta=su_sub(to,from);float length=su_sqrt(su_dot(delta,delta));if(length<.0001f)return 1;
    SPVec3 direction=su_mul(delta,1/length),a=su_add(from,su_mul(up,height*.5f-radius)),b=su_sub(from,su_mul(up,height*.5f-radius));
    float r=radius-.012f;int ok=1,mask=~(1<<2),triggers=1;PTR hits=su_get(sw.hit_array);
    void *args[]={&a,&b,&r,&direction,hits,&length,&mask,&triggers};PTR result=call(su.physics_cast,0,args,&ok);
    if(!ok||!result)return 0;return *(int *)unbox(result)==0;
}
/* At a ramp/landing join the centre ray can see the lower plane while the
 * capsule touches the neighbouring ramp. Retry only against witnessed nearby
 * walkable planes, with a bounded clearance lift; never move the logical foot.
 * This extra work occurs only after the ordinary complete capsule query fails. */
static inline float sw_crease_lift(SPVec3 center,SPVec3 up,float radius,float height,float base){
    SPGround ground;if(!sw_ground(&sw,su_add(center,su_mul(up,.1f)),up,height*.5f+.2f,&ground))return base;
    if(su_dot(su_sub(center,ground.point),up)>height*.5f+.08f)return base;
    SPVec3 normal=su_normal(ground.normal),axis=su_normal(su_cross(up,up.x<.8f&&up.x>-.8f?su_v(1,0,0):su_v(0,0,1)));
    SPVec3 right=su_cross(up,axis),sphere=su_sub(center,su_mul(up,height*.5f-radius));float lift=base;
    for(unsigned i=0;i<4;i++){
        SPVec3 offset=su_mul(i<2?axis:right,(i%2?1.f:-1.f)*radius);SPGround sample;
        if(!sw_ground(&sw,su_add(su_add(center,offset),su_mul(up,.1f)),up,height*.5f+.2f,&sample))continue;
        SPVec3 n=su_normal(sample.normal);float cosine=su_dot(n,up);
        if(cosine<SP_SLOPE_COS)continue;
        if(su_dot(normal,n)>.9995f&&sp_abs(su_dot(su_sub(sample.point,ground.point),normal))<.02f)continue;
        float need=(radius-su_dot(su_sub(sphere,sample.point),n))/cosine;
        if(need>lift&&need<=base+.08f)lift=need;
    }
    return lift;
}
static inline int sw_clear(void *ctx,SPVec3 from,SPVec3 to,SPVec3 up,float radius,float height,SPU64 support_id){
    (void)ctx;(void)support_id;sw.sweeps++;
    float a=sw_slope_lift(from,up,radius,height),b=sw_slope_lift(to,up,radius,height);
    SPVec3 start=su_add(from,su_mul(up,a)),end=su_add(to,su_mul(up,b));
    if(sw_cast_clear(start,end,up,radius,height))return 1;
    float raised_a=sw_crease_lift(from,up,radius,height,a),raised_b=sw_crease_lift(to,up,radius,height,b);
    if(raised_a<=a+.0001f&&raised_b<=b+.0001f)return 0;
    return sw_cast_clear(su_add(from,su_mul(up,raised_a)),su_add(to,su_mul(up,raised_b)),up,radius,height);
}
static inline int sw_support(void *ctx,SPU64 id,SPTransform *out){
    SWContext *w=ctx;for(unsigned i=0;i<w->support_count;i++)if(w->supports[i].id==id){
        PTR tr=su_get(w->supports[i].transform);if(!su_alive(tr))return 0;
        out->origin=su_vector(su.tr_get_position,tr);out->right=su_vector(su.tr_right,tr);out->up=su_vector(su.tr_up,tr);out->forward=su_vector(su.tr_forward,tr);return 1;
    }
    return 0;
}
static inline SPWorld sw_callbacks(void){return (SPWorld){&sw,sw_up,sw_ground,sw_clear,sw_support};}
static inline SPU64 sw_hash(const char *text){SPU64 hash=1469598103934665603ull;while(*text){hash^=(unsigned char)*text++;hash*=1099511628211ull;}return hash?hash:1;}
static inline SPU32 sw_balance(PTR world,PTR player_id,int *ok){
    void *args[]={player_id};PTR money=call(sw.wallet,world,args,ok);if(!*ok||!money){*ok=0;return 0;}
    void *convert[]={unbox(money)};PTR value=call(sw.decimal_long,0,convert,ok);if(!*ok||!value){*ok=0;return 0;}
    long long amount=*(long long *)unbox(value);if(amount<0||amount>SP_MAX_BALANCE){*ok=0;return 0;}return (SPU32)amount;
}
#include "../../runtime/sdk/wallet.h"
typedef struct {PTR world,player;} SWWalletContext;
static inline int sw_wallet_balance(void *context,long long *amount){SWWalletContext *c=context;int ok=1;*amount=sw_balance(c->world,c->player,&ok);return ok;}
static inline int sw_wallet_adjust(void *context,int delta){SWWalletContext *c=context;int ok=1;unsigned char decimal[16];su_zero(decimal,sizeof(decimal));void *ctor[]={&delta};call(sw.decimal_int,decimal,ctor,&ok);void *args[]={c->player,decimal};if(ok)call(sw.charge,c->world,args,&ok);return ok;}
static inline int sw_debit(PTR world,PTR player_id,SPU32 cost){
    if(!cost)return 1;if(cost>2147483647u)return 0;SWWalletContext context={world,player_id};
    const MoonWallet wallet={sw_wallet_balance,sw_wallet_adjust};
    return mw_debit(&wallet,&context,(int)cost,0)==MW_PAID;
}
static inline PTR sw_list_item(PTR list,int index){int ok=1;PTR m=get_method(object_class(list),"get_Item",1);void *a[]={&index};return call(m,list,a,&ok);}
static inline int sw_list_count(PTR list){int ok=1;return list?integer(get_method(object_class(list),"get_Count",0),list,&ok):0;}
static inline int sw_bind(void){
#define SW_BIND(F,T,N,S) sw.F=su_method("MoonCleaners",T,N,S);if(!sw.F)return 0
    SW_BIND(stations,"MoonWorld","get_Stations","");SW_BIND(station_id,"MoonWorld","StationId","MoonCleaners.MoonCollector");
    SW_BIND(wallet,"MoonWorld","WalletDollars","System.String");SW_BIND(charge,"MoonWorld","ChargeBuild","System.String;System.Decimal");
    SW_BIND(save_root,"MoonWorld","get_SaveDirectory","");SW_BIND(request_save,"MoonWorld","RequestSave","");
    SW_BIND(restoring,"MoonWorld","get_IsRestoringRendering","");SW_BIND(blocked,"MoonWalker","get_MenuBlocked","");
    SW_BIND(firstperson,"MoonWalker","get_IsFirstPerson","");SW_BIND(typing,"MoonInput","get_Typing","");
    SW_BIND(is_terrain,"MoonWorld","IsTerrainCollider","UnityEngine.Collider");
    SW_BIND(bedrock,"MoonWorld","BedrockPoint","UnityEngine.Vector3;UnityEngine.Vector3&");SW_BIND(dust_height,"MoonWorld","VisibleDustHeight","UnityEngine.Vector3");
#undef SW_BIND
    sw.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");sw.camera_field=get_field(find_class("MoonCleaners","MoonWalker"),"viewCamera");
    sw.component_transform=su_method("UnityEngine","Component","get_transform","");sw.instance_id=su_method("UnityEngine","Object","GetInstanceID","");
    sw.hit_transform=su_method("UnityEngine","RaycastHit","get_transform","");
    sw.decimal_int=su_method("System","Decimal",".ctor","System.Int32");sw.decimal_long=su_method("System","Decimal","ToInt64","System.Decimal");
    sw.find_objects=su_method("UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode");
    sw.hit_array=su_hold(su.new_array(find_class("UnityEngine","RaycastHit"),8));
    return sw.world_field&&sw.camera_field&&sw.component_transform&&sw.instance_id&&sw.hit_transform&&sw.decimal_int&&sw.decimal_long&&sw.find_objects&&sw.hit_array;
}
#endif
