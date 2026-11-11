/* Current-build metadata evidence remains in the harness's ignored run folder. */
static PTR inspection_file;
static const char *(*sp_method_name)(PTR);
static PTR (*sp_methods)(PTR,PTR *),(*sp_fields)(PTR,PTR *);
static DWORD (*sp_arity)(PTR),(*sp_flags)(PTR,DWORD *);
static PTR (*sp_param)(PTR,DWORD),(*sp_return)(PTR),(*sp_field_type)(PTR);
static const char *(*sp_field_name)(PTR);
static int (*sp_field_offset)(PTR);
static char *(*sp_type_name)(PTR);
static void (*sp_free)(PTR);
static char *sp_quote(char *p,const char *s){
    *p++='"';if(s)while(*s){if(*s=='"'||*s=='\\')*p++='\\';*p++=*s++;}*p++='"';*p=0;return p;
}
static void sp_line(const char *s){DWORD n=0;const char *p=s;while(*p)p++;WriteFile(inspection_file,s,(DWORD)(p-s),&n,0);}
static char *sp_type_text(char *p,PTR t){char *s=sp_type_name(t);p=sp_quote(p,s);if(s)sp_free(s);return p;}
static void sp_inspect_class(const char *ns,const char *name,PTR game){
    PTR k=find_class(ns,name);if(!k)return;PTR it=0,m;char b[3072],*p;
    while((m=sp_methods(k,&it))){
        p=cat(b,"{\"kind\":\"method\",\"class\":");p=sp_quote(p,name);p=cat(p,",\"namespace\":");p=sp_quote(p,ns);
        p=cat(p,",\"name\":");p=sp_quote(p,sp_method_name(m));p=cat(p,",\"token\":");p=num(p,method_token(m));
        p=cat(p,",\"rva\":");p=num(p,(U64)*(PTR *)m-(U64)game);p=cat(p,",\"flags\":");p=num(p,sp_flags(m,0));
        p=cat(p,",\"return\":");p=sp_type_text(p,sp_return(m));p=cat(p,",\"parameters\":[");
        DWORD n=sp_arity(m);if(n>20)n=20;for(DWORD i=0;i<n;i++){if(i)p=cat(p,",");p=sp_type_text(p,sp_param(m,i));}
        cat(p,"]}\n");sp_line(b);
    }
    it=0;while((m=sp_fields(k,&it))){
        p=cat(b,"{\"kind\":\"field\",\"class\":");p=sp_quote(p,name);p=cat(p,",\"name\":");p=sp_quote(p,sp_field_name(m));
        p=cat(p,",\"offset\":");p=num(p,(U64)sp_field_offset(m));p=cat(p,",\"type\":");p=sp_type_text(p,sp_field_type(m));cat(p,"}\n");sp_line(b);
    }
}
static int sp_inspect(PTR game,const char *folder){
#define SP_EXPORT(VAR,NAME,TYPE) VAR=(TYPE)GetProcAddress(game,"il2cpp_" NAME);if(!VAR)return 0
    SP_EXPORT(sp_methods,"class_get_methods",PTR(*)(PTR,PTR *));SP_EXPORT(sp_fields,"class_get_fields",PTR(*)(PTR,PTR *));
    SP_EXPORT(sp_method_name,"method_get_name",const char *(*)(PTR));SP_EXPORT(sp_arity,"method_get_param_count",DWORD(*)(PTR));
    SP_EXPORT(sp_param,"method_get_param",PTR(*)(PTR,DWORD));SP_EXPORT(sp_return,"method_get_return_type",PTR(*)(PTR));
    SP_EXPORT(sp_flags,"method_get_flags",DWORD(*)(PTR,DWORD *));SP_EXPORT(sp_type_name,"type_get_name",char *(*)(PTR));
    SP_EXPORT(sp_field_name,"field_get_name",const char *(*)(PTR));SP_EXPORT(sp_field_type,"field_get_type",PTR(*)(PTR));
    SP_EXPORT(sp_field_offset,"field_get_offset",int(*)(PTR));SP_EXPORT(sp_free,"free",void(*)(PTR));
#undef SP_EXPORT
    char path[1100];cat(cat(path,folder),"\\slime-bindings.jsonl");inspection_file=CreateFileA(path,0x40000000,1,0,1,0x80,0);
    if(inspection_file==(PTR)(U64)-1)return 0;
    const char *game_types[]={"MoonWorld","MoonWalker","MoonCrew","MoonCrewSession","MoonRemotePlayer","MoonStationSites","MoonStationView","MoonCollector","MoonCollectorInteriorGrade","MoonSteam","MoonSteamLobby","MoonInput","MoonProgressStore","MoonPrototypeStation","MoonStationTravel","MoonSpawnArea","MoonTerrain","MoonWalletSnapshot","MoonSettings","MoonGameSettings","MoonSession","MoonSaveSlots","MoonUpgradeShop","MoonPlayerRecordSnapshot","MoonBuiltDepot","MoonOwnedVehicle","MoonPrototypeVehicle"};
    const char *unity_types[]={"Object","GameObject","Component","Transform","Mesh","MeshFilter","MeshRenderer","Renderer","Material","Shader","Texture2D","Physics","RaycastHit","Collider","SphereCollider","BoxCollider","CapsuleCollider","Time","Application","Camera","TextMesh","Font","AudioSource","AudioClip","Quaternion","Mathf","SceneManager","Resources"};
    const char *steam_types[]={"SteamNetworking","SteamNetworkingMessages","SteamNetworkingIdentity","SteamNetworkingMessage_t","SteamUser","SteamFriends","SteamMatchmaking","CSteamID"};
    for(U64 i=0;i<sizeof(game_types)/sizeof(*game_types);i++)sp_inspect_class("MoonCleaners",game_types[i],game);
    sp_inspect_class("MoonCleaners","MoonPrototypeEquipment",game);sp_inspect_class("MoonCleaners","MoonDepotBuilder",game);
    for(U64 i=0;i<sizeof(unity_types)/sizeof(*unity_types);i++)sp_inspect_class("UnityEngine",unity_types[i],game);
    sp_inspect_class("UnityEngine.SceneManagement","SceneManager",game);
    for(U64 i=0;i<sizeof(steam_types)/sizeof(*steam_types);i++)sp_inspect_class("Steamworks",steam_types[i],game);
    sp_inspect_class("System","Decimal",game);
    FlushFileBuffers(inspection_file);return 1;
}
