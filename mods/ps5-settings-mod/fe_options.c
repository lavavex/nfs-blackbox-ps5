/*
 * Reconstruction of the PC Display options page from DRM-free speed.exe 1.3.
 * Every function here was read back from that file with Capstone. Helpers that
 * were only called, not opened, are declared in fe_options.h and left out.
 *
 * Conventions: thiscall object in ecx; a comment gives the VA and the return.
 */
#include "fe_options.h"

/* Globals the page actually touches. */
#define LANG_TABLE   (*(LangRecord **)0x91CF80)
#define LANG_COUNT   (*(u32 *)0x91CF78)
#define CFENG        (*(void **)0x91CADC)
#define P_OPTIONS    (*(void ***)0x91CA20)   /* points at menu+0x144 */
#define LANG_NOTIFY  (*(void **)0x91DD08)
#define CATEGORY_ROOT (*(void **)0x91CF90)

static void *FindInPackage(void *package, u32 hash); /* 0x5B8550, thiscall ret 4 */
static void *FindPackage(void *cfeng, const char *name); /* 0x516AF0, thiscall ret 4 */
static u32 FEHashUpper(const char *s); /* 0x5AF1C0, cdecl */
static void Format(char *dest, const char *a, const char *b); /* 0x460ED0, cdecl. Joins a and b. */
static int Snprintf(char *dest, u32 cap, const char *fmt, ...); /* 0x515E20 */
static void HideObject(FEObject *obj); /* 0x514C70, cdecl */
static void OptionsBlock_Construct(void *block); /* 0x50EFB0, thiscall */
static void FinishPopulate(OptionMenu *menu, int arg); /* 0x5A5EA0, thiscall ret 4 */

/* -------------------------------------------------------------------------- */
/* 0x56BB80. Hash in edx. Binary search, unsigned, ascending. Returns the
 * string pointer, or null. Does not use the stack for the hash. */
char *LookupLanguage(u32 hash)
{
    LangRecord *table = LANG_TABLE;
    if (!table)
        return 0;
    int hi = (int)LANG_COUNT - 1;
    int lo = 0;
    int mid = hi >> 1;
    u32 found = table[mid].hash;
    for (;;) {
        if (found == hash)
            return table[mid].text;
        if (hi - lo <= 2) {
            if (table[lo].hash == hash) return table[lo].text;
            if (found == hash) return table[mid].text;
            if (table[hi].hash == hash) return table[hi].text;
        }
        if (mid == lo)
            return 0;
        if (found > hash)
            hi = mid;
        else if (found < hash)
            lo = mid;
        mid = (hi + lo) >> 1;
        found = table[mid].hash;
    }
}

/* 0x56BC10 and the identical copy at 0x56BC60. cdecl. */
char *GetLocalizedString(u32 hash)
{
    char *text = LookupLanguage(hash);
    if (text)
        return text;
    return LookupLanguage(HASH_MISSING);
}

char *GetLocalizedString2(u32 hash)
{
    return GetLocalizedString(hash);
}

/* 0x56BC30. cdecl (dest, count, hash). On either hit or miss it tail-calls
 * 0x460CA0 with (dest, text, count). A null text still reaches the copy. */
void GetLocalizedWideBytes(char *dest, int count, u32 hash)
{
    char *text = LookupLanguage(hash);
    if (!text)
        text = LookupLanguage(HASH_MISSING);
    CopyBounded(dest, text, count);
}

/* 0x460CA0. cdecl (dest, src, count). Copies at most count bytes, stops after
 * writing a NUL, and returns dest. Caller cleans the stack. */
void CopyBounded(char *dest, const char *src, int count)
{
    if (!count)
        return;
    char *out = dest;
    while (count--) {
        char c = *src++;
        *out = c;
        if (!c)
            break;
        out++;
    }
}

/* 0x57E920. This is the wide lookup Widescreen Fix hooks. cdecl, hash is the
 * third argument. A hit copies through 0x4611C0 and returns 1. The miss path
 * was not transcribed; Widescreen only substitutes a resolution string when
 * this function returns 0, so a hash present in the table is left alone. */
int GetLocalizedWide(void *dest, int count, u32 hash)
{
    char *text = LookupLanguage(hash);
    if (!text)
        goto miss; /* 0x57E956. Not transcribed. */
    if (*(void **)0x91CF84 == 0) {
        /* 0x4611C0(dest, text); then return 1. */
        (void)dest;
        (void)text;
        (void)count;
        return 1;
    }
    /* The branch at 0x57E948 was not transcribed. */
    return 1;
miss:
    (void)dest;
    (void)count;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* 0x5251E0. cdecl (package name, hash). Null unless the object's type is 2. */
FEObject *FindStringObject(const char *package, u32 hash)
{
    if (!package)
        return 0;
    void *pkg = FindPackage(CFENG, package);
    if (!pkg)
        return 0;
    FEObject *obj = FindInPackage(pkg, hash);
    if (!obj || obj->type != 2)
        return 0;
    return obj;
}

/* 0x515C00. cdecl (object, hash). The same flag/notify sequence Draw uses. */
void SetObjectLanguageHash(FEObject *obj, u32 hash)
{
    if (!obj)
        return;
    obj->flags |= 0x400000;
    obj->language_hash = hash;
    if (LANG_NOTIFY) {
        void (__attribute__((thiscall)) *notify)(void *, FEObject *) =
            *(void (__attribute__((thiscall)) **)(void *, FEObject *))(*(void **)LANG_NOTIFY);
        notify(LANG_NOTIFY, obj);
    }
    obj->flags = (obj->flags & ~2u) | 0x400000;
}

/* 0x573CC0. thiscall, ret 4. templ is "OPTION_NAME_" or "OPTION_DATA_".
 * Builds "<templ>%d", prints menu->next_index into it, and finds that string. */
FEObject *ResolveNumbered(OptionMenu *menu, const char *templ)
{
    char joined[0x40];
    char printed[0x20];
    Format(joined, templ, "%d");                 /* "%d" lives at 0x89D764 */
    Snprintf(printed, 0x20, joined, menu->next_index);
    u32 hash = FEHashUpper(printed);
    return FindStringObject(menu->package_name, hash);
}

/* 0x573D10. thiscall, ret 4. Same numbering. If that object is missing,
 * 0x573140 formats "<templ>0" (the format at 0x8A0BB8 is "%s0") and looks
 * that up instead. Arrow rows use this. Name and value rows do not. */
/* 0x5251A0. cdecl (package name, hash). Same as FindStringObject, but the
 * object must be type 1 (an image). Arrows are images. */
static FEObject *FindImageObject(const char *package, u32 hash)
{
    if (!package)
        return 0;
    void *pkg = FindPackage(CFENG, package);
    if (!pkg)
        return 0;
    FEObject *obj = FindInPackage(pkg, hash);
    if (!obj || obj->type != 1)
        return 0;
    return obj;
}

/* 0x573140. cdecl ("%s0", template). Returns the hash passed to FindImageObject.
 * The format literal is "%s0", so the looked-up name is the template with a
 * trailing 0: LEFT_ARROW_0, RIGHT_ARROW_0. Its body was not copied line by line. */
static u32 HashTemplateZero(const char *fmt, const char *templ);

FEObject *ResolveArrow(OptionMenu *menu, const char *templ)
{
    char joined[0x40];
    char printed[0x20];
    Format(joined, templ, "%d");
    Snprintf(printed, 0x20, joined, menu->next_index);
    FEObject *obj = FindImageObject(menu->package_name, FEHashUpper(printed));
    if (obj)
        return obj;
    return FindImageObject(menu->package_name, HashTemplateZero("%s0", templ));
}

/* -------------------------------------------------------------------------- */
/* 0x589300. thiscall, ret 4. Zeros a 0x5C toggle and stores the flag byte.
 * The caller overwrites the vtable immediately afterwards. */
void ToggleWidget_Init(OptionWidget *w, int enabled)
{
    w->field_0c = 0;
    w->field_10 = 0;
    w->hit_x = 0;
    w->hit_y = 0;
    w->flags_1c = 0xC38A0000;
    w->flags_20 = 0xC1200000;
    w->backing = 0;
    w->enabled = (unsigned char)enabled;
    w->field_29 = 0;
    w->dirty = 0;
    w->label = 0;
    w->value = 0;
    w->geom_34 = w->geom_38 = w->geom_3c = 0;
    w->geom_40 = w->geom_44 = w->geom_48 = 0;
    w->vt = (WidgetVtable *)0x8A140C;
    w->left_arrow = 0;
    w->right_arrow = 0;
    w->field_54 = 0x7AB5521A;
    w->field_58 = 0x36819D93;
}

/* 0x528430. Slot 0 of the resolution vtable. 0x528450 only stores 0x89B890
 * into the vtable pointer. The flag bit frees the widget with GameFree. */
void ToggleWidget_Destroy(OptionWidget *w, int free_it)
{
    w->vt = (WidgetVtable *)0x89B890;
    if (free_it & 1)
        GameFree(w);
}

/* 0x588C90. thiscall. Deletes every row, sets next_index back to 1, then
 * jumps to 0x573DD0 to remeasure the arrow boxes. OptionsPopulate calls this
 * before it builds, including when the advanced page is toggled. */
void Menu_ClearRows(OptionMenu *menu)
{
    ListNode *head = &menu->widgets;
    OptionWidget *sentinel = (OptionWidget *)((char *)head - 4);
    OptionWidget *w = head->next ? (OptionWidget *)((char *)head->next - 4) : 0;
    while (w != sentinel) {
        w->vt->slot6(w);
        w->vt->slot10(w);
        ListNode *next = w->node.next;
        w = next ? (OptionWidget *)((char *)next - 4) : 0;
    }
    while (head->next != head) {
        ListNode *node = head->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        OptionWidget *dead = (OptionWidget *)((char *)node - 4);
        if (dead)
            dead->vt->destroy(dead, 1);
    }
    menu->next_index = 1;
    menu->field_108 = 1;
    menu->field_34 = 0;
    menu->field_11d = 0;
    /* tail: 0x573DD0 remeasures menu+0x3C and menu+0x40 into the cursor box */
}

/* 0x588570. thiscall, ret 8. The second argument is not read. Binds the row's
 * FE objects from the current next_index, links the widget, advances the
 * cursor, then draws it. Returns the index it consumed. */
int Menu_AddToggle(OptionMenu *menu, OptionWidget *w, int unused)
{
    (void)unused;
    w->label = ResolveNumbered(menu, menu->name_template);
    w->value = ResolveNumbered(menu, menu->data_template);
    w->backing = (FEObject *)0; /* 0x573D80(menu, menu->backing_template): numbered, no "_0" fallback */
    (void)w->backing;
    w->field_0c = menu->cursor_x;
    w->field_10 = menu->cursor_y;
    w->geom_34 = menu->box_x0;
    w->geom_38 = menu->box_y0;
    w->geom_3c = menu->box_x1;
    w->geom_40 = menu->box_y1;
    w->geom_44 = menu->box_x2;
    w->geom_48 = menu->box_y2;
    w->left_arrow = ResolveArrow(menu, menu->left_template);
    w->right_arrow = ResolveArrow(menu, menu->right_template);

    ListNode *head = &menu->widgets;
    ListNode *prev = head->prev;
    prev->next = &w->node;
    head->prev = &w->node;
    w->node.next = head;
    w->node.prev = prev;

    /* cursor_y += y_step + row_pitch; box_y2 = cursor_y; next_index++ */
    menu->next_index += 1;

    if (w->enabled == 0)
        w->vt->slot8(w);
    w->vt->slot5(w);
    w->vt->draw(w);
    w->vt->slot4(w);

    /* 0x514FD0 / 0x515520 on the right arrow fill w->hit_x and w->hit_y. */
    return menu->next_index - 1;
}

/* -------------------------------------------------------------------------- */
/* 0x50F970. Resolution row, slot 1. thiscall ret 8.
 * The hash is the second stack argument. Any other hash leaves the index. */
void Resolution_Act(OptionWidget *w, int ignored, u32 hash)
{
    (void)ignored;
    void *block = *P_OPTIONS;                 /* menu+0x144 */
    int *index = (int *)((char *)block + 0x6C);
    if (hash == HASH_LEFT || hash == HASH_RIGHT) {
        int step = (hash == HASH_LEFT) ? -1 : 1;
        int next = *index + step;
        if (next < 0)
            next = RES_COUNT - 1;
        if (next >= RES_COUNT)
            next = 0;
        /* Skip a zero entry in the six-wide support table at 0x93DBEC. */
        while (*(u32 *)(0x93DBEC + next * 4) == 0) {
            next += step;
            if (next < 0) next = RES_COUNT - 1;
            if (next >= RES_COUNT) next = 0;
        }
        *index = next;
    }
    w->dirty = 1;
    w->vt->notify(w, hash);
    w->vt->draw(w);
}

/* Value hashes, in index order. Each one is a Labels.bin key. */
static const u32 kResolutionValues[RES_COUNT] = {
    0xDCCBD59E, /* OPT_VO_PC_RES_640X480 */
    0x6D6BB416, /* 800X600 */
    0xA7470774, /* 1024X768 */
    0x4EF21B32, /* 1280X960 */
    0x2D31055A, /* 1280X1024 */
    0x76667B12  /* 1600X1200 */
};

/* 0x51B7A0. Slot 3. Writes the label hash and the value hash to object+0x60. */
void Resolution_Draw(OptionWidget *w)
{
    if (w->label) {
        w->label->flags |= 0x400000;
        w->label->language_hash = 0xC96D57C9; /* OPT_VO_RESOLUTION */
        if (LANG_NOTIFY) {
            /* virtual call, ecx = LANG_NOTIFY, one stack arg = the FE object */
        }
        w->label->flags = (w->label->flags & ~2u) | 0x400000;
    }
    int index = *(int *)((char *)(*P_OPTIONS) + 0x6C);
    u32 value = 0;
    if ((u32)index < RES_COUNT)
        value = kResolutionValues[index];
    if (w->value) {
        w->value->flags |= 0x400000;
        w->value->language_hash = value;
        w->value->flags = (w->value->flags & ~2u) | 0x400000;
    }
}

/* -------------------------------------------------------------------------- */
/* One display row. Size 0x5C uses ToggleWidget_Init and Menu_AddToggle.
 * Size 0x9C uses SliderWidget_Init and Menu_AddSlider. The init byte is 1,
 * or (global > 0) where a global is named. The row is added either way. */
static void AddRow(OptionMenu *menu, u32 bytes, WidgetVtable *vt, int enabled, int slider)
{
    OptionWidget *w = GameMalloc(bytes);
    if (w) {
        if (slider)
            SliderWidget_Init(w, enabled);
        else
            ToggleWidget_Init(w, enabled);
        w->vt = vt;
    }
    if (slider)
        Menu_AddSlider(menu, w, 1);
    else
        Menu_AddToggle(menu, w, 1);
}

/* 0x5295F0. thiscall. The only caller is the call at 0x545D95. */
void BuildPCDisplay(OptionMenu *menu)
{
    /* If package name is set, find object 0x08007B4C. When it is an image
     * (type 1) and its +0x24 pointer is not 0x8A006328, store that pointer
     * and set flags |= 0x02400000. */
    u32 title = menu->in_game ? 0xD94EA03F : 0x48478029;
    /* PAUSE_VIDEO_OPTIONS_TITLE, or OPT_VIDEO_TITLE. Object hash 0x42ADB44C. */
    SetPackageLanguageHash(menu->package_name, 0x42ADB44C, title);

    if (!menu->options) {
        void *block = GameMalloc(0x84);
        if (block)
            OptionsBlock_Construct(block);
        menu->options = block;
        /* rep movsd 0x21 dwords from 0x9017B0 over the block, then
         * menu->options_wrap = [0x982C00]. The copy replaces the constructor's
         * bytes. Side effects of 0x50EFB0 outside the block were not checked. */
    }
    if (!menu->options_copy) {
        void *block = GameMalloc(0x84);
        if (block)
            OptionsBlock_Construct(block);
        menu->options_copy = block;
        /* The stores between here and the header lookup were not transcribed
         * line by line. They follow the same shape as the first block. */
    }

    /* Header string 0xCE757C8A, type 2, kept in ebp. Null is allowed. */
    FEObject *header = FindStringObject(menu->package_name, 0xCE757C8A);

    if (menu->advanced == 0) {
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BB40, 1, 1); /* OPT_VO_LOD */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BB04, 1, 0); /* OPT_VO_RESOLUTION */
        if (header)
            header->language_hash = 0x1459065C; /* H_ADVANCED */
    } else {
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BB04, 1, 0);                  /* resolution */
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BC78, *(int *)0x901890 > 0, 1); /* OPT_DO_AA */
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BCB8, *(int *)0x9018A0 > 0, 1); /* texture filtering */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BD34, 1, 0);                  /* vsync */
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BC38, *(int *)0x90187C > 0, 1); /* world LOD */
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BD70, *(int *)0x90185C > 0, 1); /* road reflections */
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BE28, *(int *)0x9018B8 > 0, 1); /* shadows */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BBFC, *(int *)0x901880 > 0, 0); /* car geometry */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BBC0, *(int *)0x901840 > 0, 0); /* car reflections */
        AddRow(menu, 0x9C, (WidgetVtable *)0x89BB80, *(int *)0x901844 > 0, 1); /* reflect update rate */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BDB0, *(int *)0x901898 > 0, 0); /* rain */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BCF8, *(int *)0x901884 > 0, 0); /* overbright */
        AddRow(menu, 0x5C, (WidgetVtable *)0x89BDEC, *(int *)0x9018B0 > 0, 0); /* visual treatment */
        if (header)
            header->language_hash = 0x9C88B5C8; /* H_BASIC */
    }

    /* Shared tail 0x529BD7: if header is non-null, run the same notify
     * SetObjectLanguageHash uses, then the SEH epilogue. Both paths reach it.
     * next_index is 3 after the short page and 14 after the long one. */
    if (header)
        SetObjectLanguageHash(header, header->language_hash);
}

/* 0x52A290. thiscall. Category 9. The only caller is the call at 0x545DEA
 * (`E8 A1 44 FE FF`), which is `mov ecx, esi` at 0x545DE8 then this call.
 * The package router sends category 9 through the same path as categories
 * 0, 3 and 4 (0x545642): Options.fng, or Pause_Options.fng only when
 * screen+0x148 is set and bit 3 of [0x91CF90]+0x12C is clear. The Online
 * menu entry itself is added only when that bit is set, so the page the
 * player opens is Options.fng. That package has OPTION_NAME_1..10,
 * OPTION_DATA_1..10, and LEFT_ARROW_ / RIGHT_ARROW_ 1..10, plus the title
 * object 0x42ADB44C.
 *
 * The menu entry is built at 0x5290D0 inside 0x528F30 (caller 0x5452C4,
 * bytes `E8 67 3C FE FF`). IconOption_Create stores OL_OPTIONS (0x7D347E05)
 * at the icon +0x2C. The highlight updater 0x573870 writes that hash onto
 * the highlight FE objects through 0x515C00. The entry's message handler
 * 0x50F100 sets the category dword to 9 on hash 0x0C407210.
 *
 * 0x57FE80, called on the 0x100-byte block, was not opened. */
static void OnlineSettings_Construct(void *block); /* 0x57FE80, thiscall */

void BuildOnlineOptions(OptionMenu *menu)
{
    u32 first = menu->in_game ? 0x966C856D : 0xE463B5F7; /* pause title, or OPT_ONLINE_TITLE */
    SetPackageLanguageHash(menu->package_name, 0x42ADB44C, first);
    AddRow(menu, 0x5C, (WidgetVtable *)0x89C1B0, 1, 0); /* label OL_OP_BADCONWARNING */
    SetPackageLanguageHash(menu->package_name, 0x42ADB44C, 0x22B1C2E6); /* OL_OPTIONS_TITLE */
    {
        void *block = GameMalloc(0x100);
        if (block)
            OnlineSettings_Construct(block);
        menu->field_140 = (u32)block;
        /* rep movsd, 0x40 dwords, from [0x91CF90]+0x134 into the block. */
    }
}

/* 0x50F710. Slot 1 of vtable 0x89C1B0. thiscall ret 8. Left or right flips
 * the byte at [0x91CF90]+0x175. Any other hash leaves it. Then slot 14 and
 * Draw, same shape as the resolution row. */
void OnlineWarning_Act(OptionWidget *w, int ignored, u32 hash)
{
    (void)ignored;
    if (hash == HASH_LEFT || hash == HASH_RIGHT) {
        unsigned char *flag = (unsigned char *)0x91CF90 + 0x175;
        *flag = *flag ? 0 : 1;
    }
    w->dirty = 1;
    w->vt->notify(w, hash);
    w->vt->draw(w);
}

/* 0x51B4A0. Slot 3. Label hash OL_OP_BADCONWARNING (0xD1009B34). The value
 * hash is COMMON_ON (0x417B2604) when byte [0x91CF90]+0x175 is set, and
 * COMMON_OFF (0x70DFE5C2) when it is clear. The notify call is the same one
 * SetObjectLanguageHash makes; it is not repeated here. */
void OnlineWarning_Draw(OptionWidget *w)
{
    void *root = *(void **)0x91CF90;
    unsigned char on = *(unsigned char *)((char *)root + 0x175);
    if (w->label) {
        w->label->flags |= 0x400000;
        w->label->language_hash = 0xD1009B34;
        w->label->flags = (w->label->flags & ~2u) | 0x400000;
    }
    if (w->value) {
        w->value->flags |= 0x400000;
        w->value->language_hash = on ? 0x417B2604 : 0x70DFE5C2;
        w->value->flags = (w->value->flags & ~2u) | 0x400000;
    }
}

/* 0x545C50. thiscall. Vtable slot 3 of the PC options screen, and the direct
 * call from the constructor at 0x54FE4E. */
void OptionsPopulate(OptionMenu *menu)
{
    Menu_ClearRows(menu);
    menu->field_125 = 0;
    /* Hides five objects (0xE54C30BE, 0x8E1BEA84, 0x608BB8C8, 0x444969FD,
     * 0x444969FE) through FindPackage / FindInPackage / HideObject. */
    (void)HideObject;

    /* Category is [[[0x91CF90]+0x10]+0x24]. Above 9 uses the empty case. */
    void *root = *(void **)((char *)CATEGORY_ROOT + 0x10);
    u32 category = *(u32 *)((char *)root + 0x24);
    if (category > 9)
        category = 2;

    switch (category) {
    case 0: /* 0x545D81 */ (void)0x529310; break; /* gameplay-style page */
    case 1: /* 0x545D93 */ BuildPCDisplay(menu); break;
    case 3: /* 0x545DA5 */ (void)0x529C10; break;
    case 4: /* 0x545DB7 */ (void)0x529F10; break; /* then shows the two 0x444969Fx objects */
    case 9: /* 0x545DE8 */ BuildOnlineOptions(menu); break;
    default: break; /* cases 2, 5, 6, 7, 8 and the ja target: no rows */
    }
    FinishPopulate(menu, 0);
}

/* 0x545A91, inside the screen message handler. Flips the advanced byte and
 * calls vtable+0x0C, which is OptionsPopulate on this screen. */
void ToggleAdvanced(OptionMenu *menu)
{
    menu->advanced = menu->advanced ? 0 : 1;
    OptionsPopulate(menu);
}

/* 0x54FD60. thiscall, ret 4. Sole caller 0x570AB9 allocates 0x158 bytes.
 * Base constructor 0x593920 stores the seven name templates and next_index = 1,
 * and sets scroll_threshold to 7. This constructor then overwrites it. */
OptionMenu *OptionsScreen_Construct(OptionMenu *menu, void *arg)
{
    (void)arg;
    /* 0x593920: templates at 0x8A1944 .. 0x8A18F0, next_index = 1, threshold = 7 */
    menu->vt = (void *)0x89CF60;
    menu->in_game = 0; /* actually: byte from the constructor argument, compared below */
    menu->options = 0;
    P_OPTIONS = &menu->options;
    menu->advanced = 0;
    menu->options_copy = 0;
    menu->scroll_threshold = 9 + (menu->in_game != 0);
    OptionsPopulate(menu);
    return menu;
}

/* 0x593B40. Scroll-marker choice, not a limit on next_index.
 * cmp edi, [menu+0x10C]; ja uses a fallback marker. edi is a row count the
 * caller computed. Adding a 14th or 15th name does not take this branch's
 * failure path by itself. */
void Menu_PlaceScrollMarker(OptionMenu *menu, int row_count)
{
    if (row_count != 0 && row_count <= menu->scroll_threshold) {
        /* 0x573140(menu, "POS%d" at 0x89C32C, row_count) then show it. */
    }
}
