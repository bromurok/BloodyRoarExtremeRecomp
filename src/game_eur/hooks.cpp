// Hooks for default_eur.xbe (+0xFA0 offset from USA release)
#include "cpu/x86_ops.h"
#include "core/log.h"

X86_FUNC(sub_000F0900) {
    ctx.esp += 4;
}

X86_FUNC(sub_000F1440) {
    ctx.esp += 4;
}

X86_FUNC(sub_000EC5E0) {
    ctx.eax = 0;
    ctx.esp += 4;
}
