#pragma once

// Two-player co-op for the PS2 port of Abe's Oddysee, the same as in Exoddus (AliveLibAE/Ps2Coop):
// a second Abe, driven by the second DualShock, plays alongside the first one. Player 2 joins with
// START on pad 2 (hold START ~2 s to leave), can chant and possess creatures, follows player 1's
// camera, and respawns next to player 1 instead of resetting the level when they die.
// All functions are no-ops elsewhere.

namespace AO {

class BaseGameObject;

// Once per game-loop frame, before objects update: joining, (re)spawning, keeping P2 on screen.
void Ps2Coop_Frame();

// True once player 2 has joined.
bool Ps2Coop_IsActive();

// True for player 2's Abe and for the creature player 2 is possessing (both use pad 2).
bool Ps2Coop_IsPlayer2(const BaseGameObject* pObj);

// Updates player 2 with the engine believing it is the hero on pad 2 (see the .cpp).
void Ps2Coop_UpdatePlayer2(BaseGameObject* pObj);

// Called from Abe's death sequence; true when it was player 2 (who must not reset the level).
bool Ps2Coop_OnAbeDeathReset(const BaseGameObject* pAbe);

// Called from Abe's destructor. True when the Abe being destroyed is (or was) player 2: then the
// global hero must be kept.
bool Ps2Coop_OnAbeDestroyed(const BaseGameObject* pAbe);

// Render-time tint so the two Abes can be told apart.
void Ps2Coop_TintPlayer2(const BaseGameObject* pObj, s16* r, s16* g, s16* b);

} // namespace AO
