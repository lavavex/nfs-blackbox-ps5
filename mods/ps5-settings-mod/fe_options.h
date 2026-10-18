/*
 * Need for Speed: Most Wanted (2005) PC v1.3, DRM-free speed.exe.
 * sha256 80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c
 * Image base 0x400000. For .text/.rdata and the raw part of .data, file
 * offset equals RVA. No relocation directory.
 *
 * Hand reconstruction of the PC Display options page (FEng). Addresses are
 * virtual. This is not the Ghidra export under mw-re/decompiled/exe_c: that
 * export is the encrypted disc exe.
 *
 * A thiscall takes the object in ecx. Stack arguments are pushed right to
 * left. "ret N" means the callee removes N bytes. A cdecl leaves them for
 * the caller.
 */
#ifndef FE_OPTIONS_H
#define FE_OPTIONS_H

typedef unsigned int u32;

typedef struct FEObject FEObject;
typedef struct OptionWidget OptionWidget;
typedef struct OptionMenu OptionMenu;

/* 15 slots. The resolution row's table is at 0x89BB04.
 * Slot 0 destroys, slot 1 acts on a message, slot 3 draws. */
typedef struct WidgetVtable {
    void (__attribute__((thiscall)) *destroy)(OptionWidget *w, int free_it); /* ret 4 */
    void (__attribute__((thiscall)) *act)(OptionWidget *w, int ignored, u32 hash); /* ret 8 */
    void (__attribute__((thiscall)) *slot2)(OptionWidget *w);
    void (__attribute__((thiscall)) *draw)(OptionWidget *w); /* ret */
    void (__attribute__((thiscall)) *slot4)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot5)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot6)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot7)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot8)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot9)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot10)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot11)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot12)(OptionWidget *w);
    void (__attribute__((thiscall)) *slot13)(OptionWidget *w);
    void (__attribute__((thiscall)) *notify)(OptionWidget *w, u32 hash); /* +0x38, ret 4 */
} WidgetVtable;

/* Intrusive node. The menu's head lives at menu+0x2C. A widget's node is widget+4. */
typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

struct OptionWidget {
    WidgetVtable *vt;          /* +0x00 */
    ListNode node;             /* +0x04 */
    u32 field_0c;              /* +0x0C  copied from menu+0xD4 */
    u32 field_10;              /* +0x10  copied from menu+0xD8 */
    u32 hit_x;                 /* +0x14  filled from the right arrow */
    u32 hit_y;                 /* +0x18 */
    u32 flags_1c;              /* +0x1C  init writes float bits 0xC38A0000 */
    u32 flags_20;              /* +0x20  init writes float bits 0xC1200000 */
    FEObject *backing;         /* +0x24 */
    unsigned char enabled;     /* +0x28  the byte passed to init. Nonzero skips slot 8. */
    unsigned char field_29;    /* +0x29 */
    unsigned char dirty;       /* +0x2A  set when the row handles a message */
    unsigned char pad_2b;
    FEObject *label;           /* +0x2C */
    FEObject *value;           /* +0x30 */
    u32 geom_34;               /* +0x34 .. +0x48 copied from menu+0xE4 .. +0xF8 */
    u32 geom_38;
    u32 geom_3c;
    u32 geom_40;
    u32 geom_44;
    u32 geom_48;
    FEObject *left_arrow;      /* +0x4C */
    FEObject *right_arrow;     /* +0x50 */
    u32 field_54;              /* +0x54  init writes 0x7AB5521A */
    u32 field_58;              /* +0x58  init writes 0x36819D93 */
};

/* Language record. The table at [0x91CF80] is sorted by hash, ascending, unsigned.
 * Count is [0x91CF78]. */
typedef struct LangRecord {
    u32 hash;
    char *text;
} LangRecord;

/* FE object fields used by this page. Type 1 is an image, type 2 is a string. */
struct FEObject {
    void *vt;
    void *next;
    void *prev;
    u32 guid;
    u32 name_hash;
    char *name;
    u32 type;                  /* +0x18 */
    u32 flags;                 /* +0x1C */
    u32 field_20;
    void *field_24;            /* +0x24 */
    u32 pad[14];
    u32 language_hash;         /* +0x60 */
};

/* Offsets below are the ones the display page uses. The object is 0x158 bytes. */
struct OptionMenu {
    void *vt;                  /* +0x00  PC options screen: 0x89CF60 (4 slots) */
    u32 pad_04[3];
    char *package_name;        /* +0x10 */
    u32 pad_14[6];
    ListNode widgets;          /* +0x2C  circular */
    u32 field_34;              /* +0x34  cleared on rebuild */
    u32 pad_38;
    FEObject *measure_a;       /* +0x3C */
    FEObject *measure_b;       /* +0x40 */
    u32 pad_44[3];
    FEObject *field_50;        /* +0x50  scroll-marker target */
    u32 pad_54[23];
    char *name_template;       /* +0xB0  "OPTION_NAME_" */
    char *data_template;       /* +0xB4  "OPTION_DATA_" */
    char *image_template;      /* +0xB8  "OPTION_IMAGE_" */
    char *backing_template;    /* +0xBC  "OPTION_BACKING_" */
    char *left_template;       /* +0xC0  "LEFT_ARROW_" */
    char *right_template;      /* +0xC4  "RIGHT_ARROW_" */
    char *slider_template;     /* +0xC8  "SLIDER_" */
    u32 origin_x;              /* +0xCC */
    u32 origin_y;              /* +0xD0 */
    u32 cursor_x;              /* +0xD4 */
    u32 cursor_y;              /* +0xD8 */
    u32 row_gap;               /* +0xDC */
    u32 row_pitch;             /* +0xE0 */
    u32 box_x0;                /* +0xE4 */
    u32 box_y0;                /* +0xE8 */
    u32 box_x1;                /* +0xEC */
    u32 box_y1;                /* +0xF0 */
    u32 box_x2;                /* +0xF4 */
    u32 box_y2;                /* +0xF8 */
    u32 field_fc;
    u32 y_step;                /* +0x100 */
    int next_index;            /* +0x104  first row is 1 */
    int field_108;             /* +0x108 */
    int scroll_threshold;      /* +0x10C  9, or 10 when opened in-game. Not a slot cap. */
    u32 pad_110[3];
    unsigned char field_11c;
    unsigned char field_11d;   /* +0x11D cleared on rebuild */
    unsigned char pad_11e[6];
    unsigned char in_game;     /* +0x124 */
    unsigned char field_125;
    unsigned char field_126;
    unsigned char pad_127[9];
    u32 field_130;
    u32 field_134;
    u32 field_138;
    u32 field_13c;
    u32 field_140;             /* +0x140  online page: pointer to a 0x100-byte block */
    void *options;             /* +0x144  0x84-byte block. Global 0x91CA20 points here. */
    u32 options_wrap;          /* +0x148  copy of [0x982C00] */
    void *options_copy;        /* +0x14C  second 0x84-byte block */
    u32 options_copy_wrap;     /* +0x150 */
    unsigned char advanced;    /* +0x154  0 = two rows, 1 = the long display page */
};

/* PC options screen vtable 0x89CF60. Slot 3 (+0x0C) is OptionsPopulate. */
enum {
    PC_DISPLAY_SIMPLE_ROWS = 2,
    PC_DISPLAY_ADVANCED_ROWS = 13,
    PC_DISPLAY_NAME_SLOTS = 15, /* OPTION_NAME_1 .. _15 in OptionsPCDisplay.fng */

    HASH_LEFT = 0x9120409E,
    HASH_RIGHT = 0xB5971BF1,
    HASH_MISSING = 0x9BB9CCC3, /* ERROR_DEFAULT_STRING */

    RES_COUNT = 6
};

void *GameMalloc(u32 bytes);                 /* 0x652AD0, cdecl, jmp 0x7C66D0 */
void GameFree(void *p);                      /* 0x652B00, cdecl */

char *LookupLanguage(u32 hash);              /* 0x56BB80, hash in edx */
char *GetLocalizedString(u32 hash);          /* 0x56BC10, cdecl */
char *GetLocalizedString2(u32 hash);         /* 0x56BC60, cdecl, same body */
void CopyBounded(char *dest, const char *src, int count); /* 0x460CA0, cdecl */

FEObject *FindStringObject(const char *package, u32 hash); /* 0x5251E0, cdecl */
void SetObjectLanguageHash(FEObject *obj, u32 hash);       /* 0x515C00, cdecl */
void SetPackageLanguageHash(const char *package, u32 object_hash, u32 string_hash); /* 0x525220 */

FEObject *ResolveNumbered(OptionMenu *menu, const char *templ); /* 0x573CC0, thiscall ret 4 */
FEObject *ResolveArrow(OptionMenu *menu, const char *templ);    /* 0x573D10, thiscall ret 4 */

void ToggleWidget_Init(OptionWidget *w, int enabled);     /* 0x589300, thiscall ret 4. Object is 0x5C. */
void SliderWidget_Init(OptionWidget *w, int enabled);     /* 0x589540, thiscall ret 4. Object is 0x9C. */
void ToggleWidget_Destroy(OptionWidget *w, int free_it);  /* 0x528430, thiscall ret 4 */

void Menu_ClearRows(OptionMenu *menu);                    /* 0x588C90, thiscall */
int Menu_AddToggle(OptionMenu *menu, OptionWidget *w, int unused); /* 0x588570, thiscall ret 8 */
int Menu_AddSlider(OptionMenu *menu, OptionWidget *w, int unused); /* 0x588860, thiscall ret 8 */

void OptionsPopulate(OptionMenu *menu);                   /* 0x545C50, thiscall */
void BuildPCDisplay(OptionMenu *menu);                    /* 0x5295F0, thiscall. Only caller: 0x545D95. */
void BuildOnlineOptions(OptionMenu *menu);                /* 0x52A290, thiscall. Only caller: 0x545DEA. */

void Resolution_Act(OptionWidget *w, int ignored, u32 hash); /* 0x50F970, thiscall ret 8 */
void Resolution_Draw(OptionWidget *w);                    /* 0x51B7A0, thiscall */

/* Current resolution index: (*(int **)0x91CA20)[0x6C/4], i.e. options->field at +0x6C.
 * The block is 0x84 bytes. What saves it (registry / user.reg) is not traced here. */

#endif
