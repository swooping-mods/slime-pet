#ifndef MOONLAB_SLIME_PET_PROTOCOL_H
#define MOONLAB_SLIME_PET_PROTOCOL_H
#include "pet_core.h"
/* Explicit little endian bytes, never raw structure layouts. Sender identity
 * comes from authenticated transport, not from command payload. */
#define SP_SESSION_VERSION 2u /* Held mode/pickup/drop require matching peers. */
#define SP_COMMAND_BYTES 40u
#define SP_RESULT_BYTES 64u
#define SP_SNAPSHOT_MAX (32u + SP_PLAYERS * 64u)
#define SP_PERSIST_MAX (32u + SP_PLAYERS * 64u)
typedef struct {
  SPId id;
  SPVec3 position, up, heading;
  SPU32 variant, mode, emotion, blink;
} SPReplicaPet;
typedef struct {
  SPU64 world_id, session_id;
  SPU32 tick, has_snapshot, count;
  SPReplicaPet pets[SP_PLAYERS];
} SPReplica;
#ifdef __cplusplus
extern "C" {
#endif
SPU32 sp_command_write(const SPCommand *, unsigned char *, SPU32 capacity);
int sp_command_read(SPCommand *, const unsigned char *, SPU32 size);
SPU32 sp_result_write(const SPResult *, SPU64 world_id, SPU64 session_id,
                      unsigned char *, SPU32 capacity);
int sp_result_read(SPResult *, SPU64 expected_world, SPU64 expected_session,
                   const unsigned char *, SPU32 size);
SPU32 sp_snapshot_write(const SPCore *, unsigned char *, SPU32 capacity);
int sp_snapshot_read(SPReplica *, SPU64 expected_world, SPU64 expected_session,
                     const unsigned char *, SPU32 size);
/* Save only durable policy, not scene pointers, transforms, paths or sessions.
 * Load requires sp_init(new_session), preserves configured stations, validates
 * all records before any mutation, and leaves pets inactive until owner joins.
 * Adapter must atomically persist this state with its vanilla wallet debit. */
SPU32 sp_persist_write(const SPCore *, unsigned char *, SPU32 capacity);
int sp_persist_read(SPCore *, const unsigned char *, SPU32 size);
#ifdef __cplusplus
}
#endif
#endif
#ifdef SP_IMPLEMENTATION
#ifndef MOONLAB_SLIME_PET_PROTOCOL_IMPLEMENTED
#define MOONLAB_SLIME_PET_PROTOCOL_IMPLEMENTED
static SPU32 sp_r32(const unsigned char *b) {
  return (SPU32)b[0] | ((SPU32)b[1] << 8) | ((SPU32)b[2] << 16) |
         ((SPU32)b[3] << 24);
}
static SPU64 sp_r64(const unsigned char *b) {
  return (SPU64)sp_r32(b) | ((SPU64)sp_r32(b + 4) << 32);
}
static void sp_w32(unsigned char *b, SPU32 n) {
  b[0] = (unsigned char)n;
  b[1] = (unsigned char)(n >> 8);
  b[2] = (unsigned char)(n >> 16);
  b[3] = (unsigned char)(n >> 24);
}
static void sp_w64(unsigned char *b, SPU64 n) {
  sp_w32(b, (SPU32)n);
  sp_w32(b + 4, (SPU32)(n >> 32));
}
static void sp_wvec(unsigned char *b, SPVec3 p) {
  union {
    float f;
    SPU32 u;
  } v;
  v.f = p.x;
  sp_w32(b, v.u);
  v.f = p.y;
  sp_w32(b + 4, v.u);
  v.f = p.z;
  sp_w32(b + 8, v.u);
}
static SPVec3 sp_rvec(const unsigned char *b) {
  union {
    float f;
    SPU32 u;
  } v;
  SPVec3 p;
  v.u = sp_r32(b);
  p.x = v.f;
  v.u = sp_r32(b + 4);
  p.y = v.f;
  v.u = sp_r32(b + 8);
  p.z = v.f;
  return p;
}
static int sp_wire_unit(SPVec3 p) {
  float d = sp_dot(p, p);
  return sp_vec_valid(p) && d > .98f && d < 1.02f;
}
SPU32 sp_command_write(const SPCommand *c, unsigned char *b, SPU32 cap) {
  if (!c || !b || cap < SP_COMMAND_BYTES || !c->world_id || !c->session_id ||
      !c->txn_id || (c->kind < SP_PURCHASE || c->kind > SP_DROP) ||
      c->station >= SP_STATIONS)
    return 0;
  sp_w32(b, 0x31305053u);
  sp_w32(b + 4, c->kind);
  sp_w64(b + 8, c->world_id);
  sp_w64(b + 16, c->session_id);
  sp_w64(b + 24, c->txn_id);
  sp_w32(b + 32, c->station);
  sp_w32(b + 36, 0);
  return SP_COMMAND_BYTES;
}
int sp_command_read(SPCommand *c, const unsigned char *b, SPU32 size) {
  SPCommand n;
  if (!c || !b || size != SP_COMMAND_BYTES || sp_r32(b) != 0x31305053u ||
      sp_r32(b + 36))
    return 0;
  n.kind = sp_r32(b + 4);
  n.world_id = sp_r64(b + 8);
  n.session_id = sp_r64(b + 16);
  n.txn_id = sp_r64(b + 24);
  n.station = sp_r32(b + 32);
  if (!n.world_id || !n.session_id || !n.txn_id ||
      (n.kind < SP_PURCHASE || n.kind > SP_DROP) ||
      n.station >= SP_STATIONS)
    return 0;
  *c = n;
  return 1;
}
static int sp_result_valid(const SPResult *r) {
  return r->txn_id && r->code <= SP_FULL && r->balance <= SP_MAX_BALANCE &&
         r->variant < SP_STATIONS && r->debit <= SP_MAX_BALANCE &&
         r->duplicate <= 1 &&
         (!(r->code != SP_OK || r->duplicate) || !r->debit);
}
SPU32 sp_result_write(const SPResult *r, SPU64 world, SPU64 session,
                      unsigned char *b, SPU32 cap) {
  if (!r || !b || !world || !session || cap < SP_RESULT_BYTES ||
      !sp_result_valid(r))
    return 0;
  sp_w32(b, 0x31305253u);
  sp_w32(b + 4, 1);
  sp_w64(b + 8, world);
  sp_w64(b + 16, session);
  sp_w64(b + 24, r->txn_id);
  sp_w32(b + 32, r->code);
  sp_w32(b + 36, r->balance);
  sp_w32(b + 40, r->variant);
  sp_w32(b + 44, r->debit);
  sp_w32(b + 48, r->duplicate);
  sp_w32(b + 52, 0);
  sp_w64(b + 56, r->watermark);
  return SP_RESULT_BYTES;
}
int sp_result_read(SPResult *r, SPU64 world, SPU64 session,
                   const unsigned char *b, SPU32 size) {
  SPResult n;
  if (!r || !b || !world || !session || size != SP_RESULT_BYTES ||
      sp_r32(b) != 0x31305253u || sp_r32(b + 4) != 1 ||
      sp_r64(b + 8) != world || sp_r64(b + 16) != session || sp_r32(b + 52))
    return 0;
  n.txn_id = sp_r64(b + 24);
  n.watermark = sp_r64(b + 56);
  n.code = sp_r32(b + 32);
  n.balance = sp_r32(b + 36);
  n.variant = sp_r32(b + 40);
  n.debit = sp_r32(b + 44);
  n.duplicate = sp_r32(b + 48);
  if (!sp_result_valid(&n))
    return 0;
  *r = n;
  return 1;
}

SPU32 sp_snapshot_write(const SPCore *c, unsigned char *b, SPU32 cap) {
  SPU32 i, n = 0, off = 32;
  if (!c || !b || !c->world_id || !c->session_id)
    return 0;
  for (i = 0; i < SP_PLAYERS; i++)
    if (c->players[i].active)
      n++;
  if (cap < 32 + n * 64)
    return 0;
  sp_w32(b, 0x31305353u);
  sp_w32(b + 4, 1);
  sp_w64(b + 8, c->world_id);
  sp_w64(b + 16, c->session_id);
  sp_w32(b + 24, c->tick);
  sp_w32(b + 28, n);
  for (i = 0; i < SP_PLAYERS; i++) {
    const SPPet *p = &c->players[i];
    if (!p->active)
      continue;
    sp_w32(b + off, p->id.lo);
    sp_w32(b + off + 4, p->id.hi);
    sp_wvec(b + off + 8, p->position);
    sp_wvec(b + off + 20, p->up);
    sp_wvec(b + off + 32, p->heading);
    sp_w32(b + off + 44, p->variant);
    sp_w32(b + off + 48, p->mode);
    sp_w32(b + off + 52, p->emotion);
    sp_w32(b + off + 56, p->blink);
    sp_w32(b + off + 60, 0);
    off += 64;
  }
  return off;
}
int sp_snapshot_read(SPReplica *v, SPU64 world, SPU64 session,
                     const unsigned char *b, SPU32 size) {
  SPU32 count, i, j, tick, off;
  SPU32 difference;
  if (!v || !b || !world || !session || size < 32 || sp_r32(b) != 0x31305353u ||
      sp_r32(b + 4) != 1 || sp_r64(b + 8) != world || sp_r64(b + 16) != session)
    return 0;
  count = sp_r32(b + 28);
  tick = sp_r32(b + 24);
  if (count > SP_PLAYERS || size != 32 + count * 64)
    return 0;
  if (v->has_snapshot && v->world_id == world && v->session_id == session) {
    difference = tick - v->tick;
    if (!difference || difference >= 0x80000000u)
      return 0;
  }
  for (i = 0; i < count; i++) {
    SPVec3 up, heading;
    off = 32 + i * 64;
    if (!sp_r64(b + off) || !sp_vec_valid(sp_rvec(b + off + 8)))
      return 0;
    up = sp_rvec(b + off + 20);
    heading = sp_rvec(b + off + 32);
    if (!sp_wire_unit(up) || !sp_wire_unit(heading) ||
        sp_abs(sp_dot(up, heading)) > .05f ||
        sp_r32(b + off + 44) >= SP_STATIONS ||
        sp_r32(b + off + 48) > SP_HELD ||
        sp_r32(b + off + 52) > SP_CURIOUS || sp_r32(b + off + 56) > 1 ||
        sp_r32(b + off + 60))
      return 0;
    for (j = 0; j < i; j++)
      if (sp_r64(b + 32 + j * 64) == sp_r64(b + off))
        return 0;
  }
  v->world_id = world;
  v->session_id = session;
  v->tick = tick;
  v->count = count;
  v->has_snapshot = 1;
  for (i = 0; i < count; i++) {
    SPReplicaPet *p = &v->pets[i];
    off = 32 + i * 64;
    p->id.lo = sp_r32(b + off);
    p->id.hi = sp_r32(b + off + 4);
    p->position = sp_rvec(b + off + 8);
    p->up = sp_rvec(b + off + 20);
    p->heading = sp_rvec(b + off + 32);
    p->variant = sp_r32(b + off + 44);
    p->mode = sp_r32(b + off + 48);
    p->emotion = sp_r32(b + off + 52);
    p->blink = sp_r32(b + off + 56);
  }
  return 1;
}
SPU32 sp_persist_write(const SPCore *c, unsigned char *b, SPU32 cap) {
  SPU32 i, n = 0, off = 32;
  if (!c || !b || !c->world_id)
    return 0;
  for (i = 0; i < SP_PLAYERS; i++)
    if (c->players[i].used)
      n++;
  if (cap < 32 + n * 64)
    return 0;
  sp_zero(b, 32 + n * 64);
  sp_w32(b, 0x31305653u);
  sp_w32(b + 4, 1);
  sp_w64(b + 8, c->world_id);
  sp_w32(b + 16, n);
  sp_w32(b + 20, 64);
  for (i = 0; i < SP_PLAYERS; i++) {
    const SPPet *p = &c->players[i];
    if (!p->used)
      continue;
    sp_w32(b + off, p->id.lo);
    sp_w32(b + off + 4, p->id.hi);
    sp_w32(b + off + 8, p->balance);
    sp_w32(b + off + 12, p->owned);
    sp_w32(b + off + 16, p->variant);
    sp_w32(b + off + 20, p->last_kind);
    sp_w32(b + off + 24, p->last_station);
    sp_w32(b + off + 28, p->last_code);
    sp_w64(b + off + 32, p->last_txn);
    sp_w32(b + off + 40, p->last_balance);
    sp_w32(b + off + 44, p->last_variant);
    off += 64;
  }
  return off;
}
int sp_persist_read(SPCore *c, const unsigned char *b, SPU32 size) {
  SPU32 n, i, j, off;
  if (!c || !b || !c->world_id || !c->session_id || size < 32 ||
      sp_r32(b) != 0x31305653u || sp_r32(b + 4) != 1 ||
      sp_r64(b + 8) != c->world_id || sp_r32(b + 20) != 64 || sp_r64(b + 24))
    return 0;
  n = sp_r32(b + 16);
  if (n > SP_PLAYERS || size != 32 + n * 64)
    return 0;
  for (i = 0; i < n; i++) {
    SPU32 owned, variant, kind;
    SPU64 txn;
    off = 32 + i * 64;
    owned = sp_r32(b + off + 12);
    variant = sp_r32(b + off + 16);
    kind = sp_r32(b + off + 20);
    txn = sp_r64(b + off + 32);
    if (!sp_r64(b + off) || sp_r32(b + off + 8) > SP_MAX_BALANCE || owned > 1 ||
        variant >= SP_STATIONS || (!owned && variant) ||
        sp_r32(b + off + 24) >= SP_STATIONS || sp_r32(b + off + 28) > SP_FULL ||
        sp_r32(b + off + 40) > SP_MAX_BALANCE ||
        sp_r32(b + off + 44) >= SP_STATIONS || sp_r64(b + off + 48) ||
        sp_r64(b + off + 56))
      return 0;
    if ((txn && (kind < SP_PURCHASE || kind > SP_DROP)) ||
        (!txn && (kind || sp_r32(b + off + 24) || sp_r32(b + off + 28) ||
                  sp_r32(b + off + 40) || sp_r32(b + off + 44))))
      return 0;
    for (j = 0; j < i; j++)
      if (sp_r64(b + 32 + j * 64) == sp_r64(b + off))
        return 0;
  }
  sp_zero(c->players, (SPU32)sizeof(c->players));
  for (i = 0; i < n; i++) {
    SPPet *p = &c->players[i];
    off = 32 + i * 64;
    p->used = 1;
    p->id.lo = sp_r32(b + off);
    p->id.hi = sp_r32(b + off + 4);
    p->balance = sp_r32(b + off + 8);
    p->owned = sp_r32(b + off + 12);
    p->variant = sp_r32(b + off + 16);
    p->last_kind = sp_r32(b + off + 20);
    p->last_station = sp_r32(b + off + 24);
    p->last_code = sp_r32(b + off + 28);
    p->last_txn = sp_r64(b + off + 32);
    p->last_balance = sp_r32(b + off + 40);
    p->last_variant = sp_r32(b + off + 44);
    p->blink_time = 2.5f;
  }
  return 1;
}
#endif
#endif
