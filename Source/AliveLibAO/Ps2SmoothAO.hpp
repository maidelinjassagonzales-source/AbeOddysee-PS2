#pragma once

// 60 fps presentation for the PS2 port of Abe's Oddysee (same idea as AliveLibAE/Ps2Smooth): the
// game logic keeps its 30 steps per second, and between two steps an extra frame is drawn with
// every moving object halfway between its previous and its new position. No-ops elsewhere.

struct PrimHeader;

namespace AO {

// Records object positions; call right before the objects update for a game step.
void Ps2Smooth_SnapshotPositions();

// After the update: draws and shows the in-between frame, using renderFn to fill the ordering table.
void Ps2Smooth_DrawIntermediateFrame(void (*renderFn)(PrimHeader** ppOt));

} // namespace AO
