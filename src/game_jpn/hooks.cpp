// Hooks for default_jpn.xbe
#include "cpu/x86_ops.h"
#include "core/log.h"

X86_FUNC(sub_000EF960) {
    ctx.esp += 4;
}

X86_FUNC(sub_000EFAA0) {
    ctx.esp += 4;
}

X86_FUNC(sub_000EB640) {
    ctx.eax = 0;
    ctx.esp += 4;
}
