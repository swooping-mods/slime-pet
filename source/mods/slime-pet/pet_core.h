#ifndef MOONLAB_SLIME_PET_CORE_H
#define MOONLAB_SLIME_PET_CORE_H
/* Original engine-independent host policy. Define SP_IMPLEMENTATION in one TU.
 * All calls are externally serialized. No allocator, CRT, engine or transport.
 * World callbacks are authoritative, synchronous, bounded, and may not reenter.
 */
typedef unsigned int SPU32;
typedef unsigned long long SPU64;
typedef char SP_require_u32[(sizeof(SPU32) == 4) ? 1 : -1];
typedef char SP_require_u64[(sizeof(SPU64) == 8) ? 1 : -1];
typedef char SP_require_f32[(sizeof(float) == 4) ? 1 : -1];
typedef struct {
  SPU32 lo, hi;
} SPId;
#ifndef SP_VEC3_DEFINED
#define SP_VEC3_DEFINED
typedef struct {
  float x, y, z;
} SPVec3;
#endif
typedef struct {
  SPVec3 origin, right, up, forward;
} SPTransform;
#define SP_PLAYERS 16
#define SP_STATIONS 5
#define SP_PATH 64
#define SP_NAV_NODES 63
#define SP_MAX_BALANCE 1000000000u
#define SP_RADIUS 0.28f
#define SP_HEIGHT 0.70f
#define SP_HALF_HEIGHT 0.35f
#define SP_STEP_UP 0.45f
#define SP_STEP_DOWN 0.75f
#define SP_SLOPE_COS 0.65f
#define SP_FLOOR_DROP 8.f
#define SP_FALL_GRAVITY 1.62f
#define SP_FALL_MAX_SPEED 3.2f
#ifndef SP_TRACE_EDGE
#define SP_TRACE_EDGE(stage,from,to) ((void)0)
#endif
/* A ground query starts above the desired capsule center; return the first
 * solid foot contact beneath it along -up, within max_drop. Dust is optional
 * surface decoration, never the required support. A cast must check the whole
 * oriented capsule, including occupancy at from==to, and fail closed. Permit
 * floor skin contacts; support_id is a contact hint, never permission to ignore
 * walls belonging to the same building/vehicle. */
typedef struct {
  SPVec3 point, normal;
  SPU64 support_id;
} SPGround;
typedef struct {
  void *ctx;
  int (*up)(void *, SPVec3, SPVec3 *);
  int (*ground)(void *, SPVec3, SPVec3, float, SPGround *);
  int (*clear)(void *, SPVec3, SPVec3, SPVec3, float, float, SPU64);
  int (*support)(void *, SPU64, SPTransform *);
} SPWorld;
enum { SP_PURCHASE = 1, SP_SUMMON = 2, SP_PICKUP = 3, SP_DROP = 4 };
enum {
  SP_OK = 0,
  SP_BAD_INPUT = 1,
  SP_WRONG_WORLD = 2,
  SP_NO_PLAYER = 3,
  SP_OLD_TRANSACTION = 4,
  SP_TRANSACTION_CONFLICT = 5,
  SP_ALREADY_OWNED = 6,
  SP_NO_FUNDS = 7,
  SP_TOO_FAR = 8,
  SP_NO_STATION = 9,
  SP_NO_SPACE = 10,
  SP_NO_PET = 11,
  SP_RATE_LIMIT = 12,
  SP_FULL = 13
};
enum { SP_STARTER = 0, SP_NORTH = 1, SP_EAST = 2, SP_SOUTH = 3, SP_WEST = 4 };
enum {
  SP_BLUE = 0,
  SP_GOLD = 1,
  SP_EMERALD = 2,
  SP_AMETHYST = 3,
  SP_PEARL = 4
};
enum { SP_IDLE = 0, SP_FOLLOW = 1, SP_SLEEP = 2, SP_BLOCKED = 3, SP_HELD = 4 };
enum { SP_CONTENT = 0, SP_HAPPY = 1, SP_SLEEPY = 2, SP_CURIOUS = 3 };
/* position is the measured owner foot/floor point, never the camera. */
typedef struct {
  SPU64 world_id;
  SPVec3 position, up, heading;
} SPOwner;
typedef struct {
  SPU64 world_id, session_id, txn_id;
  SPU32 kind, station;
} SPCommand;
typedef struct {
  SPU64 txn_id, watermark;
  SPU32 code, balance, variant, debit, duplicate;
} SPResult;
typedef struct {
  SPU32 enabled, variant, cost;
  SPU64 world_id;
  SPVec3 position;
} SPStation;
typedef struct {
  SPId id;
  SPU32 used, balance, owned, variant;
  SPU64 last_txn;
  SPU32 last_kind, last_station, last_code;
  /* Latest accepted transaction result is copied, including rejections. */
  SPU32 last_balance, last_variant;
  SPOwner owner;
  SPU32 owner_valid, active, mode, emotion, blink;
  SPVec3 position, up, heading;
  SPU64 support_id;
  SPTransform support_frame;
  SPU32 support_valid;
  SPVec3 path[SP_PATH];
  SPU32 path_count, path_index;
  float idle_time, replan_time, summon_time, recovery_time, blink_time,
      happy_time;
  /* Transient host motion only; wire snapshots and durable records exclude it.
   */
  float fall_speed;
} SPPet;
typedef struct {
  SPVec3 p;
  float g, f;
  int parent;
  SPU32 flags;
  SPU64 support;
} SPNavNode;
typedef struct {
  SPU64 world_id, session_id;
  SPU32 tick;
  SPStation stations[SP_STATIONS];
  SPPet players[SP_PLAYERS];
  /* Shared scratch keeps the native call stack below Win64 stack-probe size. */
  SPNavNode nav[SP_NAV_NODES];
  SPU32 rejected, plans, blocked, teleports;
} SPCore;
#ifdef __cplusplus
extern "C" {
#endif
void sp_init(SPCore *, SPU64 world_id, SPU64 session_id);
SPU32 sp_join(SPCore *, SPId, SPU32 authoritative_balance);
SPPet *sp_player(SPCore *, SPId);
SPU32 sp_balance(SPCore *, SPId, SPU32 authoritative_balance);
SPU32 sp_owner(SPCore *, SPId, const SPOwner *);
void sp_leave(SPCore *, SPId); /* retains purchase and transaction state */
SPU32 sp_station(SPCore *, SPU32, const SPStation *);
SPResult sp_command(SPCore *, SPId authenticated_sender, const SPCommand *,
                    const SPWorld *);
SPU32 sp_tick(SPCore *, float dt, const SPWorld *); /* dt in (0,.1] */
int sp_finite(float);
int sp_vec_valid(SPVec3);
int sp_unit(SPVec3, SPVec3 *);
SPVec3 sp_v(float, float, float);
SPVec3 sp_add(SPVec3, SPVec3);
SPVec3 sp_sub(SPVec3, SPVec3);
SPVec3 sp_mul(SPVec3, float);
float sp_dot(SPVec3, SPVec3);
float sp_length(SPVec3);
#ifdef __cplusplus
}
#endif
#endif

#ifdef SP_IMPLEMENTATION
#ifndef MOONLAB_SLIME_PET_CORE_IMPLEMENTED
#define MOONLAB_SLIME_PET_CORE_IMPLEMENTED
static void sp_zero(void *dst, SPU32 n) {
  unsigned char *p = (unsigned char *)dst;
  while (n--)
    *p++ = 0;
}
int sp_finite(float x) {
  union {
    float f;
    SPU32 u;
  } v;
  v.f = x;
  return (v.u & 0x7f800000u) != 0x7f800000u;
}
static float sp_abs(float x) { return x < 0 ? -x : x; }
static float sp_sqrt(float x) {
  union {
    float f;
    SPU32 u;
  } v;
  int i;
  if (x <= 0)
    return 0;
  v.f = x;
  v.u = (v.u >> 1) + 0x1fc00000u;
  for (i = 0; i < 5; i++)
    v.f = .5f * (v.f + x / v.f);
  return v.f;
}
SPVec3 sp_v(float x, float y, float z) {
  SPVec3 p;
  p.x = x;
  p.y = y;
  p.z = z;
  return p;
}
SPVec3 sp_add(SPVec3 a, SPVec3 b) {
  return sp_v(a.x + b.x, a.y + b.y, a.z + b.z);
}
SPVec3 sp_sub(SPVec3 a, SPVec3 b) {
  return sp_v(a.x - b.x, a.y - b.y, a.z - b.z);
}
SPVec3 sp_mul(SPVec3 a, float s) { return sp_v(a.x * s, a.y * s, a.z * s); }
float sp_dot(SPVec3 a, SPVec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float sp_length(SPVec3 a) { return sp_sqrt(sp_dot(a, a)); }
static SPVec3 sp_cross(SPVec3 a, SPVec3 b) {
  return sp_v(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}
int sp_vec_valid(SPVec3 p) {
  return sp_finite(p.x) && sp_finite(p.y) && sp_finite(p.z) &&
         sp_abs(p.x) <= 10000000.f && sp_abs(p.y) <= 10000000.f &&
         sp_abs(p.z) <= 10000000.f;
}
int sp_unit(SPVec3 p, SPVec3 *out) {
  float n;
  if (!out || !sp_vec_valid(p))
    return 0;
  n = sp_dot(p, p);
  if (n < .000001f || n > 1e15f)
    return 0;
  *out = sp_mul(p, 1.f / sp_sqrt(n));
  return 1;
}
static int sp_id_valid(SPId id) { return id.lo || id.hi; }
static int sp_id_eq(SPId a, SPId b) { return a.lo == b.lo && a.hi == b.hi; }
static SPVec3 sp_tangent(SPVec3 h, SPVec3 up) {
  SPVec3 out;
  h = sp_sub(h, sp_mul(up, sp_dot(h, up)));
  if (sp_unit(h, &out))
    return out;
  h = sp_abs(up.x) < .8f ? sp_v(1, 0, 0) : sp_v(0, 0, 1);
  sp_unit(sp_cross(h, up), &out);
  return out;
}
static int sp_world_valid(const SPWorld *w) {
  return w && w->up && w->ground && w->clear;
}
static int sp_up(const SPWorld *w, SPVec3 p, SPVec3 *up) {
  SPVec3 v;
  return w->up(w->ctx, p, &v) && sp_unit(v, up);
}
static void sp_count(SPU32 *p) {
  if (*p != 0xffffffffu)
    (*p)++;
}
void sp_init(SPCore *c, SPU64 world, SPU64 session) {
  if (!c)
    return;
  sp_zero(c, (SPU32)sizeof(*c));
  c->world_id = world;
  c->session_id = session;
}
SPPet *sp_player(SPCore *c, SPId id) {
  int i;
  if (!c || !sp_id_valid(id))
    return 0;
  for (i = 0; i < SP_PLAYERS; i++)
    if (c->players[i].used && sp_id_eq(c->players[i].id, id))
      return &c->players[i];
  return 0;
}
SPU32 sp_join(SPCore *c, SPId id, SPU32 balance) {
  SPPet *p;
  int i;
  if (!c || !c->world_id || !c->session_id || !sp_id_valid(id) ||
      balance > SP_MAX_BALANCE)
    return SP_BAD_INPUT;
  p = sp_player(c, id);
  if (p)
    return SP_OK;
  for (i = 0; i < SP_PLAYERS; i++)
    if (!c->players[i].used) {
      p = &c->players[i];
      sp_zero(p, (SPU32)sizeof(*p));
      p->id = id;
      p->used = 1;
      p->balance = balance;
      p->blink_time = 2.f + (float)i * .13f;
      return SP_OK;
    }
  return SP_FULL;
}
SPU32 sp_balance(SPCore *c, SPId id, SPU32 balance) {
  SPPet *p = sp_player(c, id);
  if (!p)
    return SP_NO_PLAYER;
  if (balance > SP_MAX_BALANCE)
    return SP_BAD_INPUT;
  p->balance = balance;
  return SP_OK;
}
SPU32 sp_owner(SPCore *c, SPId id, const SPOwner *owner) {
  SPPet *p = sp_player(c, id);
  SPVec3 up;
  if (!p)
    return SP_NO_PLAYER;
  if (!owner || owner->world_id != c->world_id ||
      !sp_vec_valid(owner->position) || !sp_unit(owner->up, &up) ||
      !sp_vec_valid(owner->heading))
    return SP_BAD_INPUT;
  p->owner = *owner;
  p->owner.up = up;
  p->owner.heading = sp_tangent(owner->heading, up);
  p->owner_valid = 1;
  return SP_OK;
}
void sp_leave(SPCore *c, SPId id) {
  SPPet *p = sp_player(c, id);
  if (!p)
    return;
  p->owner_valid = 0;
  p->active = 0;
  p->path_count = 0;
  p->support_valid = 0;
  p->fall_speed = 0;
}
SPU32 sp_station(SPCore *c, SPU32 n, const SPStation *s) {
  if (!c || !s || n >= SP_STATIONS || s->enabled > 1 ||
      s->variant >= SP_STATIONS || s->cost > SP_MAX_BALANCE ||
      s->world_id != c->world_id || !sp_vec_valid(s->position))
    return SP_BAD_INPUT;
  c->stations[n] = *s;
  return SP_OK;
}
static int sp_grounded(const SPWorld *w, SPVec3 desired, float rise, float drop,
                       SPVec3 *center, SPVec3 *up, SPGround *g) {
  SPVec3 normal, origin;
  float delta;
  if (!sp_up(w, desired, up))
    return 0;
  origin = sp_add(desired, sp_mul(*up, rise));
  if (!w->ground(w->ctx, origin, *up, rise + SP_HALF_HEIGHT + drop, g) ||
      !sp_vec_valid(g->point) || !sp_unit(g->normal, &normal) ||
      sp_dot(normal, *up) < SP_SLOPE_COS)
    return 0;
  delta = sp_dot(sp_sub(origin, g->point), *up);
  if (delta < -.001f || delta > rise + SP_HALF_HEIGHT + drop + .01f)
    return 0; /* Reject callbacks returning a side contact far outside their
                 ray. */
  if (sp_length(sp_sub(sp_sub(origin, g->point), sp_mul(*up, delta))) > .08f)
    return 0;
  *center = sp_add(g->point, sp_mul(*up, SP_HALF_HEIGHT));
  return sp_vec_valid(*center);
}
static int sp_crowded(SPCore *c, const SPPet *pet, SPVec3 p) {
  int i;
  for (i = 0; i < SP_PLAYERS; i++) {
    SPPet *o = &c->players[i];
    if (o != pet && o->active &&
        sp_length(sp_sub(o->position, p)) < SP_RADIUS * 1.8f)
      return 1;
  }
  return 0;
}
/* Every edge is split into short surface contacts. No interpolation segment
 * bypasses a cast, and a failed segment commits no ordinary movement. */
static int sp_edge(SPCore *c, SPPet *pet, const SPWorld *w, SPVec3 from,
                   SPVec3 to, SPVec3 *end, SPVec3 *end_up, SPGround *end_g,
                   int crowd) {
  float length = sp_length(sp_sub(to, from));
  int steps = (int)(length / .32f) + 1, i;
  SPVec3 cur = from, up, prev_up, next;
  SPGround g;
  if (steps > 32 || !sp_up(w, from, &prev_up))
    return 0;
  for (i = 1; i <= steps; i++) {
    SPVec3 desired =
        sp_add(from, sp_mul(sp_sub(to, from), (float)i / (float)steps));
    SPVec3 direction = sp_tangent(sp_sub(to, from), prev_up);
    float height;
    int side, clear;
    if (!sp_grounded(w, desired, SP_STEP_UP, SP_STEP_DOWN, &next, &up, &g))
      return 0;
    /* The capsule footprint can contact a threshold before its centre ray.
     * Front/rear probes also retain support until a descending edge clears.
     * A two-centimetre skin avoids a sub-tick dead zone at the riser. */
    for (side = -1; side <= 1; side += 2) {
      SPVec3 contact, contact_up;
      SPGround sample;
      SPVec3 query =
          sp_add(desired, sp_mul(direction, (SP_RADIUS + .02f) * (float)side));
      if (sp_grounded(w, query, SP_STEP_UP, SP_STEP_DOWN, &contact, &contact_up,
                      &sample)) {
        SPVec3 normal, sample_normal;
        if (!sp_unit(g.normal, &normal) ||
            !sp_unit(sample.normal, &sample_normal))
          continue;
        /* A ramp's higher leading contact lies on the same plane. Treating
         * it as a step lifts the logical foot every frame, then settling
         * spends the following frames falling back instead of following. */
        float cosine = sp_dot(sample_normal, up);
        float discontinuity = sp_dot(sp_sub(sample.point, g.point), normal);
        float rise = cosine >= SP_SLOPE_COS
            ? sp_dot(sp_sub(sample.point, sp_sub(next, sp_mul(up, SP_HALF_HEIGHT))),
                     sample_normal) / cosine : 0;
        if (discontinuity > .025f && rise > .005f) {
          next = sp_add(next, sp_mul(up, rise));
          g = sample;
        }
      }
    }
    height = sp_dot(sp_sub(next, cur), prev_up);
    if (height > SP_STEP_UP + .01f || height < -(SP_STEP_DOWN + .01f) ||
        (crowd && sp_crowded(c, pet, next)))
      return 0;
    clear = w->clear(w->ctx, cur, next, up, SP_RADIUS, SP_HEIGHT, g.support_id);
    if (!clear && height > .005f) {
      SPVec3 lifted = sp_add(cur, sp_mul(prev_up, height + .025f));
      SPVec3 over = sp_add(next, sp_mul(up, .025f));
      clear =
          w->clear(w->ctx, cur, lifted, prev_up, SP_RADIUS, SP_HEIGHT,
                   g.support_id) &&
          w->clear(w->ctx, lifted, over, up, SP_RADIUS, SP_HEIGHT,
                   g.support_id) &&
          w->clear(w->ctx, over, next, up, SP_RADIUS, SP_HEIGHT, g.support_id);
    }
    if (!clear && height < -.005f) {
      /* Descending across a floor seam needs the reverse of step-up: move
       * over the edge at the old height, then lower onto verified support.
       * A diagonal sweep otherwise collides with the floor being left. */
      SPVec3 lifted=sp_add(cur,sp_mul(prev_up,.025f));
      SPVec3 over=sp_add(next,sp_mul(up,-height+.025f));
      clear=w->clear(w->ctx,cur,lifted,prev_up,SP_RADIUS,SP_HEIGHT,g.support_id)&&
            w->clear(w->ctx,lifted,over,up,SP_RADIUS,SP_HEIGHT,g.support_id)&&
            w->clear(w->ctx,over,next,up,SP_RADIUS,SP_HEIGHT,g.support_id);
    }
    if (!clear) {
      SP_TRACE_EDGE("edge-clearance",cur,next);
      return 0;
    }
    cur = next;
    prev_up = up;
  }
  *end = cur;
  *end_up = up;
  *end_g = g;
  return 1;
}
static int sp_frame_valid(SPTransform *t) {
  SPVec3 r, u, f;
  return sp_vec_valid(t->origin) && sp_unit(t->right, &r) &&
         sp_unit(t->up, &u) && sp_unit(t->forward, &f) &&
         sp_abs(sp_dot(t->right, t->right) - 1) < .02f &&
         sp_abs(sp_dot(t->up, t->up) - 1) < .02f &&
         sp_abs(sp_dot(t->forward, t->forward) - 1) < .02f &&
         sp_abs(sp_dot(r, u)) < .01f && sp_abs(sp_dot(r, f)) < .01f &&
         sp_abs(sp_dot(u, f)) < .01f && sp_dot(sp_cross(r, u), f) > .98f;
}
static void sp_contact(SPPet *p, const SPWorld *w, SPGround *g) {
  p->fall_speed = 0;
  p->support_id = g->support_id;
  p->support_valid = 0;
  if (g->support_id && w->support &&
      w->support(w->ctx, g->support_id, &p->support_frame) &&
      sp_frame_valid(&p->support_frame))
    p->support_valid = 1;
}
static int sp_safe_near(SPCore *c, SPPet *p, const SPWorld *w) {
  static const float cs[8] = {1,  .70710678f,  0, -.70710678f,
                              -1, -.70710678f, 0, .70710678f};
  static const float sn[8] = {0, .70710678f,  1,  .70710678f,
                              0, -.70710678f, -1, -.70710678f};
  SPVec3 f = p->owner.heading, r = sp_cross(f, p->owner.up), center, up;
  SPGround g;
  int ring, i;
  for (ring = 0; ring < 3; ring++)
    for (i = 0; i < 8; i++) {
      SPVec3 dir = sp_add(sp_mul(f, -cs[i]), sp_mul(r, sn[i]));
      SPVec3 desired =
          sp_add(p->owner.position, sp_mul(dir, 1.35f + (float)ring * .6f));
      if (sp_grounded(w, desired, SP_STEP_UP, 1.4f, &center, &up, &g) &&
          sp_abs(sp_dot(sp_sub(g.point, p->owner.position), p->owner.up)) <=
              .6f &&
          !sp_crowded(c, p, center) &&
          w->clear(w->ctx, center, center, up, SP_RADIUS, SP_HEIGHT,
                   g.support_id)) {
        p->position = center;
        p->up = up;
        p->heading = sp_tangent(f, up);
        p->active = 1;
        p->mode = SP_IDLE;
        p->emotion = SP_HAPPY;
        p->happy_time = 3;
        p->idle_time = 0;
        p->fall_speed = 0;
        p->path_count = 0;
        p->support_valid = 0;
        p->recovery_time = 0;
        sp_contact(p, w, &g);
        sp_count(&c->teleports);
        return 1;
      }
    }
  return 0;
}
/* A carried pet belongs to its authenticated owner. Clients never supply a
 * held position. Clearance still applies to the entire pickup/drop sweep. */
static SPVec3 sp_hold_point(const SPPet *p) {
  return sp_add(p->owner.position,
                sp_add(sp_mul(p->owner.up, 1.05f),
                       sp_mul(p->owner.heading, .72f)));
}
static int sp_pickup(SPPet *p, const SPWorld *w) {
  SPVec3 held = sp_hold_point(p);
  if (!w->clear(w->ctx,p->position,held,p->owner.up,SP_RADIUS,SP_HEIGHT,0))
    return 0;
  p->position=held;p->up=p->owner.up;
  p->heading=sp_mul(p->owner.heading,-1);
  p->mode=SP_HELD;p->emotion=SP_HAPPY;p->happy_time=3;p->blink=0;p->blink_time=4.5f;
  p->path_count=0;p->support_valid=0;p->support_id=0;p->fall_speed=0;
  return 1;
}
static int sp_put_down(SPCore *c, SPPet *p, const SPWorld *w) {
  SPVec3 center,up;
  SPGround floor;
  SPVec3 desired=sp_add(p->owner.position,sp_mul(p->owner.heading,1.15f));
  if (!sp_grounded(w,desired,SP_STEP_UP,1.4f,&center,&up,&floor) ||
      sp_crowded(c,p,center) ||
      !w->clear(w->ctx,p->position,center,up,SP_RADIUS,SP_HEIGHT,floor.support_id))
    return 0;
  p->position=center;p->up=up;p->heading=sp_mul(p->owner.heading,-1);
  p->mode=SP_IDLE;p->emotion=SP_HAPPY;p->happy_time=3;p->idle_time=0;
  sp_contact(p,w,&floor);return 1;
}
static SPResult sp_result(SPPet *p, SPU64 txn, SPU32 code) {
  SPResult r;
  r.txn_id = txn;
  r.watermark = p ? p->last_txn : 0;
  r.code = code;
  r.balance = p ? p->balance : 0;
  r.variant = p ? p->variant : 0;
  r.debit = 0;
  r.duplicate = 0;
  return r;
}
SPResult sp_command(SPCore *c, SPId id, const SPCommand *cmd,
                    const SPWorld *w) {
  SPPet *p = sp_player(c, id);
  SPU32 code = SP_OK;
  if (!c || !cmd || !sp_world_valid(w) || !cmd->txn_id ||
      (cmd->kind < SP_PURCHASE || cmd->kind > SP_DROP) ||
      cmd->station >= SP_STATIONS)
    return sp_result(p, cmd ? cmd->txn_id : 0, SP_BAD_INPUT);
  if (cmd->world_id != c->world_id || cmd->session_id != c->session_id)
    return sp_result(p, cmd->txn_id, SP_WRONG_WORLD);
  if (!p)
    return sp_result(0, cmd->txn_id, SP_NO_PLAYER);
  if (cmd->txn_id == p->last_txn) {
    SPResult r;
    if (cmd->kind != p->last_kind || cmd->station != p->last_station)
      return sp_result(p, cmd->txn_id, SP_TRANSACTION_CONFLICT);
    r = sp_result(p, cmd->txn_id, p->last_code);
    r.balance = p->last_balance;
    r.variant = p->last_variant;
    r.duplicate = 1;
    return r;
  }
  if (cmd->txn_id < p->last_txn)
    return sp_result(p, cmd->txn_id, SP_OLD_TRANSACTION);
  if (!p->owner_valid || p->owner.world_id != c->world_id)
    code = SP_WRONG_WORLD;
  else if (cmd->kind == SP_PURCHASE) {
    SPStation *s = &c->stations[cmd->station];
    if (!s->enabled || s->world_id != c->world_id)
      code = SP_NO_STATION;
    else if (p->owned && p->variant == s->variant)
      code = SP_ALREADY_OWNED;
    else if (sp_length(sp_sub(p->owner.position, s->position)) > 4.f)
      code = SP_TOO_FAR;
    else if (p->balance < s->cost)
      code = SP_NO_FUNDS;
    else if (!sp_safe_near(c, p, w))
      code = SP_NO_SPACE;
    else {
      p->balance -= s->cost;
      p->owned = 1;
      p->variant = s->variant;
      p->summon_time = 1.f;
    }
  } else if (!p->owned || !p->active)
    code = SP_NO_PET;
  else if (cmd->kind == SP_PICKUP) {
    if (p->mode == SP_HELD) code = SP_ALREADY_OWNED;
    else if (sp_length(sp_sub(p->position,p->owner.position)) > 2.5f)
      code = SP_TOO_FAR;
    else if (!sp_pickup(p,w)) code = SP_NO_SPACE;
  } else if (cmd->kind == SP_DROP) {
    if (p->mode != SP_HELD) code = SP_BAD_INPUT;
    else if (!sp_put_down(c,p,w)) code = SP_NO_SPACE;
  } else if (p->summon_time > 0)
    code = SP_RATE_LIMIT;
  else if (!sp_safe_near(c, p, w))
    code = SP_NO_SPACE;
  else
    p->summon_time = 1.f;
  p->last_txn = cmd->txn_id;
  p->last_kind = cmd->kind;
  p->last_station = cmd->station;
  p->last_code = code;
  p->last_balance = p->balance;
  p->last_variant = p->variant;
  if (code != SP_OK)
    sp_count(&c->rejected);
  {
    SPResult r = sp_result(p, cmd->txn_id, code);
    if (code == SP_OK && cmd->kind == SP_PURCHASE)
      r.debit = c->stations[cmd->station].cost;
    return r;
  }
}
/* Local 9x7 cardinal A*: eight metres wide, one metre back/five forward.
 * Its bounded horizon may stop at a large enclosed obstacle. It never converts
 * that local failure into a wall-crossing teleport. Replan against live casts.
 */
static int sp_plan(SPCore *c, SPPet *p, const SPWorld *w, SPVec3 goal) {
  SPVec3 forward = sp_tangent(sp_sub(goal, p->position), p->up),
         right = sp_cross(forward, p->up);
  int i, j, start = 13, target = -1, best = -1, expanded;
  float target_dist = 1e30f;
  SPVec3 end, up;
  SPGround g;
  sp_count(&c->plans);
  p->path_count = 0;
  p->path_index = 0;
  for (i = 0; i < SP_NAV_NODES; i++) {
    SPNavNode *n = &c->nav[i];
    SPVec3 q = sp_add(p->position, sp_add(sp_mul(right, (float)(i % 9 - 4)),
                                          sp_mul(forward, (float)(i / 9 - 1))));
    n->flags = 0;
    n->parent = -1;
    n->g = 1e30f;
    n->f = 1e30f;
    if (sp_grounded(w, q, SP_STEP_UP, 1.4f, &n->p, &up, &g) &&
        w->clear(w->ctx, n->p, n->p, up, SP_RADIUS, SP_HEIGHT, g.support_id) &&
        !sp_crowded(c, p, n->p)) {
      float d = sp_length(sp_sub(n->p, goal));
      n->flags = 1;
      n->support = g.support_id;
      if (d < target_dist) {
        target_dist = d;
        target = i;
      }
    }
  }
  c->nav[start].p = p->position;
  c->nav[start].flags = 3;
  c->nav[start].g = 0;
  c->nav[start].f = sp_length(sp_sub(p->position, goal));
  if (target < 0 || target == start)
    return 0;
  for (expanded = 0; expanded < SP_NAV_NODES; expanded++) {
    float f = 1e30f;
    best = -1;
    for (i = 0; i < SP_NAV_NODES; i++)
      if ((c->nav[i].flags & 3) == 3 && !(c->nav[i].flags & 4) &&
          c->nav[i].f < f) {
        f = c->nav[i].f;
        best = i;
      }
    if (best < 0)
      break;
    if (best == target)
      break;
    c->nav[best].flags |= 4;
    for (j = 0; j < 4; j++) {
      int dx = j == 0 ? 1 : j == 1 ? -1 : 0, dz = j == 2 ? 1 : j == 3 ? -1 : 0;
      int x = best % 9 + dx, z = best / 9 + dz;
      SPNavNode *a = &c->nav[best], *n;
      float ng;
      if (x < 0 || x >= 9 || z < 0 || z >= 7)
        continue;
      i = z * 9 + x;
      n = &c->nav[i];
      if (!(n->flags & 1) || (n->flags & 4))
        continue;
      ng = a->g + sp_length(sp_sub(a->p, n->p));
      if (ng >= n->g)
        continue;
      if (!sp_edge(c, p, w, a->p, n->p, &end, &up, &g, 1))
        continue;
      n->g = ng;
      n->f = ng + sp_length(sp_sub(n->p, goal));
      n->parent = best;
      n->flags |= 2;
    }
  }
  if (best != target)
    return 0;
  i = target;
  while (i != start && p->path_count < SP_PATH) {
    p->path[p->path_count++] = c->nav[i].p;
    i = c->nav[i].parent;
    if (i < 0) {
      p->path_count = 0;
      return 0;
    }
  }
  for (i = 0; i < (int)p->path_count / 2; i++) {
    SPVec3 t = p->path[i];
    p->path[i] = p->path[p->path_count - 1 - (SPU32)i];
    p->path[p->path_count - 1 - (SPU32)i] = t;
  }
  return p->path_count != 0;
}
static void sp_carry(SPCore *c, SPPet *p, const SPWorld *w) {
  SPTransform t;
  SPVec3 local, rel, carried, up, end;
  SPGround g;
  if (!p->support_valid || !p->support_id || !w->support)
    return;
  if (!w->support(w->ctx, p->support_id, &t) || !sp_frame_valid(&t)) {
    p->support_valid = 0;
    return;
  }
  rel = sp_sub(p->position, p->support_frame.origin);
  local = sp_v(sp_dot(rel, p->support_frame.right),
               sp_dot(rel, p->support_frame.up),
               sp_dot(rel, p->support_frame.forward));
  carried =
      sp_add(t.origin,
             sp_add(sp_mul(t.right, local.x),
                    sp_add(sp_mul(t.up, local.y), sp_mul(t.forward, local.z))));
  if (sp_length(sp_sub(carried, p->position)) < .0001f) {
    p->support_frame = t;
    return;
  }
  if (sp_length(sp_sub(carried, p->position)) <= 8.f &&
      sp_grounded(w, carried, SP_STEP_UP, SP_STEP_DOWN, &end, &up, &g) &&
      w->clear(w->ctx, p->position, end, up, SP_RADIUS, SP_HEIGHT,
               p->support_id) &&
      !sp_crowded(c, p, end)) {
    p->position = end;
    p->up = up;
    p->heading = sp_tangent(
        sp_add(sp_mul(t.right, sp_dot(p->heading, p->support_frame.right)),
               sp_mul(t.forward, sp_dot(p->heading, p->support_frame.forward))),
        up);
    p->path_count = 0;
    sp_contact(p, w, &g);
  } else {
    p->support_valid = 0;
    p->path_count = 0;
  }
}
/* Refresh contact independently of owner liveness and support ID. A central
 * loaded floor is required before descent; lateral samples cannot authorize
 * falling into an unloaded centre. Probe origins stay below the capsule top
 * so an overhead roof cannot replace the floor beneath the pet. */
static int sp_settle(SPCore *c, SPPet *p, float dt, const SPWorld *w) {
  SPVec3 center, up, normal, foot, end, end_up, forward, right, central_point;
  SPGround floor;
  float height, fall = 0;
  int landed = 1;
  if (!sp_grounded(w, p->position, SP_STEP_UP - SP_HALF_HEIGHT, SP_FLOOR_DROP,
                   &center, &up, &floor))
    goto blocked;
  if (!sp_unit(floor.normal, &normal))
    goto blocked;
  foot = sp_sub(p->position, sp_mul(up, SP_HALF_HEIGHT));
  central_point = floor.point;
  height = sp_dot(sp_sub(center, p->position), up);
  forward = sp_tangent(p->heading, up);
  right = sp_cross(forward, up);
  /* Preserve the central logical foot on a slope. A discontinuous raised
   * threshold may support the footprint first, but its plane projects back
   * onto the central gravity ray rather than shifting/lifting by slope width.
   */
  for (int i = 0; i < 4; i++) {
    SPVec3 offset = i < 2 ? forward : right, sample_center, sample_up,
           sample_normal;
    SPGround sample;
    float denominator, candidate;
    offset = sp_mul(offset, (i % 2 ? 1.f : -1.f) * (SP_RADIUS + .02f));
    if (!sp_grounded(w, sp_add(p->position, offset),
                     SP_STEP_UP - SP_HALF_HEIGHT, SP_FLOOR_DROP, &sample_center,
                     &sample_up, &sample) ||
        sp_dot(sp_sub(sample.point, central_point), normal) <= .025f ||
        !sp_unit(sample.normal, &sample_normal))
      continue;
    denominator = sp_dot(sample_normal, up);
    if (denominator < SP_SLOPE_COS)
      continue;
    candidate = sp_dot(sp_sub(sample.point, foot), sample_normal) / denominator;
    if (candidate > height + .005f && candidate <= SP_STEP_UP + .01f) {
      height = candidate;
      center = sp_add(p->position, sp_mul(up, height));
      floor = sample;
    }
  }
  if (height > SP_STEP_UP + .01f || height < -(SP_FLOOR_DROP + .01f))
    goto blocked;
  if (height < -.015f) {
    fall = sp_finite(p->fall_speed) && p->fall_speed >= 0 &&
                   p->fall_speed <= SP_FALL_MAX_SPEED
               ? p->fall_speed
               : 0;
    fall += SP_FALL_GRAVITY * dt;
    if (fall > SP_FALL_MAX_SPEED)
      fall = SP_FALL_MAX_SPEED;
    float descent = fall * dt;
    if (descent < -height) {
      center = sp_sub(p->position, sp_mul(up, descent));
      landed = 0;
    }
  }
  end = center;
  if (!sp_vec_valid(end) || !sp_up(w, end, &end_up) ||
      !w->clear(w->ctx, p->position, end, up, SP_RADIUS, SP_HEIGHT,
                floor.support_id) ||
      sp_crowded(c, p, end))
    goto blocked;
  p->position = end;
  p->up = end_up;
  p->heading = sp_tangent(p->heading, end_up);
  if (landed) {
    sp_contact(p, w, &floor);
    return 1;
  }
  p->fall_speed = fall;
  p->support_id = 0;
  p->support_valid = 0;
  p->path_count = 0;
  p->path_index = 0;
  return 0;
blocked:
  SP_TRACE_EDGE("settle-clearance",p->position,p->position);
  p->fall_speed = 0;
  p->support_id = 0;
  p->support_valid = 0;
  p->path_count = 0;
  p->path_index = 0;
  return 0;
}

SPU32 sp_tick(SPCore *c, float dt, const SPWorld *w) {
  int i;
  if (!c || !sp_world_valid(w) || !sp_finite(dt) || dt <= 0 || dt > .1f)
    return SP_BAD_INPUT;
  c->tick++;
  for (i = 0; i < SP_PLAYERS; i++) {
    SPPet *p = &c->players[i];
    SPVec3 goal, delta, to, end, up;
    SPGround g;
    float dist, step;
    if (!p->used)
      continue;
    if (p->summon_time > 0)
      p->summon_time -= dt;
    if (!p->owned)
      continue;
    if (!p->active) {
      if (p->owner_valid)
        sp_safe_near(c, p, w);
      continue;
    }
    if (p->mode == SP_HELD) {
      if (p->owner_valid) {
        SPVec3 held=sp_hold_point(p);
        /* Fail closed against walls while moving. A large discontinuity
         * releases through the same checked floor placement as E/drop. */
        if (sp_length(sp_sub(held,p->position)) < 3.f &&
            w->clear(w->ctx,p->position,held,p->owner.up,SP_RADIUS,SP_HEIGHT,0)) {
          p->position=held;p->up=p->owner.up;
          p->heading=sp_mul(p->owner.heading,-1);
        } else sp_put_down(c,p,w);
      }
      p->blink_time-=dt;p->blink=p->blink_time<.10f;
      if(p->blink_time<=0)p->blink_time=4.5f;
      p->emotion=SP_HAPPY;
      continue;
    }
    sp_carry(c, p, w);
    int grounded = sp_settle(c, p, dt, w);
    p->blink_time -= dt;
    p->blink = 0;
    if (p->blink_time < .12f && p->mode != SP_SLEEP)
      p->blink = 1;
    if (p->blink_time <= 0)
      p->blink_time = 2.5f + (float)((c->tick + (SPU32)i * 17) % 19) * .1f;
    if (p->happy_time > 0)
      p->happy_time -= dt;
    p->emotion = p->happy_time > 0 ? SP_HAPPY : SP_CONTENT;
    if (!grounded) {
      p->mode = SP_BLOCKED;
      p->emotion = SP_CURIOUS;
      sp_count(&c->blocked);
      continue;
    }
    if (!p->owner_valid) {
      p->path_count = 0;
      p->idle_time += dt;
      p->recovery_time = 0;
      p->mode = p->idle_time >= 8.f ? SP_SLEEP : SP_IDLE;
      if (p->mode == SP_SLEEP) {
        p->blink = 1;
        p->emotion = SP_SLEEPY;
      }
      continue;
    }
    goal = sp_sub(p->owner.position, sp_mul(p->owner.heading, 1.35f));
    dist = sp_length(sp_sub(p->owner.position, p->position));
    if (dist > 80.f) {
      p->recovery_time += dt;
      if (p->recovery_time >= 5.f) {
        if (sp_safe_near(c, p, w))
          p->happy_time = 1.f;
        else
          p->recovery_time = 4.f;
      }
      p->mode = SP_BLOCKED;
      continue;
    }
    p->recovery_time = 0;
    if (dist < 2.f) {
      p->idle_time += dt;
      p->path_count = 0;
      p->mode = p->idle_time >= 8.f ? SP_SLEEP : SP_IDLE;
      if (p->mode == SP_SLEEP) {
        p->blink = 1;
        p->emotion = SP_SLEEPY;
      }
      continue;
    }
    p->idle_time = 0;
    p->mode = SP_FOLLOW;
    p->replan_time -= dt;
    delta = sp_sub(goal, p->position);
    float speed=dist>7.f?7.f:dist>4.f?5.5f:4.5f;
    float ahead=sp_length(delta);SPVec3 direct=delta;
    if(ahead>speed*dt)direct=sp_mul(delta,speed*dt/ahead);
    /* An obstruction several metres ahead must not veto a safe step here.
     * Check actual travel first; only blocked travel asks the local planner. */
    to=sp_add(p->position,direct);
    if(sp_edge(c,p,w,p->position,to,&end,&up,&g,1)){
      p->heading=sp_tangent(direct,up);p->position=end;p->up=up;sp_contact(p,w,&g);
      p->path_count=0;p->path_index=0;continue;
    }
    if (p->path_index >= p->path_count || p->replan_time <= 0) {
      sp_plan(c, p, w, goal);
      p->replan_time = 1.f;
    }
    while (p->path_index < p->path_count &&
           sp_length(sp_sub(p->path[p->path_index], p->position)) < .18f)
      p->path_index++;
    if (p->path_index >= p->path_count) {
      p->mode = SP_BLOCKED;
      p->emotion = SP_CURIOUS;
      sp_count(&c->blocked);
      continue;
    }
    goal = p->path[p->path_index];
    delta = sp_sub(goal, p->position);
    step = sp_length(delta);
    if (step < .001f)
      continue;
    if (step > speed * dt)
      delta = sp_mul(delta, speed * dt / step);
    to = sp_add(p->position, delta);
    if (sp_edge(c, p, w, p->position, to, &end, &up, &g, 1)) {
      p->heading = sp_tangent(delta, up);
      p->position = end;
      p->up = up;
      sp_contact(p, w, &g);
    } else {
      p->path_count = 0;
      p->mode = SP_BLOCKED;
      p->emotion = SP_CURIOUS;
      sp_count(&c->blocked);
    }
  }
  return SP_OK;
}
#endif
#endif
