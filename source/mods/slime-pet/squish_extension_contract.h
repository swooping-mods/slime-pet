#ifndef MOONLAB_SLIME_SQUISH_EXTENSION_CONTRACT_H
#define MOONLAB_SLIME_SQUISH_EXTENSION_CONTRACT_H
/* Inert I27 preparation. No production source includes this header. This is a
 * pure admission/proposal contract, not I22, mesh deformation or flight code.
 * All context/witness fields are host-derived; none are client assertions. */
#include "pet_core.h"

#define SP_SQUISH_EXTENSION_CONTRACT_VERSION 1u
#define SP_SQUISH_PET_REACH 2.5f
#define SP_SQUISH_RELEASE_OFFSET_MAX .75f
#define SP_SQUISH_INHERITED_SPEED_MAX 4.f
#define SP_SQUISH_RELEASE_SPEED_MAX 8.f
#define SP_SQUISH_LAUNCH_SPEED_MIN 2.f
#define SP_SQUISH_LAUNCH_SPEED_MAX 6.f
#define SP_SQUISH_HELD_AMOUNT_MAX .75f
#define SP_SQUISH_PET_AMOUNT_MAX .35f

enum {
  SP_SQUISH_HELD = 1,
  SP_SQUISH_THROW = 2,
  SP_SQUISH_PET = 3
};
enum {
  SP_SQUISH_ACCEPT = 0,
  SP_SQUISH_BAD_INPUT,
  SP_SQUISH_FOUNDATION_PENDING,
  SP_SQUISH_NO_AUTHORITY,
  SP_SQUISH_STALE_IDENTITY,
  SP_SQUISH_NO_TARGET,
  SP_SQUISH_NOT_OWNER,
  SP_SQUISH_BUSY,
  SP_SQUISH_NO_GRAVITY,
  SP_SQUISH_TOO_FAR,
  SP_SQUISH_NO_CLEARANCE
};
enum {
  SP_SQUISH_EFFECT_NONE = 0,
  SP_SQUISH_EFFECT_HELD,
  SP_SQUISH_EFFECT_RELEASE_PROPOSAL,
  SP_SQUISH_EFFECT_PET
};

/* Authority epoch changes on any authority/host/session reset; pet incarnation
 * changes on replacement, despawn/rejoin and world reset, even for the same ID.
 * These are future adapter-owned counters, absent from today's SPPet/store. */
typedef struct {
  SPU32 version, action;
  SPU64 world_id, session_id, authority_epoch, pet_incarnation;
  SPId target_owner;
  float strength; /* Intent only, in [0,1]; no client direction or velocity. */
} SPSquishRequest;

typedef struct {
  SPU64 world_id, session_id, authority_epoch, pet_incarnation;
  SPU32 tick;
  SPId target_owner;
  SPVec3 target_position, up, gravity, release_position;
  /* Exact capsule occupancy/sweep and aim witnesses, all for this target/tick.
   * A failed query must produce zero, never an optimistic default. */
  SPU32 gravity_known, aim_clear, capsule_clear, release_sweep_clear;
} SPSquishWitness;

typedef struct {
  const SPCore *core;
  SPId authenticated_actor, target_owner;
  SPU64 authority_epoch, pet_incarnation, current_host_session;
  SPU32 authoritative_latched, host_now, suspended, actor_authenticated;
  SPU32 plushie_accepted, i22_proven;
  SPU32 hands_claimed, request_pending;
  SPVec3 actor_position, host_forward, host_owner_velocity;
  SPSquishWitness witness;
} SPSquishContext;

typedef struct {
  SPU32 code, effect, retain_carry;
  float amount; /* Abstract cue intensity; not I22 compression or impulse. */
  SPVec3 release_position, release_velocity;
  SPVec3 up; /* Measured gravity frame, never a held/pet reaction axis. */
} SPSquishDecision;

static inline int sp_squish_id_valid(SPId id) { return id.lo || id.hi; }
static inline int sp_squish_id_equal(SPId a, SPId b) {
  return a.lo == b.lo && a.hi == b.hi;
}
static inline int sp_squish_unit_vector(SPVec3 v) {
  float square = sp_dot(v, v);
  return sp_vec_valid(v) && square >= .98f && square <= 1.02f;
}

/* Never mutates core, claims, ledger or presentation. In particular, ACCEPT /
 * RELEASE_PROPOSAL retains carry until the future host flight commit and its
 * authenticated acknowledgment succeed. Every rejection keeps carry/hands. */
static inline SPSquishDecision sp_squish_extension_decide(
    const SPSquishContext *c, const SPSquishRequest *r) {
  SPSquishDecision d = {0};
  const SPPet *pet = 0;
  const SPSquishWitness *w;
  SPVec3 gravity_direction, forward, velocity;
  float speed;
  SPU32 i;
  d.code = SP_SQUISH_BAD_INPUT;
  d.retain_carry = 1;
  if (!c || !r || !c->core ||
      r->version != SP_SQUISH_EXTENSION_CONTRACT_VERSION ||
      r->action < SP_SQUISH_HELD || r->action > SP_SQUISH_PET ||
      !sp_finite(r->strength) || r->strength < 0 || r->strength > 1 ||
      !sp_squish_id_valid(r->target_owner))
    return d;
  if (c->plushie_accepted != 1 || c->i22_proven != 1) {
    d.code = SP_SQUISH_FOUNDATION_PENDING;
    return d;
  }
  if (c->authoritative_latched != 1 || c->host_now != 1 || c->suspended ||
      c->actor_authenticated != 1 || !sp_squish_id_valid(c->authenticated_actor) ||
      !c->current_host_session ||
      c->current_host_session != c->core->session_id) {
    d.code = SP_SQUISH_NO_AUTHORITY;
    return d;
  }
  w = &c->witness;
  if (!r->world_id || !r->session_id || !c->authority_epoch ||
      !c->pet_incarnation || r->world_id != c->core->world_id ||
      r->session_id != c->core->session_id ||
      r->authority_epoch != c->authority_epoch ||
      r->pet_incarnation != c->pet_incarnation ||
      !sp_squish_id_equal(r->target_owner, c->target_owner) ||
      w->world_id != r->world_id || w->session_id != r->session_id ||
      w->authority_epoch != r->authority_epoch ||
      w->pet_incarnation != r->pet_incarnation ||
      !sp_squish_id_equal(w->target_owner, r->target_owner) ||
      w->tick != c->core->tick) {
    d.code = SP_SQUISH_STALE_IDENTITY;
    return d;
  }
  for (i = 0; i < SP_PLAYERS; ++i)
    if (c->core->players[i].used &&
        sp_squish_id_equal(c->core->players[i].id, r->target_owner)) {
      pet = &c->core->players[i];
      break;
    }
  if (!pet || !pet->owned || !pet->active || !pet->owner_valid ||
      pet->owner.world_id != r->world_id || pet->mode > SP_HELD) {
    d.code = SP_SQUISH_NO_TARGET;
    return d;
  }
  if (!sp_vec_valid(pet->position) || !sp_vec_valid(w->target_position) ||
      sp_length(sp_sub(pet->position, w->target_position)) > .001f) {
    d.code = SP_SQUISH_STALE_IDENTITY;
    return d;
  }
  if (r->action != SP_SQUISH_PET &&
      !sp_squish_id_equal(c->authenticated_actor, pet->id)) {
    d.code = SP_SQUISH_NOT_OWNER;
    return d;
  }
  if (c->request_pending ||
      (r->action == SP_SQUISH_PET ? pet->mode == SP_HELD :
       (pet->mode != SP_HELD || c->hands_claimed != 1))) {
    d.code = SP_SQUISH_BUSY;
    return d;
  }
  if (w->gravity_known != 1 || !sp_squish_unit_vector(w->up) ||
      !sp_squish_unit_vector(pet->up) ||
      !sp_vec_valid(w->gravity) || !sp_unit(w->gravity, &gravity_direction) ||
      sp_dot(w->up, gravity_direction) > -.98f ||
      sp_dot(w->up, pet->up) < .98f) {
    d.code = SP_SQUISH_NO_GRAVITY;
    return d;
  }
  if (!sp_vec_valid(c->actor_position) ||
      sp_length(sp_sub(c->actor_position, pet->position)) > SP_SQUISH_PET_REACH) {
    d.code = SP_SQUISH_TOO_FAR;
    return d;
  }
  if (w->aim_clear != 1 || w->capsule_clear != 1) {
    d.code = SP_SQUISH_NO_CLEARANCE;
    return d;
  }
  if (r->action == SP_SQUISH_THROW) {
    if (!sp_vec_valid(w->release_position) || w->release_sweep_clear != 1 ||
        sp_length(sp_sub(w->release_position, pet->position)) >
            SP_SQUISH_RELEASE_OFFSET_MAX) {
      d.code = SP_SQUISH_NO_CLEARANCE;
      return d;
    }
    if (!sp_unit(c->host_forward, &forward) ||
        !sp_vec_valid(c->host_owner_velocity) ||
        sp_length(c->host_owner_velocity) > SP_SQUISH_INHERITED_SPEED_MAX)
      return d;
    speed = SP_SQUISH_LAUNCH_SPEED_MIN + r->strength *
        (SP_SQUISH_LAUNCH_SPEED_MAX - SP_SQUISH_LAUNCH_SPEED_MIN);
    velocity = sp_add(sp_mul(forward, speed), c->host_owner_velocity);
    speed = sp_length(velocity);
    if (!sp_vec_valid(velocity) || !sp_finite(speed))
      return d;
    if (speed > SP_SQUISH_RELEASE_SPEED_MAX)
      velocity = sp_mul(velocity, SP_SQUISH_RELEASE_SPEED_MAX / speed);
    d.release_position = w->release_position;
    d.release_velocity = velocity;
    d.effect = SP_SQUISH_EFFECT_RELEASE_PROPOSAL;
  } else if (r->action == SP_SQUISH_HELD) {
    d.amount = r->strength * SP_SQUISH_HELD_AMOUNT_MAX;
    d.effect = SP_SQUISH_EFFECT_HELD;
  } else {
    d.amount = r->strength * SP_SQUISH_PET_AMOUNT_MAX;
    d.effect = SP_SQUISH_EFFECT_PET;
    d.retain_carry = 0; /* Admission required an unheld target. */
  }
  d.up = w->up;
  d.code = SP_SQUISH_ACCEPT;
  return d;
}
#endif
