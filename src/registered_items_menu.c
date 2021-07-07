#include "global.h"
#include "constants/songs.h"
#include "bg.h"
#include "decoration.h"
#include "event_scripts.h"
#include "event_object_movement.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "international_string_util.h"
#include "item.h"
#include "item_icon.h"
#include "item_menu.h"
#include "item_menu_icons.h"
#include "constants/items.h"
#include "list_menu.h"
#include "mail.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "player_pc.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "window.h"
#include "menu_specialized.h"
#include "registered_items_menu.h"

struct TEST_Struct
{
    struct ListMenuItem unk0[51];
    u8 unk198[51][0x18];
    u8 windowIds[1];
    u8 unk666;
    u8 spriteId;
    u8 spriteIds[7];
};

static void TEST_ItemStorage_Withdraw(u8 taskId);
static void TEST_ItemStorage_ClearAndInitData(u8 taskId);
static void TEST_ItemStorage_ProcessWithdrawTossInput(u8 taskId);
static void TEST_ItemStorage_ProcessInput(u8 taskId);
static void TEST_ItemStorage_DoItemAction(u8 taskId);
static void TEST_ItemStorage_CloseMenu(u8 taskId);
static void TEST_ItemStorage_ItemSwapChoosePrompt(u8 taskId);
static void TEST_HandleSwapInput(u8 taskId);
//helper
static void TEST_CalcCursorPos(void);
static void TEST_CalculateUsedSlots(void);
static void TEST_AllocateStruct(void);
static u8 TEST_InitWindow(void);
static void TEST_ItemStorage_RefreshListMenu(void);
static void TEST_ItemStorage_MoveCursor(s32 id, bool8 b, struct ListMenu *thisMenu);
static void TEST_ItemStorage_PrintFunc(u8 windowId, s32 id, u8 yOffset);
static void TEST_PrintItemIcon(u16 itemId);
static void TEST_ItemStorage_DoItemSwap(u8 taskId, bool8 a);
static void TEST_ItemStorage_StartScrollIndicator(void);
static void TEST_UpdateSwapLinePos(u8 y);
static void TEST_CopyItemName(u8 *string, u16 itemId);
static void TEST_PrintSwappingCursor(u8 y, u8 b, u8 speed);
static void TEST_GetSwappingCursorPositionAndPrint(u8 a, u8 b, u8 speed);
static void TEST_MoveItemSlotInList(struct RegisteredItemSlot* registeredItemSlots_, u32 from, u32 to_);
static void TEST_CalcAndSetUsedSlotsCount(struct RegisteredItemSlot *slots, u8 count, u8 *arg2, u8 *usedSlotsCount, u8 maxUsedSlotsCount);
static void TEST_RemoveRegisteredItemIndex(u8 index);
//helper cleanup
static void TEST_RemoveItemIcon(void);
static void TEST_RemoveWinow(void);
static void TEST_ItemStorage_RemoveScrollIndicator(void);
static void TEST_FreeStructs(void);


static const struct WindowTemplate TEST_WindowTemplates[1] =
{
    {//item list window
        .bg = 0,
        .tilemapLeft = 1, //0
        .tilemapTop = 13,
        .width = 28, //30
        .height = 6, //7
        .paletteNum = 15,
        .baseBlock = 0x0001
    },
};

static const struct ListMenuTemplate gTEST_List = //item storage list
{
    .items = NULL,
    .moveCursorFunc = TEST_ItemStorage_MoveCursor,
    .itemPrintFunc = TEST_ItemStorage_PrintFunc,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = 0,
    .header_X = 0,
    .item_X = 48,
    .cursor_X = 40,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = FALSE,
    .itemVerticalPadding = 0,
    .scrollMultiple = FALSE,
    .fontId = 7
};


//COMPATIBILITY with new pokeemerald versions
#define TASK_NONE 0xFF
#define SPRITE_NONE 0xFF
#define WINDOW_NONE 0xFF
void LoadListMenuSwapLineGfx(void)
{
    LoadListMenuArrowsGfx();
}
void CreateSwapLineSprites(u8 *spriteIds, u8 count)
{
    sub_8122344(spriteIds, count);
}
void DestroySwapLineSprites(u8 *spriteIds, u8 count)
{
    sub_81223B0(spriteIds, count);
}
void SetSwapLineSpritesInvisibility(u8 *spriteIds, u8 count, bool8 invisible)
{
    sub_81223FC(spriteIds, count, invisible);
}
void UpdateSwapLineSpritesPos(u8 *spriteIds, u8 count, s16 x, u16 y)
{
    sub_8122448(spriteIds, count, x, y);
}

// EWRAM
static EWRAM_DATA struct TEST_Struct *gTest = NULL;
static EWRAM_DATA struct TEST_ItemPageStruct TESTItemPageInfo = {0, 0, 0, 0, {0, 0, 0}, 0};




void TEST_PlayerPC(void)
{
    u8 taskId = CreateTask(TaskDummy, 0);
    gTasks[taskId].func = TEST_ItemStorage_Withdraw;
}

static void TEST_ItemStorage_Withdraw(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    NUM_ITEMS = TEST_CountUsedRegisteredItemSlots();
    TEST_ItemStorage_ClearAndInitData(taskId);
}

static void TEST_ItemStorage_ClearAndInitData(u8 taskId)
{
    u16 *data = gTasks[taskId].data;
    u8 offset = 0;
    u8 cursorStart = gSaveBlock1Ptr->registeredItemLastSelected;
    u8 count = TEST_CountUsedRegisteredItemSlots();
    //calculate offset from list top
    if (cursorStart > 1 && count > 3)
    {
        if (cursorStart == count - 1)
        {
            offset = cursorStart - 2;
            cursorStart = 2;
        }
        else
        {
            offset = cursorStart - 1;
            cursorStart = 1;
        }
    }

    // sub_816B4DC(taskId); //clear old windows
    TESTItemPageInfo.cursorPos = cursorStart;
    TESTItemPageInfo.itemsAbove = offset;
    TESTItemPageInfo.scrollIndicatorTaskId = TASK_NONE;
    TESTItemPageInfo.pageItems = 3; //ItemStorage_SetItemAndMailCount(taskId);
    TEST_AllocateStruct(); //sub_816BC14(); //allocate struct
    FreeAndReserveObjectSpritePalettes();
    LoadListMenuSwapLineGfx();
    CreateSwapLineSprites(gTest->spriteIds, 7);
    // ClearDialogWindowAndFrame(0,0);
    gTasks[taskId].func = TEST_ItemStorage_ProcessWithdrawTossInput;
}

static void TEST_ItemStorage_ProcessWithdrawTossInput(u8 taskId)
{
    s16 *data;
    u32 i, x;
    const u8* text;

    data = gTasks[taskId].data;
    // text = gText_WithdrawItem;
    // x = GetStringCenterAlignXOffset(1, text, 104);
    // AddTextPrinterParameterized(gUnknown_0203BCC4->windowIds[3], 1, text, x, 1, 0, NULL);
    // CopyWindowToVram(gTest->windowIds[0], 2);
    TEST_CalculateUsedSlots(); //calculate used slots
    TEST_CalcCursorPos(); //calc cursor pos
    TEST_ItemStorage_RefreshListMenu();
    data[5] = ListMenuInit(&gMultiuseListMenuTemplate, TESTItemPageInfo.itemsAbove, TESTItemPageInfo.cursorPos);
    TEST_ItemStorage_StartScrollIndicator();
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = TEST_ItemStorage_ProcessInput;
}

static void TEST_ItemStorage_ProcessInput(u8 taskId)
{
    s16 *data;
    s32 id;

    data = gTasks[taskId].data;
    if (JOY_NEW(SELECT_BUTTON))
    {
        ListMenuGetScrollAndRow(data[5], &(TESTItemPageInfo.itemsAbove), &(TESTItemPageInfo.cursorPos)); //fine
        if ((TESTItemPageInfo.itemsAbove + TESTItemPageInfo.cursorPos) != (TESTItemPageInfo.count - 1))
        {
            PlaySE(SE_SELECT);
            TEST_ItemStorage_ItemSwapChoosePrompt(taskId);
        }
    }
    else
    {
        id = ListMenu_ProcessInput(data[5]); //fine
        ListMenuGetScrollAndRow(data[5], &(TESTItemPageInfo.itemsAbove), &(TESTItemPageInfo.cursorPos)); //fine
        switch(id)
        {
        case LIST_NOTHING_CHOSEN:
            break;
        case LIST_CANCEL:
            PlaySE(SE_SELECT);
            TEST_ItemStorage_CloseMenu(taskId);
            break;
        default:
            PlaySE(SE_SELECT);
            TEST_ItemStorage_DoItemAction(taskId);
            break;
        }
    }
}

static void TEST_ItemStorage_DoItemAction(u8 taskId)
{
    s16 *data;
    u16 pos;

    data = gTasks[taskId].data;
    pos = (TESTItemPageInfo.cursorPos + TESTItemPageInfo.itemsAbove);
    TEST_ItemStorage_RemoveScrollIndicator();

    gSaveBlock1Ptr->registeredItemLastSelected = pos;
    UseRegisteredKeyItemOnField(pos+2);
    TEST_ItemStorage_CloseMenu(taskId);

    /*
    data[2] = 1;
    if (!data[3])
    {
        // if (gSaveBlock1Ptr->pcItems[b].quantity == 1)
        // {
        //     ItemStorage_DoItemWithdraw(taskId);
        //     return;
        // }
        // CopyItemName(gSaveBlock1Ptr->pcItems[b].itemId, gStringVar1);
        // ItemStorage_PrintItemPcResponse(ItemStorage_GetItemPcResponse(ITEMPC_HOW_MANY_TO_WITHDRAW));
    }
    else
    {
        // if (gSaveBlock1Ptr->pcItems[b].quantity == 1)
        // {
        //     ItemStorage_DoItemToss(taskId);
        //     return;
        // }
        // CopyItemName(gSaveBlock1Ptr->pcItems[b].itemId, gStringVar1);
        // ItemStorage_PrintItemPcResponse(ItemStorage_GetItemPcResponse(ITEMPC_HOW_MANY_TO_TOSS));
    }
    // sub_816C6BC(sub_816BC7C(4), data[2], STR_CONV_MODE_LEADING_ZEROS, 8, 1, 3);
    // gTasks[taskId].func = ItemStorage_HandleQuantityRolling;
    */
}

static void TEST_ItemStorage_CloseMenu(u8 taskId) //TEST_ItemStorage_GoBackToPlayerPCMenu
{
    s16 *data;

    data = gTasks[taskId].data;
    TEST_RemoveItemIcon();
    TEST_ItemStorage_RemoveScrollIndicator();
    DestroyListMenuTask(data[5], NULL, NULL);
    DestroySwapLineSprites(gTest->spriteIds, 7);
    TEST_RemoveWinow();
    TEST_FreeStructs();
    // gTasks[taskId].func = ItemStorage_GoBackToPlayerPCMenu_InitStorage;
    // SetMainCallback2(CB2_ReturnToField);
    EnableBothScriptContexts();
    DestroyTask(taskId);
}

static void TEST_ItemStorage_ItemSwapChoosePrompt(u8 taskId)
{
    s16 *data;

    data = gTasks[taskId].data;
    ListMenuSetUnkIndicatorsStructField(data[5], 16, 1);
    gTest->unk666 = (TESTItemPageInfo.itemsAbove + TESTItemPageInfo.cursorPos);
    TEST_GetSwappingCursorPositionAndPrint(data[5], 0, 0);
    TEST_UpdateSwapLinePos(gTest->unk666);
    // CopyItemName(gSaveBlock1Ptr->pcItems[gTest->unk666].itemId, gStringVar1);
    // ItemStorage_PrintItemPcResponse(ItemStorage_GetItemPcResponse(ITEMPC_SWITCH_WHICH_ITEM));
    gTasks[taskId].func = TEST_HandleSwapInput;
}

static void TEST_HandleSwapInput(u8 taskId)
{
    s16 *data;
    s32 id;

    data = gTasks[taskId].data;
    if (JOY_NEW(SELECT_BUTTON))
    {
        ListMenuGetScrollAndRow(data[5], &(TESTItemPageInfo.itemsAbove), &(TESTItemPageInfo.cursorPos));
        TEST_ItemStorage_DoItemSwap(taskId, FALSE);
        return;
    }
    id = ListMenu_ProcessInput(data[5]);
    ListMenuGetScrollAndRow(data[5], &(TESTItemPageInfo.itemsAbove), &(TESTItemPageInfo.cursorPos));
    SetSwapLineSpritesInvisibility(gTest->spriteIds, 7, FALSE); //fine
    TEST_UpdateSwapLinePos(TESTItemPageInfo.cursorPos);
    switch(id)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        if (JOY_NEW(A_BUTTON))
        {
            TEST_ItemStorage_DoItemSwap(taskId, FALSE);
        }
        else
        {
            TEST_ItemStorage_DoItemSwap(taskId, TRUE);
        }
        break;
    default:
        TEST_ItemStorage_DoItemSwap(taskId, FALSE);
        break;
    }
}



//helper functions
static void TEST_AllocateStruct(void)
{
    gTest = AllocZeroed(sizeof(struct TEST_Struct));
    memset(gTest->windowIds, 0xFF, 0x1);
    gTest->unk666 = 0xFF;
    gTest->spriteId = SPRITE_NONE;
}

static u8 TEST_InitWindow(void)
{
    u8 *windowIdLoc = &(gTest->windowIds[0]);
    if (*windowIdLoc == WINDOW_NONE)
    {
        *windowIdLoc = AddWindow(&TEST_WindowTemplates[0]);
        DrawStdFrameWithCustomTileAndPalette(*windowIdLoc, FALSE, 0x214, 0xE);
        ScheduleBgCopyTilemapToVram(0);
    }
    return *windowIdLoc;
}

static void TEST_CalculateUsedSlots(void) //calculate used slots
{
    TEST_CompactRegisteredItems();
    TEST_CalcAndSetUsedSlotsCount(gSaveBlock1Ptr->registeredItems, REGISTERED_ITEMS_MAX, &(TESTItemPageInfo.pageItems), &(TESTItemPageInfo.count), 3);
    //registeredItems
}

static void TEST_CalcCursorPos(void) //calc cursor pos
{
    sub_812225C(&(TESTItemPageInfo.itemsAbove), &(TESTItemPageInfo.cursorPos), TESTItemPageInfo.pageItems, TESTItemPageInfo.count); //fine
}

static void TEST_ItemStorage_RefreshListMenu(void)
{
    u16 i;
    u8 windowId = TEST_InitWindow();
    LoadMessageBoxAndBorderGfx();
    SetStandardWindowBorderStyle(windowId , 0);

    for(i = 0; i < TESTItemPageInfo.count - 1; i++)
    {
        TEST_CopyItemName(&(gTest->unk198[i][0]), gSaveBlock1Ptr->registeredItems[i].itemId);
        gTest->unk0[i].name = &(gTest->unk198[i][0]);
        gTest->unk0[i].id = i;
    }
    StringCopy(&(gTest->unk198[i][0]) ,gText_Cancel2);
    gTest->unk0[i].name = &(gTest->unk198[i][0]);
    gTest->unk0[i].id = -2;
    gMultiuseListMenuTemplate = gTEST_List;
    gMultiuseListMenuTemplate.windowId = windowId;
    gMultiuseListMenuTemplate.totalItems = TESTItemPageInfo.count;
    gMultiuseListMenuTemplate.items = gTest->unk0;
    gMultiuseListMenuTemplate.maxShowed = 3;//TESTItemPageInfo.pageItems;
}

static void TEST_ItemStorage_MoveCursor(s32 id, bool8 b, struct ListMenu *thisMenu)
{
    if (b != TRUE)
        PlaySE(SE_SELECT);
    if (gTest->unk666 == 0xFF)
    {
        TEST_RemoveItemIcon();
        if (id != -2)
            TEST_PrintItemIcon(gSaveBlock1Ptr->registeredItems[id].itemId);
        else
            TEST_PrintItemIcon(ITEMPC_GO_BACK_TO_PREV);
        // sub_816BEF0(id); //print item description
    }
}

static void TEST_ItemStorage_PrintFunc(u8 windowId, s32 id, u8 yOffset)
{
    if (id != -2)
    {
        if (gTest->unk666 != 0xFF)
        {
            if (gTest->unk666 == (u8)id)
                TEST_PrintSwappingCursor(yOffset, 0, 0xFF);
            else
                TEST_PrintSwappingCursor(yOffset, 0xFF, 0xFF);
        }
        // ConvertIntToDecimalStringN(gStringVar1, gSaveBlock1Ptr->pcItems[id].quantity, STR_CONV_MODE_RIGHT_ALIGN, 3);
        // StringExpandPlaceholders(gStringVar4, gText_xVar1);
        // AddTextPrinterParameterized(windowId, 7, gStringVar4, GetStringRightAlignXOffset(7, gStringVar4, 104), yOffset, 0xFF, NULL);
    }
}

static void TEST_PrintItemIcon(u16 itemId)
{
    u8 spriteId;
    u8* spriteIdLoc = &(gTest->spriteId);

    if (*spriteIdLoc == SPRITE_NONE)
    {
        FreeSpriteTilesByTag(0x13F6);
        FreeSpritePaletteByTag(0x13F6);
        spriteId = AddItemIconSprite(0x13F6, 0x13F6, itemId);
        if (spriteId != MAX_SPRITES)
        {
            *spriteIdLoc = spriteId;
            gSprites[spriteId].oam.priority = 0;
            gSprites[spriteId].pos2.x = 32;
            gSprites[spriteId].pos2.y = 132;
        }
    }
}

static void TEST_ItemStorage_DoItemSwap(u8 taskId, bool8 a)
{
    s16 *data;
    u16 b;

    data = gTasks[taskId].data;
    b = (TESTItemPageInfo.itemsAbove + TESTItemPageInfo.cursorPos);
    PlaySE(SE_SELECT);
    DestroyListMenuTask(data[5], &(TESTItemPageInfo.itemsAbove), &(TESTItemPageInfo.cursorPos));
    if (!a)
    {
        if (gTest->unk666 != b)
        {
            if (gTest->unk666 != b - 1)
            {
                TEST_MoveItemSlotInList(gSaveBlock1Ptr->registeredItems, gTest->unk666, b);
                TEST_ItemStorage_RefreshListMenu();
            }
        }
    }
    if (gTest->unk666 < b)
        TESTItemPageInfo.cursorPos--;
    SetSwapLineSpritesInvisibility(gTest->spriteIds, 7, TRUE);
    gTest->unk666 = 0xFF;
    data[5] = ListMenuInit(&gMultiuseListMenuTemplate, TESTItemPageInfo.itemsAbove, TESTItemPageInfo.cursorPos);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = TEST_ItemStorage_ProcessInput;
}

static void TEST_ItemStorage_StartScrollIndicator(void)
{
    if (TESTItemPageInfo.scrollIndicatorTaskId == TASK_NONE)
        TESTItemPageInfo.scrollIndicatorTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 28, 110, 148, TESTItemPageInfo.count - TESTItemPageInfo.pageItems, 0x13F8, 0x13F8, &(TESTItemPageInfo.itemsAbove)); //176, 12, 148 x, y1, y2
}

static void TEST_UpdateSwapLinePos(u8 y)
{
    UpdateSwapLineSpritesPos(gTest->spriteIds, 7, 48, ((y+1) * 16 + 90));
}

static void TEST_CopyItemName(u8 *string, u16 itemId)
{
    CopyItemName(itemId, string);
}

static const u8 gColor_gray[] = {0x01, 0x03, 0x02, 0x00};
static void TEST_PrintSwappingCursor(u8 y, u8 b, u8 speed)
{
    u8 x = 40;
    u8 windowId = gTest->windowIds[0];
    if (b == 0xFF)
        FillWindowPixelRect(windowId, PIXEL_FILL(1), x, y, GetMenuCursorDimensionByFont(1, 0), GetMenuCursorDimensionByFont(1, 1));
    else
        AddTextPrinterParameterized4(windowId, 1, x, y, 0, 0, gColor_gray, speed, gText_SelectorArrow2);
}

static void TEST_GetSwappingCursorPositionAndPrint(u8 a, u8 b, u8 speed)
{
    TEST_PrintSwappingCursor(ListMenuGetYCoordForPrintingArrowCursor(a), b, speed);
}


//registeredItems struct helper functions
static void TEST_MoveItemSlotInList(struct RegisteredItemSlot* registeredItemSlots_, u32 from, u32 to_)
{
    // dumb assignments needed to match
    struct RegisteredItemSlot *registeredItemSlots = registeredItemSlots_;
    u32 to = to_;

    if (from != to)
    {
        s16 i, count;
        struct RegisteredItemSlot firstSlot = registeredItemSlots[from];

        if (to > from)
        {
            to--;
            for (i = from, count = to; i < count; i++)
                registeredItemSlots[i] = registeredItemSlots[i + 1];
        }
        else
        {
            for (i = from, count = to; i > count; i--)
                registeredItemSlots[i] = registeredItemSlots[i - 1];
        }
        registeredItemSlots[to] = firstSlot;
    }
}

u8 TEST_CountUsedRegisteredItemSlots(void)
{
    u8 usedSlots = 0;
    u8 i;

    for (i = 0; i < PC_ITEMS_COUNT; i++)
    {
        if (gSaveBlock1Ptr->registeredItems[i].itemId != ITEM_NONE)
            usedSlots++;
    }
    return usedSlots;
}

static void TEST_RemoveRegisteredItemIndex(u8 index)
{
    // UB: should use GetPCItemQuantity and SetPCItemQuantity functions
    gSaveBlock1Ptr->registeredItems[index].itemId = ITEM_NONE;
    TEST_CompactRegisteredItems();
}

void TEST_RemoveRegisteredItem(u16 itemId)
{
    u8 i;
    for (i = i ; i < REGISTERED_ITEMS_MAX; i++)
        {
            if (gSaveBlock1Ptr->registeredItems[i].itemId == itemId)
            {
                gSaveBlock1Ptr->registeredItems[i].itemId == ITEM_NONE;
                TEST_CompactRegisteredItems();
                return;
            }
        }
}
void TEST_CompactRegisteredItems(void)
{
    u16 i;
    u16 j;

    for (i = 0; i < REGISTERED_ITEMS_MAX - 1; i++)
    {
        for (j = i + 1; j < REGISTERED_ITEMS_MAX; j++)
        {
            if (gSaveBlock1Ptr->registeredItems[i].itemId == ITEM_NONE)
            {
                struct RegisteredItemSlot temp = gSaveBlock1Ptr->registeredItems[i];
                gSaveBlock1Ptr->registeredItems[i] = gSaveBlock1Ptr->registeredItems[j];
                gSaveBlock1Ptr->registeredItems[j] = temp;
            }
        }
    }
}

static void TEST_CalcAndSetUsedSlotsCount(struct RegisteredItemSlot *slots, u8 count, u8 *arg2, u8 *usedSlotsCount, u8 maxUsedSlotsCount)
{
    u16 i;
    struct RegisteredItemSlot *slots_ = slots;

    (*usedSlotsCount) = 0;
    for (i = 0; i < count; i++)
    {
        if (slots_[i].itemId != ITEM_NONE)
            (*usedSlotsCount)++;
    }

    (*usedSlotsCount)++;
    if ((*usedSlotsCount) > maxUsedSlotsCount)
        *arg2 = maxUsedSlotsCount;
    else
        *arg2 = (*usedSlotsCount);
}

bool8 TEST_CheckRegisteredHasItem(u16 itemId)
{
    u8 i;

    for (i = 0; i < REGISTERED_ITEMS_MAX; i++)
    {
        if (gSaveBlock1Ptr->registeredItems[i].itemId == itemId)
            return TRUE;
    }
    return FALSE;
}

static s32 TEST_FindFreeRegisteredItemSlot(void)
{
    s8 i;

    for (i = 0; i < REGISTERED_ITEMS_MAX; i++)
    {
        if (gSaveBlock1Ptr->registeredItems[i].itemId == ITEM_NONE)
            return i;
    }
    return -1;
}

bool8 TEST_AddRegisteredItem(u16 itemId)
{
    u8 i;
    s8 freeSlot;
    struct ItemSlot *newItems;

    // Copy PC items
    newItems = AllocZeroed(sizeof(gSaveBlock1Ptr->registeredItems));
    memcpy(newItems, gSaveBlock1Ptr->registeredItems, sizeof(gSaveBlock1Ptr->registeredItems));

    // Use any item slots that already contain this item
    /*
    for (i = 0; i < REGISTERED_ITEMS_MAX; i++)
    {
        if (newItems[i].itemId == itemId)
        {
            TEST_RemoveRegisteredItemIndex(i);
            Free(newItems);
            return TRUE;
        }
    }
    */


    freeSlot = TEST_FindFreeRegisteredItemSlot();
    if (freeSlot == -1)
    {
        Free(newItems);
        return FALSE;
    }
    else
    {
        newItems[freeSlot].itemId = itemId;
    }

    // Copy items back to the PC
    memcpy(gSaveBlock1Ptr->registeredItems, newItems, sizeof(gSaveBlock1Ptr->registeredItems));
    Free(newItems);
    return TRUE;
}



//helper cleanup
static void TEST_RemoveItemIcon(void) //remove item storage selected item icon
{
    u8* spriteIdLoc = &(gTest->spriteId);
    if (*spriteIdLoc != SPRITE_NONE)
    {
        FreeSpriteTilesByTag(0x13F6);
        FreeSpritePaletteByTag(0x13F6);
        DestroySprite(&(gSprites[*spriteIdLoc]));
        *spriteIdLoc = SPRITE_NONE;
    }
}

static void TEST_RemoveWinow(void) //remove window
{
    u8 *windowIdLoc = &(gTest->windowIds[0]);
    if (*windowIdLoc != WINDOW_NONE)
    {
        ClearStdWindowAndFrameToTransparent(*windowIdLoc, FALSE);
        ClearWindowTilemap(*windowIdLoc);
        ScheduleBgCopyTilemapToVram(0);
        RemoveWindow(*windowIdLoc);
        *windowIdLoc = WINDOW_NONE;
    }
}

static void TEST_ItemStorage_RemoveScrollIndicator(void)
{
    if (TESTItemPageInfo.scrollIndicatorTaskId != TASK_NONE)
    {
        RemoveScrollIndicatorArrowPair(TESTItemPageInfo.scrollIndicatorTaskId);
        TESTItemPageInfo.scrollIndicatorTaskId = TASK_NONE;
    }
}

static void TEST_FreeStructs(void)
{
    Free(gTest);
}



