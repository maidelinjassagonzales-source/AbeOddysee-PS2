#include "stdafx_ao.h"
#include "Ps2SmoothAO.hpp"

#ifdef __PS2__

    #include "BaseAnimatedWithPhysicsGameObject.hpp"
    #include "Game.hpp"
    #include "Map.hpp"
    #include "PsxDisplay.hpp"
    #include "CameraSwapper.hpp"

namespace AO {

namespace {

struct Snapshot final
{
    BaseAnimatedWithPhysicsGameObject* mObj;
    FP mPrevX;
    FP mPrevY;
    FP mCurX;
    FP mCurY;
};

constexpr s32 kMaxObjects = 512;
// Anything that moved further than this in one step teleported (respawn, door, camera change):
// drawing it halfway would show it somewhere it never was.
constexpr s32 kMaxStepPixels = 48;

Snapshot sSnapshots[kMaxObjects];
s32 sCount = 0;
bool sValid = false;
LevelIds sLevel = LevelIds::eNone;
s16 sPath = 0;
s16 sCamera = 0;

bool IsStillDrawable(BaseGameObject* pObj)
{
    for (s32 i = 0; i < gObjList_drawables_504618->Size(); i++)
    {
        if (gObjList_drawables_504618->ItemAt(i) == pObj)
        {
            return true;
        }
    }
    return false;
}

s32 AbsPixels(FP value)
{
    const s32 v = FP_GetExponent(value);
    return v < 0 ? -v : v;
}

} // namespace

void Ps2Smooth_SnapshotPositions()
{
    sCount = 0;
    sValid = false;
    if (!gObjList_drawables_504618)
    {
        return;
    }

    for (s32 i = 0; i < gObjList_drawables_504618->Size() && sCount < kMaxObjects; i++)
    {
        BaseGameObject* pObj = gObjList_drawables_504618->ItemAt(i);
        if (!pObj)
        {
            break;
        }
        if (pObj->field_6_flags.Get(BaseGameObject::eIsBaseAnimatedWithPhysicsObj_Bit5) && !pObj->field_6_flags.Get(BaseGameObject::eDead_Bit3))
        {
            auto* pAnimObj = static_cast<BaseAnimatedWithPhysicsGameObject*>(pObj);
            sSnapshots[sCount++] = {pAnimObj, pAnimObj->field_A8_xpos, pAnimObj->field_AC_ypos, {}, {}};
        }
    }

    sLevel = gMap_507BA8.field_0_current_level;
    sPath = gMap_507BA8.field_2_current_path;
    sCamera = gMap_507BA8.field_4_current_camera;
    sValid = true;
}

void Ps2Smooth_DrawIntermediateFrame(void (*renderFn)(PrimHeader** ppOt))
{
    if (!sValid || sCount == 0)
    {
        return;
    }
    // Screen changes and camera swap effects are not continuous motion.
    if (sNumCamSwappers_507668 != 0 || gMap_507BA8.field_0_current_level != sLevel || gMap_507BA8.field_2_current_path != sPath || gMap_507BA8.field_4_current_camera != sCamera)
    {
        return;
    }

    // Move everything to the midpoint of this step...
    for (s32 i = 0; i < sCount; i++)
    {
        Snapshot& snap = sSnapshots[i];
        if (!IsStillDrawable(snap.mObj) || snap.mObj->field_6_flags.Get(BaseGameObject::eDead_Bit3))
        {
            snap.mObj = nullptr;
            continue;
        }
        snap.mCurX = snap.mObj->field_A8_xpos;
        snap.mCurY = snap.mObj->field_AC_ypos;
        if (AbsPixels(snap.mCurX - snap.mPrevX) > kMaxStepPixels || AbsPixels(snap.mCurY - snap.mPrevY) > kMaxStepPixels)
        {
            snap.mObj = nullptr;
            continue;
        }
        FP midX;
        FP midY;
        midX.fpValue = snap.mPrevX.fpValue + (snap.mCurX.fpValue - snap.mPrevX.fpValue) / 2;
        midY.fpValue = snap.mPrevY.fpValue + (snap.mCurY.fpValue - snap.mPrevY.fpValue) / 2;
        snap.mObj->field_A8_xpos = midX;
        snap.mObj->field_AC_ypos = midY;
    }

    // ...draw and show that frame...
    renderFn(gPsxDisplay_504C78.field_C_drawEnv[gPsxDisplay_504C78.field_A_buffer_index].field_70_ot_buffer);
    gPsxDisplay_504C78.PS2_Render_OT_Intermediate();

    // ...and put everything back for the real frame.
    for (s32 i = 0; i < sCount; i++)
    {
        Snapshot& snap = sSnapshots[i];
        if (snap.mObj)
        {
            snap.mObj->field_A8_xpos = snap.mCurX;
            snap.mObj->field_AC_ypos = snap.mCurY;
        }
    }
}

} // namespace AO

#else

namespace AO {

void Ps2Smooth_SnapshotPositions()
{
}

void Ps2Smooth_DrawIntermediateFrame(void (*)(PrimHeader**))
{
}

} // namespace AO

#endif
