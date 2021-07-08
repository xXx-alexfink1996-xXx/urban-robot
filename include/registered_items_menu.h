#ifndef GUARD_REGISTERED_ITEMS_MENU
#define GUARD_REGISTERED_ITEMS_MENU

struct TEST_ItemPageStruct
{
    u16 cursorPos;
    u16 itemsAbove;
    u8 pageItems;
    u8 count;
    u8 filler[3];
    u8 scrollIndicatorTaskId;
};

void TEST_RegisteredItemsMenuNewGame(void);
void TEST_PlayerPC(void);
u8 TEST_CountUsedRegisteredItemSlots(void);
void TEST_RemoveRegisteredItem(u16 itemId);
void TEST_CompactRegisteredItems(void);
bool8 TEST_AddRegisteredItem(u16 itemId);
bool8 TEST_CheckRegisteredHasItem(u16 itemId);
bool8 TEST_AddRegisteredItem(u16 itemId);
static s32 TEST_FindFreeRegisteredItemSlot(void);
u8 TEST_GetRegisteredItemIndex(u16 itemId);

#endif
