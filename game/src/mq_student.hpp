// Where the local student stands.
//
// The field's local player object, *0x020AD7EC + 0x383C, holds the student's place at +0x2C/+0x30/+0x34 (x, y, z in
// 20.12 fixed point; x east, z south): what the talk finder (ov001 0x020C85E4) measures from, what the game's own
// movement packets carry, and the frame the event scripts' places are in (warp_map 0x2D, actor_set_pos 0x02).
// The triple at 0x020B8B7C, long taken for the student's position, is the bottom camera's point (field+0x168, camera
// 1's +4): south of the student and up, by a different amount in each map and place (the town 3.4 south and 8.7 up at
// the school's front and the dorm's square, about 7 south at the plaza; the beach 11.85, Lovers Point 12.23, the dorm
// 8.22), and it keeps the old place for a while after a map change.
#pragma once

#include <cstdint>

#include "ntr/recomp.h"

namespace mq {

constexpr uint32_t kFieldPtr = 0x020AD7EC;     // the field object
constexpr uint32_t kLocalPlayer = 0x383C;      // its local player object
constexpr uint32_t kLocalPlayerPos = 0x2C;     // that object's x, y, z
constexpr uint32_t kCameraPoint = 0x020B8B7C;  // the bottom camera's point (x, y, z 20.12)

struct StudentPos {
    int32_t x = 0, y = 0, z = 0;
    bool real = false;  // false: no student placed (no field object, or the object zeroed while a map loads)
};

// The student's place, from the field's local player object. While a map loads (a door, a warp) the object reads
// (0, 0, 0) for some 100-150 frames, while the field already counts as loaded (coop_gate's test), before the new map's arrival puts the
// student down (seen leaving the beach into town: (5, 14) -> (0, 0, 0) -> (65.75, 5.38)); that is not a place: `real`
// is false then, and with no field object at all (the title, before the field is made), when x, y, z are the camera's
// point. Callers skip a frame whose place is not real (no jump, no settling, no learning from it).
inline StudentPos student_pos(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kFieldPtr);
    if (field >= 0x02000000 && field < 0x023F0000) {
        const uint32_t p = field + kLocalPlayer + kLocalPlayerPos;
        const int32_t x = int32_t(ntr_rd32(c, p)), y = int32_t(ntr_rd32(c, p + 4)), z = int32_t(ntr_rd32(c, p + 8));
        return {x, y, z, (x | y | z) != 0};
    }
    return {int32_t(ntr_rd32(c, kCameraPoint)), int32_t(ntr_rd32(c, kCameraPoint + 4)),
            int32_t(ntr_rd32(c, kCameraPoint + 8)), false};
}

}  // namespace mq
