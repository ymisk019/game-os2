/* Host-side test of kernel/isr.S. We fake what the CPU does on an exception
 * (align RSP, push SS/RSP/RFLAGS/CS/RIP [+ error code]) and jump into the real
 * stubs. Same-privilege iretq works in user mode, so the whole save/restore path
 * is exercised: struct layout, 16-byte alignment, register restore, modification
 * of saved registers, and vector/error-code normalisation. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "regs.h"

uint64_t saved_rsp, saved_cs, saved_ss;
uint64_t after[15];          /* regs after return: rax rbx rcx rdx rsi rdi rbp r8..r15 */
regs_t   seen;
int      aligned_ok, calls;
extern char ret_point3[], ret_point14[];

void exception_handler(regs_t *r) {
    calls++;
    seen = *r;
    aligned_ok = (((uintptr_t)__builtin_frame_address(0)) & 15) == 0;
    r->rax = 0xDEAD;                         /* prove modifications are restored */
}

#define LOAD_REGS \
    "mov rax, 0xA1\n mov rbx, 0xB2\n mov rcx, 0xC3\n mov rdx, 0xD4\n" \
    "mov rsi, 0x51\n mov rdi, 0xD1\n mov rbp, 0xBB\n mov r8, 0x08\n mov r9, 0x09\n" \
    "mov r10, 0x10\n mov r11, 0x11\n mov r12, 0x12\n mov r13, 0x13\n mov r14, 0x14\n mov r15, 0x15\n"

#define SAVE_REGS \
    "mov [rip+after+0], rax\n mov [rip+after+8], rbx\n mov [rip+after+16], rcx\n mov [rip+after+24], rdx\n" \
    "mov [rip+after+32], rsi\n mov [rip+after+40], rdi\n mov [rip+after+48], rbp\n mov [rip+after+56], r8\n" \
    "mov [rip+after+64], r9\n mov [rip+after+72], r10\n mov [rip+after+80], r11\n mov [rip+after+88], r12\n" \
    "mov [rip+after+96], r13\n mov [rip+after+104], r14\n mov [rip+after+112], r15\n"

__asm__(
".intel_syntax noprefix\n"
".text\n"
".global run_trap3\n"
"run_trap3:\n"
"  push rbx\n push rbp\n push r12\n push r13\n push r14\n push r15\n"
"  mov [rip+saved_rsp], rsp\n"
   LOAD_REGS
"  and rsp, -16\n"
"  push qword ptr [rip+saved_ss]\n push qword ptr [rip+saved_rsp]\n pushfq\n"
"  push qword ptr [rip+saved_cs]\n push offset ret_point3\n"
"  jmp isr3\n"
".global ret_point3\n"
"ret_point3:\n"
   SAVE_REGS
"  pop r15\n pop r14\n pop r13\n pop r12\n pop rbp\n pop rbx\n"
"  ret\n"

".global run_trap14\n"
"run_trap14:\n"
"  push rbx\n push rbp\n push r12\n push r13\n push r14\n push r15\n"
"  mov [rip+saved_rsp], rsp\n"
   LOAD_REGS
"  and rsp, -16\n"
"  push qword ptr [rip+saved_ss]\n push qword ptr [rip+saved_rsp]\n pushfq\n"
"  push qword ptr [rip+saved_cs]\n push offset ret_point14\n"
"  push 0x1234\n"                              /* error code pushed by the CPU */
"  jmp isr14\n"
".global ret_point14\n"
"ret_point14:\n"
   SAVE_REGS
"  pop r15\n pop r14\n pop r13\n pop r12\n pop rbp\n pop rbx\n"
"  ret\n"
".att_syntax prefix\n"
);
/* 'after' lives in C; give the asm a symbol with a stable name */
__asm__(".intel_syntax noprefix\n.global after_sym\n.att_syntax prefix\n");
extern void run_trap3(void), run_trap14(void);

static int fails;
#define CHECK(c, msg) do { if (!(c)) { printf("  FAIL: %s\n", msg); fails++; } else printf("  ok:   %s\n", msg); } while (0)

int main(void) {
    __asm__ volatile("mov %%cs, %0" : "=r"(saved_cs));
    __asm__ volatile("mov %%ss, %0" : "=r"(saved_ss));

    printf("Test 1: vector 3 (no CPU error code)\n");
    memset(after, 0, sizeof after);
    run_trap3();
    CHECK(calls == 1, "handler called once");
    CHECK(seen.vector == 3 && seen.error == 0, "vector=3, dummy error=0");
    CHECK(seen.rip == (uint64_t)(uintptr_t)ret_point3, "RIP = return point");
    CHECK(seen.cs == saved_cs && seen.ss == saved_ss, "CS/SS match");
    CHECK(seen.rsp == saved_rsp, "RSP matches");
    CHECK(seen.rax==0xA1 && seen.rbx==0xB2 && seen.rcx==0xC3 && seen.rdx==0xD4, "rax-rdx seen correctly");
    CHECK(seen.rsi==0x51 && seen.rdi==0xD1 && seen.rbp==0xBB, "rsi/rdi/rbp seen correctly");
    CHECK(seen.r8==8 && seen.r9==9 && seen.r10==0x10 && seen.r11==0x11, "r8-r11 seen correctly");
    CHECK(seen.r12==0x12 && seen.r13==0x13 && seen.r14==0x14 && seen.r15==0x15, "r12-r15 seen correctly");
    CHECK(aligned_ok, "RSP 16-byte aligned at handler call");
    CHECK(after[0] == 0xDEAD, "handler's change to rax restored");
    CHECK(after[1]==0xB2 && after[2]==0xC3 && after[3]==0xD4 && after[4]==0x51 && after[5]==0xD1 && after[6]==0xBB, "rbx..rbp preserved");
    CHECK(after[7]==8 && after[8]==9 && after[9]==0x10 && after[10]==0x11, "r8-r11 preserved");
    CHECK(after[11]==0x12 && after[12]==0x13 && after[13]==0x14 && after[14]==0x15, "r12-r15 preserved");

    printf("Test 2: vector 14 (CPU pushes error code 0x1234)\n");
    memset(after, 0, sizeof after);
    run_trap14();
    CHECK(calls == 2, "handler called again");
    CHECK(seen.vector == 14 && seen.error == 0x1234, "vector=14, error=0x1234");
    CHECK(seen.rip == (uint64_t)(uintptr_t)ret_point14, "RIP = return point");
    CHECK(seen.rsp == saved_rsp, "RSP matches");
    CHECK(aligned_ok, "RSP 16-byte aligned at handler call");
    CHECK(after[0] == 0xDEAD && after[1] == 0xB2 && after[14] == 0x15, "registers restored");

    printf(fails ? "\nRESULT: %d FAILURE(S)\n" : "\nRESULT: ALL PASSED\n", fails);
    return fails != 0;
}
