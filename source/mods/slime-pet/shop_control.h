#ifndef MOON_SLIME_SHOP_CONTROL_H
#define MOON_SLIME_SHOP_CONTROL_H
#include "../../runtime/sdk/interactible_gizmos.h"

/* Engine-independent adaptation of CRS's compact momentary button. The owning
 * runtime supplies a rising-edge activation and decides authority, price and
 * purchase outcome. This policy never changes money or spawns an entity. */
#define MS_SHOP_ENGAGE_SECONDS MOON_GIZMO_ENGAGE_SECONDS
#define MS_SHOP_REBOUND_SECONDS MOON_GIZMO_REBOUND_SECONDS
#define MS_SHOP_PRESSED_GRACE_SECONDS MOON_GIZMO_TARGET_GRACE_SECONDS
#define MS_SHOP_BUTTON_TRAVEL -.035f
#define MS_SHOP_CLICK_MAX_VOICES 6u
#define MS_SHOP_CLICK_MAX_DISTANCE 8.0f
#define MS_SHOP_CLICK_VOLUME_DB -5.0f

typedef MoonGizmoMotion MSShopControl;

typedef struct {
    float offset_z;
    float proxy_centre[3], proxy_size[3];
    float outline_min[3], outline_max[3];
    unsigned pressed, retain_pressed_target;
} MSShopControlPose;

static inline void ms_shop_control_init(MSShopControl *control) {
    moon_gizmo_init(control);
}
static inline unsigned ms_shop_control_press(MSShopControl *control) {
    return moon_gizmo_press(control,moon_gizmo_default_timing());
}
static inline void ms_shop_control_step(MSShopControl *control, float dt) {
    moon_gizmo_step(control,moon_gizmo_default_timing(),dt);
}

/* Derive geometry, proxy and outline from the same single motion value. The
 * cap-only outline excludes the buried shaft even though both share one mesh.
 * Caller composes this shop-local pose with the station's root transform. */
static inline MSShopControlPose ms_shop_control_pose(const MSShopControl *control) {
    MSShopControlPose pose;
    pose.offset_z = MS_SHOP_BUTTON_TRAVEL*control->ratio;
    pose.proxy_centre[0] = .29f;
    pose.proxy_centre[1] = .81f;
    pose.proxy_centre[2] = .305f+pose.offset_z;
    pose.proxy_size[0] = pose.proxy_size[1] = .168f;
    pose.proxy_size[2] = .078f;
    pose.outline_min[0] = .21f;
    pose.outline_min[1] = .73f;
    pose.outline_min[2] = .270f+pose.offset_z;
    pose.outline_max[0] = .37f;
    pose.outline_max[1] = .89f;
    pose.outline_max[2] = .3403f+pose.offset_z;
    pose.pressed = control->active;
    pose.retain_pressed_target = control->grace_remaining > 0.0f;
    return pose;
}

/* A six-voice round-robin is the shared CRS bound. The consumer may stop/reuse
 * this slot and position its AudioSource at the moving cap, with 8 m rolloff. */
static inline unsigned ms_shop_control_audio_voice(const MSShopControl *control) {
    return control->press_serial ? (control->press_serial-1u)%MS_SHOP_CLICK_MAX_VOICES : 0u;
}

#endif
