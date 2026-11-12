#ifndef MOON_INTERACTIBLE_GIZMOS_H
#define MOON_INTERACTIBLE_GIZMOS_H

/* Source-only presentation for momentary controls. Feed only an interaction
 * edge accepted by the owning input framework. Authority, purchases, sounds,
 * Unity objects and target identities remain with the consumer. */
typedef struct {
    float engage_seconds, rebound_seconds, target_grace_seconds;
} MoonGizmoTiming;
typedef struct {
    float elapsed, start_ratio, ratio, grace_remaining;
    unsigned press_serial, active;
} MoonGizmoMotion;
typedef struct { float x,y,z; } MoonGizmoPoint;

#define MOON_GIZMO_ENGAGE_SECONDS .065f
#define MOON_GIZMO_REBOUND_SECONDS .110f
#define MOON_GIZMO_TARGET_GRACE_SECONDS .280f

static inline MoonGizmoTiming moon_gizmo_default_timing(void) {
    return (MoonGizmoTiming){MOON_GIZMO_ENGAGE_SECONDS,
        MOON_GIZMO_REBOUND_SECONDS,MOON_GIZMO_TARGET_GRACE_SECONDS};
}
static inline int moon_gizmo_finite(float value) {
    return value==value && value<=3.402823466e38f && value>=-3.402823466e38f;
}
static inline int moon_gizmo_timing_valid(MoonGizmoTiming timing) {
    return moon_gizmo_finite(timing.engage_seconds) && timing.engage_seconds>0
        && moon_gizmo_finite(timing.rebound_seconds) && timing.rebound_seconds>0
        && moon_gizmo_finite(timing.target_grace_seconds) && timing.target_grace_seconds>=0
        && moon_gizmo_finite(timing.engage_seconds+timing.rebound_seconds);
}
static inline void moon_gizmo_init(MoonGizmoMotion *motion) {
    motion->elapsed=motion->start_ratio=motion->ratio=motion->grace_remaining=0;
    motion->press_serial=motion->active=0;
}
/* Repress from the current pose without snapping. A rejected transaction still
 * moves its physical control, so trigger before dispatching the transaction. */
static inline unsigned moon_gizmo_press(MoonGizmoMotion *motion,MoonGizmoTiming timing) {
    if(!moon_gizmo_timing_valid(timing))return 0;
    motion->start_ratio=motion->ratio;motion->elapsed=0;
    motion->grace_remaining=timing.target_grace_seconds;motion->active=1;
    if(!++motion->press_serial)++motion->press_serial;
    return motion->press_serial;
}
static inline void moon_gizmo_step(MoonGizmoMotion *motion,MoonGizmoTiming timing,float dt) {
    if(!moon_gizmo_finite(dt)||dt<=0||!moon_gizmo_timing_valid(timing))return;
    motion->grace_remaining=dt>=motion->grace_remaining?0:motion->grace_remaining-dt;
    if(!motion->active)return;
    /* Compare before adding: even a finite stall delta must not overflow. */
    if(dt>=timing.engage_seconds+timing.rebound_seconds-motion->elapsed){
        motion->elapsed=timing.engage_seconds+timing.rebound_seconds;
        motion->ratio=0;motion->active=0;return;
    }
    motion->elapsed+=dt;
    if(motion->elapsed<timing.engage_seconds){
        float t=motion->elapsed/timing.engage_seconds;
        t=1-(1-t)*(1-t);
        motion->ratio=motion->start_ratio+(1-motion->start_ratio)*t;
    }else{
        /* Small outward mechanical rebound after reaching full travel. */
        float back=(motion->elapsed-timing.engage_seconds)/timing.rebound_seconds-1;
        motion->ratio=-back*back*(2.70158f*back+1.70158f);
    }
}
/* Use this single pose for the visible cap, its attached label, collider and
 * focus outline. Consumers may also apply ratio to rotation or scale. */
static inline MoonGizmoPoint moon_gizmo_position(const MoonGizmoMotion *motion,
                                                MoonGizmoPoint rest,MoonGizmoPoint travel) {
    return (MoonGizmoPoint){rest.x+travel.x*motion->ratio,
        rest.y+travel.y*motion->ratio,rest.z+travel.z*motion->ratio};
}

#endif
