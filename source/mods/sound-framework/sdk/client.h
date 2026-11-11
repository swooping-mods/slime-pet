#ifndef MOON_SOUND_CLIENT_H
#define MOON_SOUND_CLIENT_H
#include "sound.h"
#include "assets.h"
#include "../../../runtime/native/services.h"
__declspec(dllimport) DWORD __stdcall GetModuleFileNameA(PTR,char *,DWORD);
/* Same dependency startup path as the vehicle/build frameworks, supporting
 * both standalone modkit deployment and desktop-loader dependency ordering. */
static const MoonSounds *ma_connect(PTR domain,PTR game){
    /* A recognized loader owns dependency startup and provider identity. A
     * missing/incompatible service fails closed instead of loading a second DLL. */
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        const MoonSounds *api=runtime?(const MoonSounds *)runtime->find("sound-framework","sound",MA_ABI,sizeof(MoonSounds)):0;
        return api&&api->abi==MA_ABI&&api->size>=sizeof(MoonSounds)?api:0;
    }

    char path[1200];DWORD n=GetModuleFileNameA(0,path,1000);
    if(!n||n>=1000)return 0;
    while(n&&path[n-1]!='\\'&&path[n-1]!='/')n--;
    if(!n)return 0;cat(path+n,"MoonLab\\mods\\sound-framework\\mod.dll");
    PTR module=LoadLibraryA(path);if(!module)return 0;
    int (*start)(PTR,PTR)=(int(*)(PTR,PTR))GetProcAddress(module,"MoonLab_ModStart");
    MoonSoundsGet get=(MoonSoundsGet)GetProcAddress(module,"MoonLab_Sounds_v1");
    return start&&get&&start(domain,game)?get(MA_ABI,sizeof(MoonSounds)):0;
}
static inline const MoonSoundAssets *ma_connect_assets(PTR domain,PTR game){
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        const MoonSoundAssets *api=runtime?(const MoonSoundAssets *)runtime->find("sound-framework","sound-assets",MA_ASSET_ABI,sizeof(MoonSoundAssets)):0;
        return api&&api->abi==MA_ASSET_ABI&&api->size>=sizeof(MoonSoundAssets)?api:0;
    }

    if(!ma_connect(domain,game))return 0;
    char path[1200];DWORD n=GetModuleFileNameA(0,path,1000);if(!n||n>=1000)return 0;
    while(n&&path[n-1]!='\\'&&path[n-1]!='/')n--;
    if(!n)return 0;cat(path+n,"MoonLab\\mods\\sound-framework\\mod.dll");
    PTR module=LoadLibraryA(path);if(!module)return 0;
    MoonSoundAssetsGet get=(MoonSoundAssetsGet)GetProcAddress(module,"MoonLab_SoundAssets_v1");
    return get?get(MA_ASSET_ABI,sizeof(MoonSoundAssets)):0;
}
#endif
