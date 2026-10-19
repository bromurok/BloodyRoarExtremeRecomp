// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.
#include "cpu/x86_ops.h"
#include "core/log.h"

// In original Xbox XDK D3D, sub_000EF960 is an internal GPU pushbuffer FIFO flush that
// spins on NV2A MMIO register bit [eax+0x100410] & 0x10000.
// On PC recompilation there is no NV2A GPU to clear this bit, so we stub it out.
X86_FUNC(sub_000EF960) {
    ctx.esp += 4;
}

// sub_000EFAA0 is CDevice::KickOff(wait=true) which flushes and spins on sub_000EB640.
X86_FUNC(sub_000EFAA0) {
    ctx.esp += 4;
}

// sub_000EB640 checks if the NV2A GPU is busy (returns 1 if busy, 0 if idle).
// With HLE D3D12, GPU commands are synchronous/pipelined by the host, so always report idle (0).
X86_FUNC(sub_000EB640) {
    ctx.eax = 0;
    ctx.esp += 4;
}

extern "C" void __imp__sub_00016C50(X86Context& ctx, uint8_t* base);
X86_FUNC(sub_00016C50) {
    static uint32_t count = 0;
    if (++count <= 10) {
        LOGI("game: sub_00016C50 (ClearScreen) called from %08X, arg=%08X",
             X86_MEM32(ctx.esp), X86_MEM32(ctx.esp + 4));
    }
    __imp__sub_00016C50(ctx, base);
}
