/*
 * X360Stuff Async Luminance: Xbox 360 Stuff Pack 4.2's exposure adaptation
 * without the per-frame GPU stall.
 *
 * Every frame X360Stuff renders the scene's luminance into a small lockable
 * render target (created at 0x10035337, kept at 0x100610c0) and locks it
 * read-only right away (0x1003678a) to average it on the CPU. Under DXVK that
 * lock waits for all the GPU work queued so far: on PS5 2-5 ms a frame, the
 * game thread idle meanwhile (measured with NFSMWD3D9Trace, 2026-10-11).
 *
 * Here the lock copies the target into one of RING system-memory surfaces
 * (GetRenderTargetData, queued, no wait) and hands X360Stuff the copy made
 * RING-1 frames earlier, long finished; the unlock (0x1003687e) releases that
 * copy. Adaptation is gradual, so the few frames' delay does not show.
 *
 * X360Stuff.asi has a fixed base (no relocations), so its addresses are
 * stable; both sites are checked byte for byte and nothing is patched on any
 * other build (see X360StuffAsyncLuminance.log).
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define DEVICE (*(void **)0x982bdc)         /* the game's IDirect3DDevice9 */
#define RING 4

typedef struct { UINT Format, Type, Usage, Pool, MultiSampleType, MultiSampleQuality, Width, Height; } SurfDesc;
typedef HRESULT (__stdcall *GetDesc_t)(void *surf, SurfDesc *desc);                    /* surface 12 */
typedef HRESULT (__stdcall *LockRect_t)(void *surf, void *locked, const RECT *r, DWORD flags); /* 13 */
typedef HRESULT (__stdcall *UnlockRect_t)(void *surf);                                  /* 14 */
typedef ULONG (__stdcall *Release_t)(void *unk);                                        /* 2 */
typedef HRESULT (__stdcall *GetRTData_t)(void *dev, void *rt, void *dst);               /* device 32 */
typedef HRESULT (__stdcall *CreateOffscreen_t)(void *dev, UINT w, UINT h, UINT fmt, UINT pool,
                                               void **surf, HANDLE *shared);            /* device 36 */
#define VT(obj, slot) ((*(void ***)(obj))[slot])

static void *source;            /* X360Stuff's target the ring was made for */
static SurfDesc source_desc;
static void *ring[RING];
static unsigned frame;
static void *locked;            /* the ring surface X360Stuff is reading */
static FILE *log_file;

static void klog(const char *text)
{
    if (log_file) { fputs(text, log_file); fputc('\n', log_file); fflush(log_file); }
}

static void drop_ring(void)
{
    for (int i = 0; i < RING; i++)
        if (ring[i]) { ((Release_t)VT(ring[i], 2))(ring[i]); ring[i] = NULL; }
    source = NULL;
}

static int make_ring(void *surf)
{
    SurfDesc d;
    char line[128];

    if (((GetDesc_t)VT(surf, 12))(surf, &d) != 0) return 0;
    if (surf == source && d.Width == source_desc.Width && d.Height == source_desc.Height &&
        d.Format == source_desc.Format)
        return 1;
    drop_ring();
    for (int i = 0; i < RING; i++)
        if (((CreateOffscreen_t)VT(DEVICE, 36))(DEVICE, d.Width, d.Height, d.Format, 2 /* SYSTEMMEM */,
                                                 &ring[i], NULL) != 0) {
            drop_ring();
            klog("could not create the system-memory copies: reading the target directly");
            return 0;
        }
    source = surf;
    source_desc = d;
    frame = 0;
    snprintf(line, sizeof(line), "luminance target %ux%u format %u: %d-frame ring", d.Width, d.Height,
             d.Format, RING);
    klog(line);
    return 1;
}

static HRESULT __stdcall luminance_lock(void *surf, void *rect, const RECT *r, DWORD flags)
{
    void *copy;

    if (!DEVICE || !make_ring(surf)) {
        locked = NULL;
        return ((LockRect_t)VT(surf, 13))(surf, rect, r, flags);
    }
    copy = ring[frame % RING];
    ((GetRTData_t)VT(DEVICE, 32))(DEVICE, surf, copy);
    /* Until the ring has filled, read this frame's copy (one wait each). */
    locked = frame < RING - 1 ? copy : ring[(frame + 1) % RING];
    frame++;
    return ((LockRect_t)VT(locked, 13))(locked, rect, r, flags);
}

static HRESULT __stdcall luminance_unlock(void *surf)
{
    void *target = locked ? locked : surf;
    locked = NULL;
    return ((UnlockRect_t)VT(target, 14))(target);
}

/* 0x10036787: mov eax,[ecx]; push ecx; call [eax+0x34]  ->  push ecx; call luminance_lock
 * 0x1003687e: mov ecx,[eax]; call [ecx+0x38]           ->  call luminance_unlock (eax pushed above) */
static int patch_x360stuff(void)
{
    static const unsigned char lock_site[] = { 0x8b, 0x01, 0x51, 0xff, 0x50, 0x34 };
    static const unsigned char unlock_site[] = { 0x8b, 0x08, 0xff, 0x51, 0x38 };
    unsigned char *lock_at = (unsigned char *)0x10036787, *unlock_at = (unsigned char *)0x1003687e;
    HMODULE x360 = GetModuleHandleA("X360Stuff.asi");
    DWORD old;

    if (!x360) return -1;                                           /* not loaded yet */
    if (x360 != (HMODULE)0x10000000) {
        klog("X360Stuff.asi is not at 0x10000000: not patched");
        return 0;
    }
    if (memcmp(lock_at, lock_site, sizeof(lock_site)) || memcmp(unlock_at, unlock_site, sizeof(unlock_site))) {
        klog("X360Stuff.asi is not the 4.2 build this was made for: not patched");
        return 0;
    }
    VirtualProtect(lock_at, 0x100, PAGE_EXECUTE_READWRITE, &old);
    lock_at[0] = 0x51;                                              /* push ecx */
    lock_at[1] = 0xe8;                                              /* call rel32 */
    *(int *)(lock_at + 2) = (int)luminance_lock - (int)(lock_at + 6);
    unlock_at[0] = 0xe8;
    *(int *)(unlock_at + 1) = (int)luminance_unlock - (int)(unlock_at + 5);
    VirtualProtect(lock_at, 0x100, old, &old);
    FlushInstructionCache(GetCurrentProcess(), lock_at, 0x100);
    klog("X360Stuff 4.2 luminance readback patched");
    return 1;
}

static DWORD WINAPI wait_for_x360stuff(LPVOID unused)
{
    (void)unused;
    for (int i = 0; i < 600; i++, Sleep(50))
        if (patch_x360stuff() >= 0) return 0;
    klog("X360Stuff.asi never loaded: not patched");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    char path[MAX_PATH], *dot;

    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(instance);
    GetModuleFileNameA(instance, path, sizeof(path));
    if ((dot = strrchr(path, '.'))) strcpy(dot, ".log");
    log_file = fopen(path, "w");
    /* The ASI loader's order is not fixed: if X360Stuff is not in yet, wait
     * for it on a thread (the game draws nothing for seconds after loading). */
    if (patch_x360stuff() < 0) CloseHandle(CreateThread(NULL, 0, wait_for_x360stuff, NULL, 0, NULL));
    return TRUE;
}
