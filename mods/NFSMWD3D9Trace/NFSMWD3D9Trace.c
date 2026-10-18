/*
 * NFSMW D3D9 Trace: which Direct3D 9 calls stall a frame, and who makes them.
 *
 * Diagnostic only. Under DXVK a call that needs GPU data now (a lock of a
 * resource the GPU still uses, GetRenderTargetData, a blocking query) makes
 * the game thread wait for DXVK's worker and the GPU: "CS syncs" on the HUD.
 * This wraps every such call, times it, and every 120 frames writes to
 * scripts/NFSMWD3D9Trace.log, per call kind: calls and ms per frame, calls
 * slower than 0.5 ms, and the return addresses (callers) of the slow ones.
 *
 * Hooks: the game's import of Direct3DCreate9 -> IDirect3D9::CreateDevice ->
 * the device's vtable (Present, GetRenderTargetData, GetFrontBufferData,
 * Create*) -> the first object of each resource kind's vtable (Lock/LockRect,
 * Query::GetData). Vtables are per class, so one patch covers all objects.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef HRESULT (__stdcall *Fn)();

enum { K_VB_LOCK, K_IB_LOCK, K_TEX_LOCK, K_CUBE_LOCK, K_SURF_LOCK, K_VOL_LOCK, K_GETRTDATA, K_GETFBDATA,
       K_QUERY_GETDATA, K_PRESENT, K_COUNT };
static const char *const kind_name[K_COUNT] = {
    "VB::Lock", "IB::Lock", "Tex::LockRect", "Cube::LockRect", "Surf::LockRect", "Vol::LockBox",
    "Dev::GetRenderTargetData", "Dev::GetFrontBufferData", "Query::GetData", "Dev::Present" };

#define MAX_CALLERS 32
typedef struct { void *caller; unsigned flags; unsigned count; double ms; } Caller;
typedef struct { unsigned calls, slow; double ms; Caller callers[MAX_CALLERS]; } Stat;
static Stat stats[K_COUNT];
static unsigned frames;
static double qpc_ms;
static FILE *log_file;

static double now_ms(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * qpc_ms;
}

static void record(int kind, double ms, void *caller, unsigned flags)
{
    Stat *s = &stats[kind];
    s->calls++;
    s->ms += ms;
    if (ms < 0.5) return;
    s->slow++;
    for (int i = 0; i < MAX_CALLERS; i++) {
        Caller *c = &s->callers[i];
        if (c->count == 0) { c->caller = caller; c->flags = flags; }
        if (c->caller == caller && c->flags == flags) { c->count++; c->ms += ms; return; }
    }
}

static void report(void)
{
    if (!log_file) return;
    fprintf(log_file, "--- %u frames\n", frames);
    for (int k = 0; k < K_COUNT; k++) {
        Stat *s = &stats[k];
        if (!s->calls) continue;
        fprintf(log_file, "%-26s %8.1f calls/f %7.3f ms/f  slow(>0.5ms) %6.2f/f\n", kind_name[k],
                (double)s->calls / frames, s->ms / frames, (double)s->slow / frames);
        for (int i = 0; i < MAX_CALLERS && s->callers[i].count; i++)
            fprintf(log_file, "    caller %p flags %08x  %6.2f/f  %7.3f ms/f\n", s->callers[i].caller,
                    s->callers[i].flags, (double)s->callers[i].count / frames, s->callers[i].ms / frames);
    }
    fflush(log_file);
    memset(stats, 0, sizeof(stats));
    frames = 0;
}

/* Replace vtable slot `slot` of `object`'s class with `hook`, once; returns
 * the original into *orig. */
static void patch(void *object, int slot, void *hook, Fn *orig)
{
    void **vtable = *(void ***)object;
    DWORD old;
    if (*orig || vtable[slot] == hook) return;
    *orig = (Fn)vtable[slot];
    VirtualProtect(&vtable[slot], sizeof(void *), PAGE_EXECUTE_READWRITE, &old);
    vtable[slot] = hook;
    VirtualProtect(&vtable[slot], sizeof(void *), old, &old);
}

#define CALLER __builtin_return_address(0)

/* ---- resources ---------------------------------------------------------- */

static Fn o_vb_lock, o_ib_lock, o_tex_lock, o_cube_lock, o_surf_lock, o_vol_lock, o_getdata;

static HRESULT __stdcall h_vb_lock(void *self, UINT off, UINT size, void **data, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_vb_lock(self, off, size, data, flags);
    record(K_VB_LOCK, now_ms() - t, CALLER, flags);
    return hr;
}
static HRESULT __stdcall h_ib_lock(void *self, UINT off, UINT size, void **data, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_ib_lock(self, off, size, data, flags);
    record(K_IB_LOCK, now_ms() - t, CALLER, flags);
    return hr;
}
static HRESULT __stdcall h_tex_lock(void *self, UINT level, void *rect, const RECT *r, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_tex_lock(self, level, rect, r, flags);
    record(K_TEX_LOCK, now_ms() - t, CALLER, flags);
    return hr;
}
static HRESULT __stdcall h_cube_lock(void *self, UINT face, UINT level, void *rect, const RECT *r, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_cube_lock(self, face, level, rect, r, flags);
    record(K_CUBE_LOCK, now_ms() - t, CALLER, flags);
    return hr;
}
static HRESULT __stdcall h_surf_lock(void *self, void *rect, const RECT *r, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_surf_lock(self, rect, r, flags);
    record(K_SURF_LOCK, now_ms() - t, CALLER, flags);
    return hr;
}
static HRESULT __stdcall h_vol_lock(void *self, UINT level, void *box, const void *b, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_vol_lock(self, level, box, b, flags);
    record(K_VOL_LOCK, now_ms() - t, CALLER, flags);
    return hr;
}
static HRESULT __stdcall h_getdata(void *self, void *data, DWORD size, DWORD flags)
{
    double t = now_ms();
    HRESULT hr = o_getdata(self, data, size, flags);
    record(K_QUERY_GETDATA, now_ms() - t, CALLER, flags | (hr == S_FALSE ? 0x80000000u : 0));
    return hr;
}

/* IDirect3DTexture9: LockRect 19, GetSurfaceLevel 18. IDirect3DCubeTexture9:
 * LockRect 19, GetCubeMapSurface 18. IDirect3DSurface9: LockRect 13.
 * IDirect3DVolumeTexture9: LockBox 19. Vertex/IndexBuffer9: Lock 11.
 * IDirect3DQuery9: GetData 7. */
static Fn o_get_surface_level, o_get_cube_surface;
static HRESULT __stdcall h_get_surface_level(void *self, UINT level, void **surface)
{
    HRESULT hr = o_get_surface_level(self, level, surface);
    if (hr == 0 && *surface) patch(*surface, 13, h_surf_lock, &o_surf_lock);
    return hr;
}
static HRESULT __stdcall h_get_cube_surface(void *self, UINT face, UINT level, void **surface)
{
    HRESULT hr = o_get_cube_surface(self, face, level, surface);
    if (hr == 0 && *surface) patch(*surface, 13, h_surf_lock, &o_surf_lock);
    return hr;
}

/* ---- device ------------------------------------------------------------- */

/* IDirect3DDevice9 slots: Present 17, GetBackBuffer 18, CreateTexture 23,
 * CreateVolumeTexture 24, CreateCubeTexture 25, CreateVertexBuffer 26,
 * CreateIndexBuffer 27, CreateRenderTarget 28, CreateDepthStencilSurface 29,
 * GetRenderTargetData 32, GetFrontBufferData 33, CreateOffscreenPlainSurface 36,
 * GetRenderTarget 38, GetDepthStencilSurface 39, CreateQuery 118. */
static Fn o_present, o_create_texture, o_create_volume, o_create_cube, o_create_vb, o_create_ib,
          o_create_rt, o_create_ds, o_getrtdata, o_getfbdata, o_create_offscreen, o_create_query;

static HRESULT __stdcall h_present(void *self, const RECT *a, const RECT *b, HWND w, const void *d)
{
    double t = now_ms();
    HRESULT hr = o_present(self, a, b, w, d);
    record(K_PRESENT, now_ms() - t, CALLER, 0);
    if (++frames >= 120) report();
    return hr;
}
static HRESULT __stdcall h_getrtdata(void *self, void *rt, void *dst)
{
    double t = now_ms();
    HRESULT hr = o_getrtdata(self, rt, dst);
    record(K_GETRTDATA, now_ms() - t, CALLER, 0);
    return hr;
}
static HRESULT __stdcall h_getfbdata(void *self, UINT chain, void *dst)
{
    double t = now_ms();
    HRESULT hr = o_getfbdata(self, chain, dst);
    record(K_GETFBDATA, now_ms() - t, CALLER, 0);
    return hr;
}
static HRESULT __stdcall h_create_texture(void *self, UINT w, UINT h, UINT l, DWORD u, DWORD f, DWORD p,
                                          void **out, HANDLE *s)
{
    HRESULT hr = o_create_texture(self, w, h, l, u, f, p, out, s);
    if (hr == 0 && *out) {
        patch(*out, 19, h_tex_lock, &o_tex_lock);
        patch(*out, 18, h_get_surface_level, &o_get_surface_level);
    }
    return hr;
}
static HRESULT __stdcall h_create_volume(void *self, UINT w, UINT h, UINT d, UINT l, DWORD u, DWORD f,
                                         DWORD p, void **out, HANDLE *s)
{
    HRESULT hr = o_create_volume(self, w, h, d, l, u, f, p, out, s);
    if (hr == 0 && *out) patch(*out, 19, h_vol_lock, &o_vol_lock);
    return hr;
}
static HRESULT __stdcall h_create_cube(void *self, UINT e, UINT l, DWORD u, DWORD f, DWORD p, void **out,
                                       HANDLE *s)
{
    HRESULT hr = o_create_cube(self, e, l, u, f, p, out, s);
    if (hr == 0 && *out) {
        patch(*out, 19, h_cube_lock, &o_cube_lock);
        patch(*out, 18, h_get_cube_surface, &o_get_cube_surface);
    }
    return hr;
}
static HRESULT __stdcall h_create_vb(void *self, UINT len, DWORD u, DWORD fvf, DWORD p, void **out, HANDLE *s)
{
    HRESULT hr = o_create_vb(self, len, u, fvf, p, out, s);
    if (hr == 0 && *out) patch(*out, 11, h_vb_lock, &o_vb_lock);
    return hr;
}
static HRESULT __stdcall h_create_ib(void *self, UINT len, DWORD u, DWORD f, DWORD p, void **out, HANDLE *s)
{
    HRESULT hr = o_create_ib(self, len, u, f, p, out, s);
    if (hr == 0 && *out) patch(*out, 11, h_ib_lock, &o_ib_lock);
    return hr;
}
static HRESULT __stdcall h_create_rt(void *self, UINT w, UINT h, DWORD f, DWORD ms, DWORD q, BOOL lock,
                                     void **out, HANDLE *s)
{
    HRESULT hr = o_create_rt(self, w, h, f, ms, q, lock, out, s);
    if (hr == 0 && *out) patch(*out, 13, h_surf_lock, &o_surf_lock);
    return hr;
}
static HRESULT __stdcall h_create_ds(void *self, UINT w, UINT h, DWORD f, DWORD ms, DWORD q, BOOL d,
                                     void **out, HANDLE *s)
{
    HRESULT hr = o_create_ds(self, w, h, f, ms, q, d, out, s);
    if (hr == 0 && *out) patch(*out, 13, h_surf_lock, &o_surf_lock);
    return hr;
}
static HRESULT __stdcall h_create_offscreen(void *self, UINT w, UINT h, DWORD f, DWORD p, void **out, HANDLE *s)
{
    HRESULT hr = o_create_offscreen(self, w, h, f, p, out, s);
    if (hr == 0 && *out) patch(*out, 13, h_surf_lock, &o_surf_lock);
    return hr;
}
static HRESULT __stdcall h_create_query(void *self, DWORD type, void **out)
{
    HRESULT hr = o_create_query(self, type, out);
    if (hr == 0 && out && *out) patch(*out, 7, h_getdata, &o_getdata);
    return hr;
}

static void hook_device(void *dev)
{
    patch(dev, 17, h_present, &o_present);
    patch(dev, 23, h_create_texture, &o_create_texture);
    patch(dev, 24, h_create_volume, &o_create_volume);
    patch(dev, 25, h_create_cube, &o_create_cube);
    patch(dev, 26, h_create_vb, &o_create_vb);
    patch(dev, 27, h_create_ib, &o_create_ib);
    patch(dev, 28, h_create_rt, &o_create_rt);
    patch(dev, 29, h_create_ds, &o_create_ds);
    patch(dev, 32, h_getrtdata, &o_getrtdata);
    patch(dev, 33, h_getfbdata, &o_getfbdata);
    patch(dev, 36, h_create_offscreen, &o_create_offscreen);
    patch(dev, 118, h_create_query, &o_create_query);
    if (log_file) { fprintf(log_file, "device %p hooked\n", dev); fflush(log_file); }
}

/* IDirect3D9::CreateDevice is slot 16. */
static Fn o_create_device;
static HRESULT __stdcall h_create_device(void *self, UINT a, DWORD t, HWND w, DWORD f, void *pp, void **dev)
{
    HRESULT hr = o_create_device(self, a, t, w, f, pp, dev);
    if (hr == 0 && dev && *dev) hook_device(*dev);
    return hr;
}

typedef void *(__stdcall *Create9)(UINT);
static Create9 o_create9;
static void *__stdcall h_create9(UINT sdk)
{
    void *d3d = o_create9(sdk);
    if (d3d) patch(d3d, 16, h_create_device, &o_create_device);
    return d3d;
}

/* Point the game's import of d3d9!Direct3DCreate9 at h_create9. */
static int hook_import(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress);
    for (; imp->Name; imp++) {
        if (_stricmp((char *)base + imp->Name, "d3d9.dll")) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(base + imp->OriginalFirstThunk);
        IMAGE_THUNK_DATA *iat = (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++) {
            if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
            IMAGE_IMPORT_BY_NAME *n = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
            if (strcmp((char *)n->Name, "Direct3DCreate9")) continue;
            DWORD old;
            o_create9 = (Create9)iat->u1.Function;
            VirtualProtect(&iat->u1.Function, sizeof(void *), PAGE_READWRITE, &old);
            iat->u1.Function = (DWORD_PTR)h_create9;
            VirtualProtect(&iat->u1.Function, sizeof(void *), old, &old);
            return 1;
        }
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    char path[MAX_PATH], *dot;
    LARGE_INTEGER f;

    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(instance);
    QueryPerformanceFrequency(&f);
    qpc_ms = 1000.0 / (double)f.QuadPart;
    GetModuleFileNameA(instance, path, sizeof(path));
    if ((dot = strrchr(path, '.'))) strcpy(dot, ".log");
    log_file = fopen(path, "w");
    if (log_file) { fprintf(log_file, "import hook: %s\n", hook_import() ? "ok" : "d3d9!Direct3DCreate9 not imported"); fflush(log_file); }
    return TRUE;
}
