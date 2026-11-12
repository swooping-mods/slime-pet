#ifndef SLIME_VISUAL_H
#define SLIME_VISUAL_H
#include "assets/slime_asset.h"
#include "shop_assets/shop_meshes.h"
#include "shop_control.h"
#include "glyphs.h"
#include "animation_control.h"
#include "../../runtime/sdk/pet_motion.h"
typedef struct {SURoot go,transform,mesh,vertices,normals;} SVBody;
typedef struct {SVBody body;char previous[65];float size;} SVLabel;
static SGText sv_glyphs;
static SPVec3 sv_next_positions[SP_JELLY_VERTICES],sv_next_normals[SP_JELLY_VERTICES];
typedef struct {
    SURoot root,transform,cradle;SVLabel zzz;
    SVBody body;unsigned variant,last_frame,last_clip,animation_mode,animation_cycle;float clock,animation_time;int made;
    MoonPetMotion motion;MoonPetHop hop;
    unsigned motion_held,motion_kind,motion_reset_reason,motion_reset_pending;
} SVPet;
typedef struct {
    SURoot root,transform,pieces[16],button,button_transform,focus;SVLabel label,buy_label;SVBody bodies[16];
    SVPet display;MSShopControl control;SPVec3 position,forward,up;
    float status_time;unsigned made,empty;
} SVShop;
static SURoot sv_pet_materials[5],sv_shop_materials[256],sv_dark,sv_white,sv_focus,sv_eye,sv_eye_texture;
static int sv_indices[49152];
static inline void sv_property(PTR mat,const char *name,float value){int ok=1;void *a[]={su.string(name),&value};call(su.material_float,mat,a,&ok);}
static inline void sv_keyword(PTR mat,const char *name){int ok=1;void *a[]={su.string(name)};call(su.material_keyword,mat,a,&ok);}
static inline void sv_integer(PTR mat,const char *name,int value){sv_property(mat,name,(float)value);}
static inline PTR sv_material(const char *name,SUColor color,float metallic,float roughness,unsigned flags){
    int ok=1;void *find[]={su.string("Universal Render Pipeline/Lit")};PTR shader=call(su.shader_find,0,find,&ok);
    if(!shader)return 0;PTR mat=su_new("UnityEngine","Material");void *a[]={shader};call(su.material_ctor,mat,a,&ok);if(!ok)return 0;
    PTR has=su_method("UnityEngine","Material","HasProperty","System.String");
    const char *properties[]={"_BaseColor","_Metallic","_Smoothness","_Surface","_SrcBlend","_DstBlend","_ZWrite","_Cull"};
    for(unsigned i=0;i<8;i++){void *query[]={su.string(properties[i])};PTR value=call(has,mat,query,&ok);if(!ok||!value||!*(unsigned char *)unbox(value)){su_destroy(mat);return 0;}}
    void *n[]={su.string(name)};call(su.obj_name,mat,n,&ok);void *c[]={&color};call(su.material_color,mat,c,&ok);
    PTR set_color=su_method("UnityEngine","Material","SetColor","System.String;UnityEngine.Color");
    void *base[]={su.string("_BaseColor"),&color};call(set_color,mat,base,&ok);
    sv_property(mat,"_Metallic",metallic);sv_property(mat,"_Smoothness",1-roughness);sv_property(mat,"_Surface",0);
    sv_integer(mat,"_ZWrite",1);sv_integer(mat,"_SrcBlend",1);sv_integer(mat,"_DstBlend",0);
    if(flags&1){
        sv_property(mat,"_Surface",1);sv_property(mat,"_Blend",0);sv_property(mat,"_BlendModePreserveSpecular",0);
        sv_integer(mat,"_SrcBlend",5);sv_integer(mat,"_DstBlend",10);sv_integer(mat,"_ZWrite",0);
        sv_integer(mat,"_SrcBlendAlpha",1);sv_integer(mat,"_DstBlendAlpha",10);
        sv_keyword(mat,"_SURFACE_TYPE_TRANSPARENT");
        su_int(su_method("UnityEngine","Material","set_renderQueue","System.Int32"),mat,3000);
        void *tag[]={su.string("RenderType"),su.string("Transparent")};call(su_method("UnityEngine","Material","SetOverrideTag","System.String;System.String"),mat,tag,&ok);
    }
    if(flags&2)sv_integer(mat,"_Cull",0);
    if(flags&4){SUColor emission={color.r*.65f,color.g*.65f,color.b*.65f,1};sv_keyword(mat,"_EMISSION");void *e[]={su.string("_EmissionColor"),&emission};call(set_color,mat,e,&ok);}
    return mat;
}
static inline int sv_materials(void){
    for(unsigned i=0;i<5;i++){
        const SPMaterial *p=sp_jelly_palette+i;SUColor c={p->color[0],p->color[1],p->color[2],p->color[3]};
        PTR m=sv_material(p->name,c,p->metallic,p->roughness,c.a<1?1u:0u);if(!m)return 0;sv_pet_materials[i]=su_hold(m);
    }
    int ok=1,size=2,format=4;unsigned char mip=1,linear=0;
    PTR texture=su_new("UnityEngine","Texture2D");sv_eye_texture=su_hold(texture);if(!texture||!sv_eye_texture)return 0;
    void *ctor[]={&size,&size,&format,&mip,&linear};
    call(su_method("UnityEngine","Texture2D",".ctor","System.Int32;System.Int32;UnityEngine.TextureFormat;System.Boolean;System.Boolean"),texture,ctor,&ok);
    PTR bytes=su_array(su.byte_class,sizeof(sp_jelly_eye_png),sp_jelly_eye_png,1);SURoot hold=su_hold(bytes);
    if(!hold)return 0;void *load[]={texture,bytes};
    PTR loaded=call(su_method("UnityEngine","ImageConversion","LoadImage","UnityEngine.Texture2D;System.Byte[]"),0,load,&ok);su_drop(&hold);
    if(!ok||!loaded||!*(unsigned char *)unbox(loaded))return 0;
    PTR eye=sv_material("Fitted jelly eyes / painted highlights",(SUColor){1,1,1,1},0,.29f,0);if(!eye)return 0;sv_eye=su_hold(eye);
    void *map[]={su.string("_BaseMap"),texture};call(su_method("UnityEngine","Material","SetTexture","System.String;UnityEngine.Texture"),eye,map,&ok);if(!ok)return 0;
    sv_dark=su_hold(sv_material("Slime expression ink",(SUColor){.012f,.022f,.045f,1},0,.5f,0));
    sv_white=su_hold(sv_material("Slime eye glint",(SUColor){1,1,1,1},0,.3f,2));
    sv_focus=su_hold(sv_material("Slime physical control focus",(SUColor){.04f,.7f,1,1},0,.3f,6));
    for(unsigned i=0;i<MS_SHOP_MATERIAL_COUNT;i++){
        const MSShopMaterial *s=&ms_shop_materials[i];SUColor c={s->color[0],s->color[1],s->color[2],s->color[3]};
        sv_shop_materials[i]=su_hold(sv_material(s->name,c,s->metallic,s->roughness,s->flags));if(!sv_shop_materials[i])return 0;
    }return sv_dark&&sv_white&&sv_focus&&sv_eye;
}
static inline void sv_materials_drop(void){
    su_destroy(su_get(sv_eye));su_drop(&sv_eye);su_destroy(su_get(sv_eye_texture));su_drop(&sv_eye_texture);
    for(unsigned i=0;i<5;i++){su_destroy(su_get(sv_pet_materials[i]));su_drop(&sv_pet_materials[i]);}
    for(unsigned i=0;i<MS_SHOP_MATERIAL_COUNT;i++){su_destroy(su_get(sv_shop_materials[i]));su_drop(&sv_shop_materials[i]);}
    su_destroy(su_get(sv_dark));su_destroy(su_get(sv_white));su_destroy(su_get(sv_focus));su_drop(&sv_dark);su_drop(&sv_white);su_drop(&sv_focus);
}
static inline void sv_assign_materials(PTR renderer,const SURoot *roots,unsigned count){
    PTR a=su.new_array(su.material_class,count);PTR *dst=su.array_address(a,sizeof(PTR),0);for(unsigned i=0;i<count;i++)dst[i]=su_get(roots[i]);
    int ok=1;void *args[]={a};call(su.renderer_materials,renderer,args,&ok);
    PTR surface=su_method("UnityEngine","Material","GetFloat","System.String");int transparent=0;
    for(unsigned i=0;i<count;i++){void *query[]={su.string("_Surface")};PTR value=call(surface,su_get(roots[i]),query,&ok);if(ok&&value&&*(float *)unbox(value)>.5f)transparent=1;}
    /* SetShaderPassEnabled is stripped in this build. Queue 3000 excludes the
     * opaque depth prepass; renderer shadow policy disables glass silhouettes. */
    if(transparent)su_int(su_method("UnityEngine","Renderer","set_shadowCastingMode","UnityEngine.Rendering.ShadowCastingMode"),renderer,0);
}
static inline int sv_mesh(SVBody *out,const char *name,PTR parent,unsigned count,const void *positions,const void *normals,const void *uv,unsigned submeshes,const SURoot *materials){
    su_zero(out,sizeof(*out));PTR go=su_gameobject(name,parent);if(!go)return 0;
    out->go=su_hold(go);out->transform=su_hold(su_transform(go));
    PTR mesh=su_new("UnityEngine","Mesh");int ok=1;call(su.mesh_ctor,mesh,0,&ok);if(!ok)return 0;out->mesh=su_hold(mesh);
    PTR vertices=su_array(su.vector_class,count,positions,sizeof(SPVec3));PTR ns=su_array(su.vector_class,count,normals,sizeof(SPVec3));
    out->vertices=su_hold(vertices);out->normals=su_hold(ns);void *va[]={vertices},*na[]={ns};call(su.mesh_vertices,mesh,va,&ok);call(su.mesh_normals,mesh,na,&ok);
    if(uv){PTR array=su_array(su.uv_class,count,uv,sizeof(SPVec2));void *ua[]={array};call(su.mesh_uv,mesh,ua,&ok);}
    su_int(su.mesh_subcount,mesh,(int)submeshes);PTR filter=su_add_component(go,"MeshFilter"),renderer=su_add_component(go,"MeshRenderer");
    void *ma[]={mesh};call(su.meshfilter_set,filter,ma,&ok);sv_assign_materials(renderer,materials,submeshes);return ok;
}
static inline int sv_triangles(PTR mesh,unsigned slot,const unsigned short *short_indices,const unsigned *long_indices,unsigned offset,unsigned count,int reverse){
    if(count>49152)return 0;for(unsigned i=0;i<count;i++)sv_indices[i]=(int)(short_indices?short_indices[offset+i]:long_indices[offset+i]);
    if(reverse)for(unsigned i=0;i+2<count;i+=3){int n=sv_indices[i+1];sv_indices[i+1]=sv_indices[i+2];sv_indices[i+2]=n;}
    PTR array=su_array(su.int_class,count,sv_indices,sizeof(int));int ok=1,index=(int)slot;unsigned char bounds=1;void *a[]={array,&index,&bounds};call(su.mesh_indices,mesh,a,&ok);return ok;
}
static inline PTR sv_primitive(const char *name,PTR parent,int shape,SPVec3 position,SPVec3 scale,SURoot material){
    int ok=1;void *a[]={&shape};PTR go=call(su.primitive,0,a,&ok);if(!ok||!go)return 0;
    void *n[]={su.string(name)};call(su.obj_name,go,n,&ok);PTR tr=su_transform(go);su_parent(tr,parent);
    su_set_vector(su.tr_local_position,tr,position);su_set_vector(su.tr_scale,tr,scale);
    PTR collider=su_component(go,"Collider");if(collider)su_destroy(collider);
    sv_assign_materials(su_component(go,"Renderer"),&material,1);return go;
}
static inline void sv_label_text(SVLabel *label,const char *value){
    if(su_eq(label->previous,value))return;
    if(!sg_build(value,label->size,&sv_glyphs))return;
    for(unsigned i=0;i<sv_glyphs.vertex_count;i++)sv_glyphs.positions[i].x=sv_glyphs.width*.5f-sv_glyphs.positions[i].x;
    PTR vs=su_array(su.vector_class,sv_glyphs.vertex_count,sv_glyphs.positions,sizeof(SGVec3));
    PTR ns=su_array(su.vector_class,sv_glyphs.vertex_count,sv_glyphs.normals,sizeof(SGVec3));
    su_drop(&label->body.vertices);su_drop(&label->body.normals);label->body.vertices=su_hold(vs);label->body.normals=su_hold(ns);
    int ok=1;void *va[]={vs},*na[]={ns};PTR mesh=su_get(label->body.mesh);
    /* Clear old references before shrinking (including hiding ZZZ entirely). */
    sv_triangles(mesh,0,0,(const unsigned *)sv_glyphs.indices,0,0,0);
    call(su.mesh_vertices,mesh,va,&ok);call(su.mesh_normals,mesh,na,&ok);
    sv_triangles(mesh,0,0,(const unsigned *)sv_glyphs.indices,0,sv_glyphs.index_count,1);call(su.mesh_bounds,mesh,0,&ok);
    cat(label->previous,value);
}
static inline int sv_label_make(SVLabel *label,const char *name,PTR parent,SPVec3 position,float size,const char *value){
    su_zero(label,sizeof(*label));label->size=size;
    if(!sv_mesh(&label->body,name,parent,0,0,0,0,1,&sv_white))return 0;
    su_set_vector(su.tr_local_position,su_get(label->body.transform),position);sv_label_text(label,value);return 1;
}
static inline int sv_pet_make(SVPet *p,PTR parent,unsigned variant,float scale){
    su_zero(p,sizeof(*p));PTR root=su_gameobject("MoonLab Slime",parent);if(!root)return 0;
    p->root=su_hold(root);p->transform=su_hold(su_transform(root));p->variant=variant;
    su_set_vector(su.tr_scale,su_get(p->transform),su_v(scale,scale,scale));
    SURoot mats[8];if(sp_slime_asset.submesh_count>8)return 0;for(unsigned i=0;i<sp_slime_asset.submesh_count;i++)mats[i]=sp_slime_asset.submeshes[i].material_index?sv_eye:sv_pet_materials[variant];
    if(!sv_mesh(&p->body,"Corrected jelly body and fitted eyes",su_get(p->transform),sp_slime_asset.vertex_count,sp_slime_asset.positions,sp_slime_asset.normals,sp_slime_asset.uvs,sp_slime_asset.submesh_count,mats))return 0;
    for(unsigned i=0;i<sp_slime_asset.submesh_count;i++){const SPSubmesh *s=&sp_slime_asset.submeshes[i];if(!sv_triangles(su_get(p->body.mesh),i,sp_slime_asset.indices,0,s->index_offset,s->index_count,0))return 0;}
    if(!sv_label_make(&p->zzz,"Sleep ZZZ",su_get(p->transform),su_v(.05f,.58f,0),.065f,"Z Z Z"))return 0;
    p->last_frame=~0u;p->last_clip=~0u;p->animation_mode=~0u;p->made=1;return 1;
}
static inline void sv_pet_pose(SVPet *p,unsigned mode,unsigned blink,unsigned emotion,float dt){
    (void)blink;(void)emotion; /* Expressions belong to the imported eye morphs. */
    if(!p->made)return;p->clock+=dt;if(p->clock>=1)p->clock-=1;
    const SPClip *jump=&sp_slime_asset.clips[spa_clip(&sp_slime_asset,SP_FOLLOW)];
    float duration=(float)jump->frame_count/jump->sample_rate;
    if(!moon_pet_hop_step(&p->hop,mode==SP_FOLLOW,mode==SP_HELD,dt,duration,1.35f))return;
    unsigned wanted=p->hop.active?SP_FOLLOW:mode;
    if(p->hop.active){p->animation_mode=SP_FOLLOW;p->animation_time=p->hop.time;p->animation_cycle=p->hop.cycle;}
    else if(p->animation_mode!=wanted){p->animation_mode=wanted;p->animation_time=0;}
    else p->animation_time+=dt;
    /* Navigation owns travel along the floor; FollowHop supplies authored airtime.
     * Correct penetration only, preserving positive foot height during a hop. */
    unsigned clip=spa_clip(&sp_slime_asset,p->animation_mode);if(clip>=sp_slime_asset.clip_count)return;
    const SPClip *c=&sp_slime_asset.clips[clip];
    if(!p->hop.active){float length=(float)c->frame_count/c->sample_rate;
        if(p->animation_time>=length)p->animation_time-=length;p->animation_cycle=0;}
    float sample=p->animation_time*c->sample_rate;unsigned frame=(unsigned)sample%c->frame_count;float alpha=sample-(float)(unsigned)sample;
    if(sp_slime_asset.vertex_count<=SP_JELLY_VERTICES){
        PTR vertices=su_get(p->body.vertices),normals=su_get(p->body.normals);SPVec3 *vs=su.array_address(vertices,sizeof(SPVec3),0),*ns=su.array_address(normals,sizeof(SPVec3),0);
        sp_decode_frame(&sp_slime_asset,clip,frame,vs,ns);
        unsigned next=(frame+1)%c->frame_count;
        sp_decode_frame(&sp_slime_asset,clip,next,sv_next_positions,sv_next_normals);
        for(unsigned i=0;i<sp_slime_asset.vertex_count;i++){
            vs[i]=su_add(vs[i],su_mul(su_sub(sv_next_positions[i],vs[i]),alpha));
            ns[i]=su_normal(su_add(ns[i],su_mul(su_sub(sv_next_normals[i],ns[i]),alpha)));
        }
        float bottom=1e10f;for(unsigned i=0;i<sp_slime_asset.vertex_count;i++)if(vs[i].y<bottom)bottom=vs[i].y;
        float correction=bottom<0?-bottom:0;for(unsigned i=0;i<sp_slime_asset.vertex_count;i++)vs[i].y+=correction;
        int ok=1;void *va[]={vertices},*na[]={normals};call(su.mesh_vertices,su_get(p->body.mesh),va,&ok);call(su.mesh_normals,su_get(p->body.mesh),na,&ok);call(su.mesh_bounds,su_get(p->body.mesh),0,&ok);
        p->last_frame=frame;p->last_clip=clip;
    }
    sv_label_text(&p->zzz,mode==SP_SLEEP?"Z Z Z":"");
    float cycle=p->clock-(float)(int)p->clock;su_set_vector(su.tr_local_position,su_get(p->zzz.body.transform),su_v(.05f+cycle*.03f,.5f+cycle*.11f,.05f));
}
static inline void sv_cradle(SVPet *p,int enabled){
    if(enabled&&!p->cradle){
        PTR go=su_gameobject("Astronaut cupped mitts",su_get(p->transform));p->cradle=su_hold(go);PTR tr=su_transform(go);
        for(int i=-1;i<=1;i+=2){
            sv_primitive("Soft glove palm",tr,0,su_v((float)i*.235f,.035f,.06f),su_v(.23f,.14f,.32f),sv_white);
            sv_primitive("Curled glove thumb",tr,0,su_v((float)i*.19f,.105f,.075f),su_v(.105f,.13f,.16f),sv_white);
            sv_primitive("Dark wrist cuff",tr,0,su_v((float)i*.31f,-.025f,.18f),su_v(.18f,.14f,.16f),sv_dark);
        }
    }
    if(p->cradle)su_bool(method("UnityEngine","GameObject","SetActive",1),su_get(p->cradle),enabled);
}
static inline void sv_body_drop(SVBody *b){su_destroy(su_get(b->mesh));su_drop(&b->mesh);su_drop(&b->vertices);su_drop(&b->normals);su_drop(&b->transform);su_drop(&b->go);}
static inline void sv_pet_drop(SVPet *p){
    su_destroy(su_get(p->root));sv_body_drop(&p->body);
    sv_body_drop(&p->zzz.body);su_drop(&p->cradle);su_drop(&p->transform);su_drop(&p->root);p->made=0;
}
static inline int sv_shop_make(SVShop *s,SPVec3 position,SPVec3 forward,SPVec3 up,unsigned variant,const char *label){
    su_zero(s,sizeof(*s));s->position=position;s->forward=forward;s->up=up;
    PTR root=su_gameobject("MoonLab Slime Adoption Terminal",0);if(!root)return 0;s->root=su_hold(root);s->transform=su_hold(su_transform(root));
    su_set_vector(su.tr_position,su_get(s->transform),position);su_set_quat(su.tr_rotation,su_get(s->transform),su_rotation(forward,up));
    if(MS_SHOP_PIECE_COUNT>16||MS_SHOP_MATERIAL_COUNT>256)return 0;
    for(unsigned i=0;i<MS_SHOP_PIECE_COUNT;i++){
        const MSShopPiece *piece=&ms_shop_pieces[i];const MSShopMesh *m=&ms_shop_meshes[piece->mesh_index];SURoot mats[64];if(m->submesh_count>64)return 0;
        for(unsigned j=0;j<m->submesh_count;j++)mats[j]=sv_shop_materials[m->submeshes[j].material_index];
        if(!sv_mesh(&s->bodies[i],piece->name,su_get(s->transform),m->vertex_count,m->positions,m->normals,m->uv,m->submesh_count,mats))return 0;
        PTR tr=su_get(s->bodies[i].transform);s->pieces[i]=su_hold(tr);
        su_set_vector(su.tr_local_position,tr,su_v(piece->position[0],piece->position[1],piece->position[2]));
        su_set_vector(su.tr_scale,tr,su_v(piece->scale,piece->scale,piece->scale));
        su_set_quat(su.tr_local_rotation,tr,(SUQuat){piece->rotation_xyzw[0],piece->rotation_xyzw[1],piece->rotation_xyzw[2],piece->rotation_xyzw[3]});
        for(unsigned j=0;j<m->submesh_count;j++){
            const MSShopSubmesh *sub=&m->submeshes[j];unsigned count=piece->motion&&sub->material_index==3?0:sub->index_count;
            if(!sv_triangles(su_get(s->bodies[i].mesh),j,0,m->indices,sub->index_offset,count,0))return 0;
        }
        if(piece->motion){
            s->button=su_hold(su_get(s->bodies[i].go));s->button_transform=su_hold(tr);
            PTR box=su_add_component(su_get(s->button),"BoxCollider");su_set_vector(method("UnityEngine","BoxCollider","set_size",1),box,su_v(.168f,.168f,.078f));
        }
    }
    /* Physical shell colliders belong to the terminal; presentation has no
     * pet collider, so the sleeping specimen cannot consume shop inventory. */
    PTR shell=su_add_component(root,"BoxCollider");su_set_vector(method("UnityEngine","BoxCollider","set_center",1),shell,su_v(-.04f,.48f,0));su_set_vector(method("UnityEngine","BoxCollider","set_size",1),shell,su_v(.72f,.96f,.48f));
    if(!sv_pet_make(&s->display,su_get(s->transform),variant,.29f))return 0;
    su_set_vector(su.tr_local_position,su_get(s->display.transform),su_v(.29f,1.055f,0));
    if(!sv_label_make(&s->label,"Slime price display",su_get(s->transform),su_v(-.13f,1.1002f,.034f),.035f,label))return 0;
    su_set_quat(su.tr_local_rotation,su_get(s->label.body.transform),(SUQuat){-.09853762f,0,0,.99513333f});
    if(!sv_label_make(&s->buy_label,"BUY lettering",su_get(s->button_transform),su_v(0,-.014f,.041f),.035f,"BUY"))return 0;
    PTR focus=su_gameobject("BUY focus and pressed grace",su_get(s->button_transform));s->focus=su_hold(focus);PTR focus_tr=su_transform(focus);
    for(unsigned i=0;i<4;i++){
        float x=i<2?(i==0?-.09f:.09f):0,y=i>=2?(i==2?-.09f:.09f):0;
        sv_primitive("Moving cap outline",focus_tr,3,su_v(x,y,.045f),i<2?su_v(.006f,.18f,.006f):su_v(.18f,.006f,.006f),sv_focus);
    }
    su_bool(method("UnityEngine","GameObject","SetActive",1),focus,0);
    ms_shop_control_init(&s->control);s->made=1;return 1;
}
static inline void sv_shop_tick(SVShop *s,float dt){
    if(!s->made)return;ms_shop_control_step(&s->control,dt);MSShopControlPose pose=ms_shop_control_pose(&s->control);
    su_set_vector(su.tr_local_position,su_get(s->button_transform),su_v(.29f,.81f,.305f+pose.offset_z));
    if(!s->empty)sv_pet_pose(&s->display,SP_SLEEP,1,SP_SLEEPY,dt);
    if(s->status_time>0)s->status_time-=dt;
}
static inline void sv_shop_drop(SVShop *s){
    if(!s->made)return;sv_pet_drop(&s->display);sv_body_drop(&s->label.body);sv_body_drop(&s->buy_label.body);
    su_destroy(su_get(s->root));for(unsigned i=0;i<MS_SHOP_PIECE_COUNT;i++){sv_body_drop(&s->bodies[i]);su_drop(&s->pieces[i]);}
    su_drop(&s->button);su_drop(&s->button_transform);su_drop(&s->focus);su_drop(&s->transform);su_drop(&s->root);s->made=0;
}
#endif
