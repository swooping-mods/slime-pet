#ifndef MOON_PICKUP_HANDS_CLIENT_H
#define MOON_PICKUP_HANDS_CLIENT_H
#include "hands.h"
#include "../../../runtime/native/services.h"
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(PTR,char *,DWORD);
static const MoonHands *mh_connect(PTR domain,PTR game){
    /* A recognized loader owns dependency startup and provider identity. A
     * missing/incompatible service fails closed instead of loading a second DLL. */
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        const MoonHands *api=runtime?(const MoonHands *)runtime->find("pickup-hands","hands",MH_ABI,sizeof(MoonHands)):0;
        return api&&api->abi==MH_ABI&&api->size>=sizeof(MoonHands)?api:0;
    }

    char path[1200];DWORD n=GetModuleFileNameA(0,path,1000);if(!n||n>=1000)return 0;
    while(n&&path[n-1]!='\\'&&path[n-1]!='/')n--;
    if(!n)return 0;cat(path+n,"MoonLab\\mods\\pickup-hands\\mod.dll");
    PTR module=LoadLibraryA(path);if(!module)return 0;
    int (*start)(PTR,PTR)=(int(*)(PTR,PTR))GetProcAddress(module,"MoonLab_ModStart");
    MoonHandsGet get=(MoonHandsGet)GetProcAddress(module,"MoonLab_Hands_v1");
    return start&&get&&start(domain,game)?get(MH_ABI,sizeof(MoonHands)):0;
}
#endif
