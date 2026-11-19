#include "stdafx_ao.h"
#include "Ps2CoopAO.hpp"

#ifdef __PS2__

    #include "Abe.hpp"
    #include "Game.hpp"
    #include "Input.hpp"
    #include "Map.hpp"
    #include "PathData.hpp"
    #include "stdlib.hpp"
    #include "../AliveLibAE/Input.hpp"
    #include "../AliveLibAE/Ps2Coop.hpp"
    #include <SDL3/SDL.h>

// Same approach as the Exoddus co-op (AliveLibAE/Ps2Coop.cpp): the engine is written around one
// hero, and almost all of Abe's logic reads the global hero (sActiveHero), the global controlled
// character and the global pad index. Player 2 is a normal Abe whose update runs with those three
// globals pointing at player 2 and pad 2, then put back. The camera stays with player 1 (Map refuses
// camera changes requested by player 2), and player 2 is brought back next to player 1 when off
// screen or dead. The on-screen hints are drawn by the shared overlay in AliveLibAE/Ps2Coop.cpp.

namespace AO {

ALIVE_VAR_EXTERN(u16, sCurrentControllerIndex_5076B8); // Input.cpp

namespace {

Abe* sPlayer2 = nullptr;
// What player 2 is steering: their Abe, or the creature they possessed by chanting.
BaseAliveGameObject* sPlayer2Controlled = nullptr;
const BaseGameObject* sDyingPlayer2 = nullptr; // marked dead, destructor not run yet
bool sJoined = false;
s32 sRespawnDelay = 0;
s32 sOffscreenFrames = 0;
s32 sStartHeldFrames = 0;

constexpr s32 kRespawnFrames = 60;   // ~2 s at 30 fps
constexpr s32 kOffscreenFrames = 15; // grace before pulling player 2 back
constexpr s32 kLeaveHoldFrames = 60; // hold START ~2 s to leave

bool IsAliveObject(const BaseGameObject* pObj)
{
    for (s32 i = 0; i < gBaseGameObject_list_9F2DF0->Size(); i++)
    {
        BaseGameObject* pItem = gBaseGameObject_list_9F2DF0->ItemAt(i);
        if (!pItem)
        {
            break;
        }
        if (pItem == pObj)
        {
            return !pItem->field_6_flags.Get(BaseGameObject::eDead_Bit3);
        }
    }
    return false;
}

void RemovePlayer2(s32 respawnDelay)
{
    if (sPlayer2)
    {
        sPlayer2->field_6_flags.Set(BaseGameObject::eDead_Bit3);
        sDyingPlayer2 = sPlayer2;
    }
    sPlayer2 = nullptr;
    sPlayer2Controlled = nullptr;
    sRespawnDelay = respawnDelay;
}

bool Player1OnGround()
{
    Abe* pHero = sActiveHero_507678;
    return pHero && pHero->field_100_health > FP_FromInteger(0) && pHero->field_F4_pLine;
}

void PlaceNextToPlayer1(Abe* pAbe)
{
    Abe* pHero = sActiveHero_507678;
    const FP offset = pHero->field_10_anim.field_4_flags.Get(AnimFlags::eBit5_FlipX) ? FP_FromInteger(18) : FP_FromInteger(-18);

    pAbe->field_A8_xpos = pHero->field_A8_xpos + offset;
    pAbe->field_AC_ypos = pHero->field_AC_ypos;
    pAbe->field_E8_LastLineYPos = pHero->field_AC_ypos;
    pAbe->field_B0_path_number = pHero->field_B0_path_number;
    pAbe->field_B2_lvl_number = pHero->field_B2_lvl_number;
    pAbe->field_F4_pLine = pHero->field_F4_pLine;
    pAbe->field_BC_sprite_scale = pHero->field_BC_sprite_scale;
    pAbe->field_C6_scale = pHero->field_C6_scale;
    pAbe->field_10_anim.field_C_layer = pHero->field_10_anim.field_C_layer;
    pAbe->field_B4_velx = FP_FromInteger(0);
    pAbe->field_B8_vely = FP_FromInteger(0);
    pAbe->field_FC_current_motion = eAbeMotions::Motion_0_Idle_423520;
    pAbe->field_FE_next_motion = eAbeMotions::Motion_0_Idle_423520;
}

void SpawnPlayer2()
{
    BaseAliveGameObject* const pControlled = sControlledCharacter_50767C;

    sPlayer2 = ao_new<Abe>();
    if (!sPlayer2)
    {
        return;
    }
    sPlayer2->ctor_420770(55888, 85, 57, 55); // same as the hero made by the start TLV (Factory.cpp)

    // Abe's constructor makes itself the controlled character; that stays player 1's.
    sControlledCharacter_50767C = pControlled;

    PlaceNextToPlayer1(sPlayer2);
    sPlayer2Controlled = sPlayer2;
    sOffscreenFrames = 0;
    LOG_INFO("Co-op: player 2 spawned");
}

} // namespace

void Ps2Coop_Frame()
{
    const bool inLevel = gMap_507BA8.field_0_current_level != LevelIds::eMenu_0 && gMap_507BA8.field_0_current_level != LevelIds::eNone;
    ::Ps2Coop_SetOverlayState(inLevel, sJoined);
    if (!inLevel)
    {
        return;
    }

    // Pad 2 as the engine reads it (AE input bits): START is ePause.
    const auto& pad2 = ::Input().field_0_pads[1];
    if (!sJoined)
    {
        if (pad2.field_C_held & ::InputCommands::Enum::ePause)
        {
            sJoined = true;
            sRespawnDelay = 0;
            ::Ps2Coop_ShowNotice(true);
            LOG_INFO("Co-op: player 2 joined");
        }
        return;
    }

    // Hold START to leave the game (only while steering their own Abe).
    sStartHeldFrames = (pad2.field_0_pressed & ::InputCommands::Enum::ePause) ? sStartHeldFrames + 1 : 0;
    if (sStartHeldFrames >= kLeaveHoldFrames && (!sPlayer2 || sPlayer2Controlled == sPlayer2))
    {
        RemovePlayer2(0);
        sJoined = false;
        sStartHeldFrames = 0;
        ::Ps2Coop_ShowNotice(false);
        LOG_INFO("Co-op: player 2 left");
        return;
    }

    // The possessed creature died or vanished: control goes back to player 2's Abe.
    if (sPlayer2 && sPlayer2Controlled != sPlayer2 && (!sPlayer2Controlled || !IsAliveObject(sPlayer2Controlled)))
    {
        sPlayer2Controlled = sPlayer2;
    }

    if (!sPlayer2)
    {
        if (sRespawnDelay > 0)
        {
            sRespawnDelay--;
        }
        else if (Player1OnGround())
        {
            SpawnPlayer2();
        }
        return;
    }

    #ifdef RELIVE_PS2_HOSTFS
    static u32 sLogTick = 0;
    if (++sLogTick % 60 == 0)
    {
        LOG_INFO("Co-op P2: pos " << FP_GetExponent(sPlayer2->field_A8_xpos) << "," << FP_GetExponent(sPlayer2->field_AC_ypos)
                 << " motion " << sPlayer2->field_FC_current_motion << " health " << FP_GetDouble(sPlayer2->field_100_health)
                 << " controls type " << static_cast<s32>(sPlayer2Controlled ? sPlayer2Controlled->field_4_typeId : Types::eNone_0)
                 << " pad2 0x" << std::hex << pad2.field_0_pressed << std::dec);
    }
    #endif

    Abe* const pHero = sActiveHero_507678;
    if (!pHero)
    {
        return;
    }

    // Keep player 2 on player 1's screen: the camera never follows player 2.
    const bool onScreen = sPlayer2->field_B2_lvl_number == gMap_507BA8.field_0_current_level
                       && sPlayer2->field_B0_path_number == gMap_507BA8.field_2_current_path
                       && gMap_507BA8.Is_Point_In_Current_Camera_4449C0(sPlayer2->field_B2_lvl_number, sPlayer2->field_B0_path_number, sPlayer2->field_A8_xpos, sPlayer2->field_AC_ypos, 0);
    // Already right next to player 1 (e.g. placed just past the screen edge): leave it alone, or
    // every pull-back resets the motion and a chant could never finish.
    const s32 dx = FP_GetExponent(sPlayer2->field_A8_xpos) - FP_GetExponent(pHero->field_A8_xpos);
    const bool nextToPlayer1 = sPlayer2->field_B0_path_number == pHero->field_B0_path_number && dx > -40 && dx < 40;

    // While possessing, player 2's Abe stands chanting; don't pull it around.
    if (onScreen || nextToPlayer1 || sPlayer2Controlled != sPlayer2)
    {
        sOffscreenFrames = 0;
    }
    else if (++sOffscreenFrames > kOffscreenFrames && Player1OnGround() && sPlayer2->field_100_health > FP_FromInteger(0))
    {
        PlaceNextToPlayer1(sPlayer2);
        sOffscreenFrames = 0;
    }
}

bool Ps2Coop_IsActive()
{
    return sJoined;
}

bool Ps2Coop_IsPlayer2(const BaseGameObject* pObj)
{
    return pObj && (pObj == sPlayer2 || pObj == sPlayer2Controlled);
}

void Ps2Coop_UpdatePlayer2(BaseGameObject* pObj)
{
    Abe* const pHero = sActiveHero_507678;
    BaseAliveGameObject* const pControlled = sControlledCharacter_50767C;
    const u16 padIndex = sCurrentControllerIndex_5076B8;

    // Player 2's Abe and whatever player 2 possessed both run with player 2's view of the world:
    // hero = player 2's Abe, controlled character = player 2's, pad = 2. Possessing sets the slot
    // to the creature, releasing sets it back to "the hero", i.e. player 2's Abe.
    sActiveHero_507678 = sPlayer2;
    sControlledCharacter_50767C = sPlayer2Controlled;
    sCurrentControllerIndex_5076B8 = 1;

    pObj->VUpdate();

    if (sPlayer2)
    {
        if (sControlledCharacter_50767C != sPlayer2Controlled)
        {
            LOG_INFO("Co-op: player 2 now controls type " << static_cast<s32>(sControlledCharacter_50767C ? sControlledCharacter_50767C->field_4_typeId : Types::eNone_0));
        }
        sPlayer2Controlled = sControlledCharacter_50767C ? sControlledCharacter_50767C : sPlayer2;
    }

    sActiveHero_507678 = pHero;
    sControlledCharacter_50767C = pControlled;
    sCurrentControllerIndex_5076B8 = padIndex;
}

bool Ps2Coop_OnAbeDeathReset(const BaseGameObject* pAbe)
{
    if (!pAbe || pAbe != sPlayer2)
    {
        return false;
    }
    // Player 2 died: remove them and bring them back next to player 1 shortly, no level reset.
    RemovePlayer2(kRespawnFrames);
    LOG_INFO("Co-op: player 2 died, respawning");
    return true;
}

bool Ps2Coop_OnAbeDestroyed(const BaseGameObject* pAbe)
{
    if (pAbe && pAbe == sDyingPlayer2)
    {
        sDyingPlayer2 = nullptr;
        return true;
    }
    if (pAbe && pAbe == sPlayer2)
    {
        sPlayer2 = nullptr;
        sPlayer2Controlled = nullptr;
        sRespawnDelay = kRespawnFrames;
        return true;
    }
    return false;
}

void Ps2Coop_TintPlayer2(const BaseGameObject* pObj, s16* r, s16* g, s16* b)
{
    if (Ps2Coop_IsPlayer2(pObj))
    {
        // Cool blue Abe: clearly different from player 1 even inside shadow zones.
        *r = static_cast<s16>(*r * 2 / 5);
        *g = static_cast<s16>(*g * 3 / 5);
        const s32 blue = *b * 8 / 5 + 20;
        *b = static_cast<s16>(blue > 255 ? 255 : blue);
    }
}

} // namespace AO

#else

namespace AO {

void Ps2Coop_Frame()
{
}

bool Ps2Coop_IsActive()
{
    return false;
}

bool Ps2Coop_IsPlayer2(const BaseGameObject*)
{
    return false;
}

void Ps2Coop_UpdatePlayer2(BaseGameObject*)
{
}

bool Ps2Coop_OnAbeDeathReset(const BaseGameObject*)
{
    return false;
}

bool Ps2Coop_OnAbeDestroyed(const BaseGameObject*)
{
    return false;
}

void Ps2Coop_TintPlayer2(const BaseGameObject*, s16*, s16*, s16*)
{
}

} // namespace AO

#endif
