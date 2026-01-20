#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "z64player.h"
#include "z64interface.h"
#include "z_en_arrow.h"

ArrowType arrowType;
ArrowMagic magicArrowType;

s32 func_808305BC(PlayState* play, Player* this, ItemId* item, ArrowType* typeParam);

void Player_SetUpperAction(PlayState* play, Player* this, PlayerUpperActionFunc upperActionFunc);

u16 D_8085CFB0[] = {
    NA_SE_PL_BOW_DRAW,
    NA_SE_NONE,
    NA_SE_IT_HOOKSHOT_READY,
};

u8 sMagicArrowCosts[] = {
    4, // ARROW_MAGIC_FIRE
    4, // ARROW_MAGIC_ICE
    8, // ARROW_MAGIC_LIGHT
    2, // ARROW_MAGIC_DEKU_BUBBLE
};

u8 sMagicArrowLifeCosts [] = {
    0x10, // ARROW_MAGIC_FIRE
    0x10, // ARROW_MAGIC_ICE
    0x20, // ARROW_MAGIC_LIGHT
    0x08, // ARROW_MAGIC_DEKU_BUBBLE
};

s32 Player_UpperAction_7(Player* this, PlayState* play);



// // Draw bow or hookshot / first person items?

// RECOMP_HOOK ("func_808306F8") s32 MagicArrowLife(Player* this, PlayState* play) {

//     ArrowType arrowType;
//     ArrowMagic magicArrowType;

// s32 magicCost = sMagicArrowCosts[magicArrowType];
// s32 healthCost = 16; // 1 heart

//     if ((gSaveContext.save.saveInfo.playerData.magic < magicCost) && (arrowType > 2) && (arrowType < 8) && (arrowType != 6)) {

//         // Not enough magic → pay with health instead
//         gSaveContext.save.saveInfo.playerData.health -= healthCost;
//         arrowType = arrowType;
//         magicArrowType = magicArrowType;
//             }
//        }

// RECOMP_HOOK_RETURN ("func_808306F8") s32 MagicArrowLifeReturn(Player* this, PlayState* play) {

//     ArrowType arrowType;
//     ArrowMagic magicArrowType;

// s32 magicCost = sMagicArrowCosts[magicArrowType];
// s32 healthCost = 16; // 1 heart

//     if ((gSaveContext.save.saveInfo.playerData.magic < magicCost) && (arrowType > 2) && (arrowType < 8) && (arrowType != 6)) {

//         // Not enough magic → pay with health instead
//         gSaveContext.save.saveInfo.playerData.health -= healthCost;
//         arrowType = arrowType;
//         magicArrowType = magicArrowType;
//             }
//        }

RECOMP_PATCH s32 func_808306F8(Player* this, PlayState* play) {

    s32 magicCost = sMagicArrowCosts[magicArrowType];
    s32 healthCost = 16; // 1 heart

    if ((this->heldItemAction >= PLAYER_IA_BOW_FIRE) && (this->heldItemAction <= PLAYER_IA_BOW_LIGHT) &&
        (gSaveContext.magicState != MAGIC_STATE_IDLE)) {
        Audio_PlaySfx(NA_SE_SY_ERROR);
    } else {
        Player_SetUpperAction(play, this, Player_UpperAction_7);

        this->stateFlags3 |= PLAYER_STATE3_40;
        this->unk_ACC = 14;

        if (this->unk_B28 >= 0) {
            s32 var_v1 = ABS_ALT(this->unk_B28);
            ItemId item;
            ArrowType arrowType;
            ArrowMagic magicArrowType;

            if (var_v1 != 2) {
                Player_PlaySfx(this, D_8085CFB0[var_v1 - 1]);
            }

            if (!Player_IsHoldingHookshot(this) && (func_808305BC(play, this, &item, &arrowType) > 0)) {
                if (this->unk_B28 >= 0) {
                    magicArrowType = ARROW_GET_MAGIC_FROM_TYPE(arrowType);

                    if ((ARROW_GET_MAGIC_FROM_TYPE(arrowType) >= ARROW_MAGIC_FIRE) &&
                        (ARROW_GET_MAGIC_FROM_TYPE(arrowType) <= ARROW_MAGIC_LIGHT)) {
                        if ((gSaveContext.save.saveInfo.playerData.magic < magicCost) && (arrowType > 2) && (arrowType < 8) && (arrowType != 6)) {
                            gSaveContext.save.saveInfo.playerData.health -= healthCost;
                        }
                    } else if ((arrowType == ARROW_TYPE_DEKU_BUBBLE) &&
                               (!CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01) || (play->sceneId != SCENE_BOWLING))) {
                        magicArrowType = ARROW_MAGIC_DEKU_BUBBLE;
                    } else {
                        magicArrowType = ARROW_MAGIC_INVALID;
                    }

                    this->heldActor = Actor_SpawnAsChild(
                        &play->actorCtx, &this->actor, play, ACTOR_EN_ARROW, this->actor.world.pos.x,
                        this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, arrowType);

                    if ((this->heldActor != NULL) && (magicArrowType > ARROW_MAGIC_INVALID)) {
                        Magic_Consume(play, sMagicArrowCosts[magicArrowType], MAGIC_CONSUME_NOW);
                    }
                }
            }
        }

        return true;
    }

    return false;
}