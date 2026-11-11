#ifndef MOON_PICKUP_HANDS_H
#define MOON_PICKUP_HANDS_H
#define MH_ABI 1u
typedef struct {
    unsigned abi,size;
    /* Unity thread only. A stable resident provider key owns one claim.
     * Claim after pickup clearance succeeds, before moving the object.
     * Release after a successful drop/throw, and during provider teardown. */
    int (*claim)(const char *owner,void *walker);
    int (*release)(const char *owner,void *walker);
    int (*occupied)(void *walker);
} MoonHands;
typedef const MoonHands *(*MoonHandsGet)(unsigned,unsigned);
#endif
