/* Private setup uses vanilla placement/ownership APIs, never a replacement
 * vehicle controller or edits to the player's persistent pet ledger. */
#ifndef SLIME_TEST_SCENE_H
#define SLIME_TEST_SCENE_H
static unsigned sc_staged,sc_was_stage,sc_was_spawn,sc_was_build,sc_was_board,sc_build_open;
static PTR sc_component(PTR component,const char *name){int ok=1;void *a[]={su_type("MoonCleaners",name)};return call(su_method("UnityEngine","Component","GetComponent","System.Type"),component,a,&ok);}
static void sc_input(PTR walker,int allow){
    unsigned stage=(sp_key(0x73)&0x8000)!=0,spawn=(sp_key(0x7b)&0x8000)!=0,build=(sp_key(0x74)&0x8000)!=0,board=(sp_key(0x72)&0x8000)!=0;int ok=1;
    if(allow&&stage&&!sc_was_stage){
        if(sr_find_terrain()){
            SPVec3 up;sw_up(&sw,sr.terrain,&up);SPVec3 pos=su_add(sr.terrain,su_mul(up,.95f)),forward=sp_shops[0].forward;float pitch=22;unsigned char refresh=1;
            void *t[]={&pos,&refresh};call(su_method("MoonCleaners","MoonWalker","Teleport","UnityEngine.Vector3;System.Boolean"),walker,t,&ok);
            void *p[]={&pos,&forward,&pitch};call(su_method("MoonCleaners","MoonWalker","Place","UnityEngine.Vector3;UnityEngine.Vector3;System.Single"),walker,p,&ok);
            sc_staged=(unsigned)ok;sp_event(ok?"test-outdoor-staged":"test-outdoor-unavailable");
        }else sp_event("test-outdoor-unavailable");
    }
    if(allow&&spawn&&!sc_was_spawn&&sc_staged){
        PTR equipment=sc_component(walker,"MoonPrototypeEquipment");
        if(!equipment){int inactive=0,sort=0;void *a[]={su_type("MoonCleaners","MoonPrototypeEquipment"),&inactive,&sort};PTR array=call(sw.find_objects,0,a,&ok);U64 n=array?su.array_length(array):0;
            if(n<=64)for(U64 i=0;i<n;i++){PTR c=*(PTR *)su.array_address(array,8,i);if(call(su_method("MoonCleaners","MoonPrototypeEquipment","get_Player",""),c,0,&ok)==walker){equipment=c;break;}}
        }
        int index=0;void *a[]={&index};PTR result=equipment?call(su_method("MoonCleaners","MoonPrototypeEquipment","SpawnVehicle","System.Int32"),equipment,a,&ok):0;
        sp_event(ok&&result&&*(unsigned char *)unbox(result)?"test-vanilla-vehicle-created":"test-vanilla-vehicle-unavailable");
    }
    if(build&&!sc_was_build&&(allow||sc_build_open)){
        PTR builder=sc_component(walker,"MoonDepotBuilder");
        if(builder){
            if(sc_build_open){call(su_method("MoonCleaners","MoonDepotBuilder","CancelPlacement",""),builder,0,&ok);unsigned char no=0;void *a[]={&no};call(su_method("MoonCleaners","MoonDepotBuilder","SetOpen","System.Boolean"),builder,a,&ok);sc_build_open=0;}
            else{call(su_method("MoonCleaners","MoonDepotBuilder","EnsureBuildUI",""),builder,0,&ok);call(su_method("MoonCleaners","MoonDepotBuilder","ToggleBuildMode",""),builder,0,&ok);sc_build_open=1;}
        }
        sp_event(builder&&ok?(sc_build_open?"test-build-catalog-opened":"test-build-catalog-closed"):"test-build-catalog-unavailable");
    }
    if(board&&!sc_was_board&&!boolean(sw.blocked,walker,&ok)){
        int inactive=0,sort=0;void *a[]={su_type("MoonCleaners","MoonPrototypeVehicle"),&inactive,&sort};PTR array=call(sw.find_objects,0,a,&ok);U64 n=array?su.array_length(array):0;PTR nearest=0;float best=12;
        SPVec3 position=su_vector(su.tr_get_position,sw_transform(walker));
        if(n<=64)for(U64 i=0;i<n;i++){PTR vehicle=*(PTR *)su.array_address(array,8,i);if(call(su_method("MoonCleaners","MoonPrototypeVehicle","get_Owner",""),vehicle,0,&ok)!=walker)continue;
            float distance=sp_length(su_sub(position,su_vector(su.tr_get_position,sw_transform(vehicle))));if(distance<best){best=distance;nearest=vehicle;}}
        if(nearest){int driven=boolean(su_method("MoonCleaners","MoonPrototypeVehicle","get_IsDriven",""),nearest,&ok);
            PTR result=call(su_method("MoonCleaners","MoonPrototypeVehicle",driven?"Exit":"Enter",""),nearest,0,&ok);
            sp_event(ok&&result&&*(unsigned char *)unbox(result)?(driven?"test-vanilla-exited":"test-vanilla-boarded"):"test-vanilla-boarding-unavailable");
        }else sp_event("test-vanilla-boarding-unavailable");
    }
    sc_was_stage=stage;sc_was_spawn=spawn;sc_was_build=build;sc_was_board=board;
}
#endif
