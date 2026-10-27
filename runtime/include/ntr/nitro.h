/*
 * Generic replacements for NitroSDK routines, used as recompiler hooks (game/config/<code>/hooks.txt).
 * Hooks that need game-specific addresses are wrapped by the game's own glue code.
 */
#ifndef NTR_NITRO_H
#define NTR_NITRO_H

#include "ntr/recomp.h"

#ifdef __cplusplus
extern "C" {
#endif

void ntr_nitro_save_context(NtrCpu* c); /* OS_SaveContext */
void ntr_nitro_load_context(NtrCpu* c); /* OS_LoadContext */
void ntr_nitro_init_context(NtrCpu* c); /* OS_InitContext */

/* DS Protect detection checks ("flashcart", "emulator", ...): report nothing detected, as on a genuine cartridge. */
void ntr_dsprot_not_detected(NtrCpu* c);
/* A game's own stack-switching task (coroutine) routines, as in Magician's Quest. Context: +0 pc, +4 sp, +8 r0.
   ntr_task_swap(save, load): saves the caller into `save` and continues `load`. ntr_task_load(_, load): continues
   `load`, abandoning the caller. Both keep r4-r11 in a 64-byte frame on the task's stack, as the originals do. */
void ntr_task_swap(NtrCpu* c);
/* OS_SpinWait(cycles): spends exactly that many ARM9 cycles (the recompiled loop would count fewer). */
void ntr_os_spin_wait(NtrCpu* c);
void ntr_task_load(NtrCpu* c);

#ifdef __cplusplus
}

namespace ntr::nitro {
/* OS_IrqHandler, given the addresses of OS_IRQTable and OS_IrqHandler_ThreadSwitch. */
void irq_handler(NtrCpu* c, uint32_t irq_table, uint32_t thread_switch);

/* CARDi_RequestStreamCommand, given the address of the card library's common state (cardi_common): a synchronous
   backup read, write or verify, done on the save memory at once. Returns false (nothing done) for any other request,
   which then has to run as usual. */
bool card_backup_request(NtrCpu* c, uint32_t common);

/* Called after each write of the save memory (its offset and length: a synchronous request above, or a page the
   emulated card's own path programmed), before the host is handed the save to write to disk: what the hook adds to
   the save memory reaches the disk in the same write as the game's own bytes. One hook (nullptr: none); `c` is null
   for the card's own path. */
using BackupWriteHook = void (*)(NtrCpu* c, uint32_t offset, uint32_t length, uint8_t* save, uint32_t size);
void set_backup_write_hook(BackupWriteHook hook);
void backup_written(NtrCpu* c, uint32_t offset, uint32_t length, uint8_t* save, uint32_t size);
/* The save memory (the save file's content as the game last wrote it) and its size; nullptr if there is none. */
uint8_t* backup_memory(NtrCpu* c, uint32_t* size);
}  // namespace ntr::nitro
#endif

#endif
