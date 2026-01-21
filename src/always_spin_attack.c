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

s32 healthCost = 16; // 1 heart
s32 LensCost = 4; // Quarter Heart

s32 Player_UpperAction_7(Player* this, PlayState* play);

static bool FakeMagicActive = false;

RECOMP_HOOK ("Magic_DrawMeter") void FakeMeter(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 magicBarY;

        if ((gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) && (gSaveContext.save.saveInfo.playerData.magic = 1)) {
        FakeMagicActive = true;
        gSaveContext.save.saveInfo.playerData.magic = 0;
    }
}

RECOMP_HOOK_RETURN ("Magic_DrawMeter") void FakeMeterReturn(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 magicBarY;

        if ((gSaveContext.save.saveInfo.playerData.magic == 0) && (FakeMagicActive == true)) {
        FakeMagicActive = false;
        gSaveContext.save.saveInfo.playerData.magic = 1;
    }
}


RECOMP_HOOK ("Magic_Update") s32 LifeMagicHook(PlayState* play, s16 magicToConsume, s16 type) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 tempTimer;

    if ((gSaveContext.magicState == MAGIC_STATE_CONSUME_GORON_ZORA_SETUP) && 
    (gSaveContext.save.saveInfo.playerData.magic == 1) && 
    (!CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI))) {

        Health_ChangeBy(play, -LensCost);
        gSaveContext.save.saveInfo.playerData.magic = 1;
        gSaveContext.magicState = MAGIC_STATE_CONSUME_GORON_ZORA;
    }
    
    switch (gSaveContext.magicState) {
        case MAGIC_STATE_CONSUME_LENS:
            tempTimer = interfaceCtx->magicConsumptionTimer - 1;
            if (tempTimer == 0) {
                if ((!CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI) && (gSaveContext.save.saveInfo.playerData.magic == 1))) {
                    Health_ChangeBy(play, -LensCost);
                    gSaveContext.save.saveInfo.playerData.magic = 1;
                }
                interfaceCtx->magicConsumptionTimer = 80;
            }
            break;
        case MAGIC_STATE_CONSUME_GORON_ZORA:
            tempTimer = interfaceCtx->magicConsumptionTimer - 1;
            if (tempTimer == 0) {
                        if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI) && (gSaveContext.save.saveInfo.playerData.magic == 1)) {
                            Health_ChangeBy(play, -LensCost);
                            gSaveContext.save.saveInfo.playerData.magic = 1;
                        }
                        interfaceCtx->magicConsumptionTimer = 10;

                        }
                        break; // add the other cases where it decrements (goron / zora / giants i think)*******
                    }
                }


RECOMP_HOOK ("func_808318C0") void LensStuff(PlayState* play, s16 magicToConsume, s16 type) {
// only runs the first time lens is used / activated on 0 MP
    if (Magic_Consume(play, 0, MAGIC_CONSUME_LENS)) {

    } else {
        Health_ChangeBy(play, -LensCost);
        gSaveContext.save.saveInfo.playerData.magic = 1;
    }
}
// No crash but still vanilla behaviour somehow, ive double checked the mod is on and everything
// RECOMP_HOOK ("Magic_Reset") s32 LensReset(PlayState* play) {
//     InterfaceContext* interfaceCtx = &play->interfaceCtx;

//     if (Magic_Consume(play, 0, MAGIC_CONSUME_LENS)) {
//         interfaceCtx->magicConsumptionTimer = 80;
//         if (interfaceCtx->magicConsumptionTimer <= 1) {
//            Health_ChangeBy(play, -LensCost);
//            interfaceCtx->magicConsumptionTimer = 80;
//         }
//         // return false; // where's the rest of it lol      p sure all of that is my own code that i wanted running at the start of that func
//     }
// }


// Arrow Stuff
ArrowType sArrowType;
ArrowMagic sMagicArrowType;
Player* gPlayer;
PlayState* gPlay;
bool idkman;

RECOMP_HOOK("func_808306F8")
void BeforeArrowStuff(Player* this, PlayState* play) {
    // Reset arrow types and save global variables
    sArrowType = ARROW_TYPE_NORMAL;
    sMagicArrowType = ARROW_MAGIC_INVALID;
    gPlayer = this;
    gPlay = play;
    idkman = false;

    // Follow the flow of the original function until we get to our desired point
    if ((this->heldItemAction >= PLAYER_IA_BOW_FIRE) && (this->heldItemAction <= PLAYER_IA_BOW_LIGHT) &&
        (gSaveContext.magicState != MAGIC_STATE_IDLE)) {
        return;
    }

    if (this->unk_B28 >= 0) {
        ItemId item;

        if (!Player_IsHoldingHookshot(this) && (func_808305BC(play, this, &item, &sArrowType) > 0)) {
            if (this->unk_B28 >= 0) {
                // Store the real arrow types
                sMagicArrowType = ARROW_GET_MAGIC_FROM_TYPE(sArrowType);
                s32 magicCost = sMagicArrowCosts[sMagicArrowType];

                if ((ARROW_GET_MAGIC_FROM_TYPE(sArrowType) >= ARROW_MAGIC_FIRE) &&
                    (ARROW_GET_MAGIC_FROM_TYPE(sArrowType) <= ARROW_MAGIC_LIGHT)) {
                    if ((gSaveContext.save.saveInfo.playerData.magic < magicCost)) {
                        idkman = true;
                        // gSaveContext.save.saveInfo.playerData.health -= healthCost; // don't do this
                        Health_ChangeBy(play, -healthCost);
                    }

                } else if ((sArrowType == ARROW_TYPE_DEKU_BUBBLE) &&
                            (!CHECK_WEEKEVENTREG(WEEKEVENTREG_08_01) || (play->sceneId != SCENE_BOWLING))) {
                    sMagicArrowType = ARROW_MAGIC_DEKU_BUBBLE;
                } else {
                    sMagicArrowType = ARROW_MAGIC_INVALID;
                }
            }
        }
    }
}

RECOMP_HOOK_RETURN("func_808306F8")
void AfterArrowStuff() {
    Player* this = gPlayer;
    PlayState* play = gPlay;

    // gets the true/false returned from the original function
    s32 rVal = recomphook_get_return_s32();

    // properly consume magic and spawn the correct arrow
    if (idkman && (sMagicArrowType > ARROW_MAGIC_INVALID)) {
        Actor_Kill(this->heldActor); // unsure if this part is needed
        Magic_Consume(play, sMagicArrowCosts[sMagicArrowType], MAGIC_CONSUME_NOW);
        this->heldActor = Actor_SpawnAsChild(
            &play->actorCtx, &this->actor, play, ACTOR_EN_ARROW, this->actor.world.pos.x,
            this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0, sArrowType);
    }
}