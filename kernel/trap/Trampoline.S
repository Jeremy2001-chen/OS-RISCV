#include <assembly/Trapframe.h>
	#
        # code to switch between user and kernel space.
        #
        # this code is mapped at the same virtual address
        # (TRAMPOLINE) in user and kernel space so that
        # it continues to work when it switches page tables.
	#
	# kernel.ld causes this to be aligned
        # to a page boundary.
        #
	.section .trampoline
    .globl trampoline
trampoline:
    .align 4
    .globl userVector
userVector:
	#
    # trap.c sets stvec to point here, so
    # traps from user space start here,
    # in supervisor mode, but with a
    # user page table.
    #
    # sscratch points to where the process's p->trapframe is
    # mapped into user space, at TRAPFRAME.
    #
        
	# swap a0 and sscratch
    # so that a0 is TRAPFRAME
    csrrw a0, sscratch, a0

    # save the user registers in TRAPFRAME
    sd ra, REG_RA(a0)
    sd sp, REG_SP(a0)
    sd gp, REG_GP(a0)
    sd tp, REG_TP(a0)
    sd t0, REG_T0(a0)
    sd t1, REG_T1(a0)
    sd t2, REG_T2(a0)
    sd s0, REG_S0(a0)
    sd s1, REG_S1(a0)
    sd a1, REG_A1(a0)
    sd a2, REG_A2(a0)
    sd a3, REG_A3(a0)
    sd a4, REG_A4(a0)
    sd a5, REG_A5(a0)
    sd a6, REG_A6(a0)
    sd a7, REG_A7(a0)
    sd s2, REG_S2(a0)
    sd s3, REG_S3(a0)
    sd s4, REG_S4(a0)
    sd s5, REG_S5(a0)
    sd s6, REG_S6(a0)
    sd s7, REG_S7(a0)
    sd s8, REG_S8(a0)
    sd s9, REG_S9(a0)
    sd s10, REG_S10(a0)
    sd s11, REG_S11(a0)
    sd t3, REG_T3(a0)
    sd t4, REG_T4(a0)
    sd t5, REG_T5(a0)
    sd t6, REG_T6(a0)

	# save the user a0 in p->trapframe->a0
    csrr t0, sscratch
    sd t0, REG_A0(a0)
    # restore kernel stack pointer from p->trapframe->kernel_sp
    ld sp, KERNEL_SP(a0)
    # make tp hold the current hartid, from p->trapframe->kernel_hartid
    ld tp, KERNEL_HART_ID(a0)
    # load the address of usertrap(), p->trapframe->kernel_trap
    ld t0, TRAP_HANDLER(a0)
    csrr t1, sepc
    sd t1, EPC(a0)
    # restore kernel page table from p->trapframe->kernel_satp
    ld t1, KERNEL_SATP(a0)
    csrw satp, t1

    # sfence.vma zero, zero
    sfence.vma
    # a0 is no longer valid, since the kernel page
    # table does not specially map p->tf.
    # jump to usertrap(), which does not return
    jr t0

.globl userReturn
userReturn:
    # userret(TRAPFRAME, pagetable)
    # switch from kernel to user.
    # usertrapret() calls here.
    # a0: TRAPFRAME, in user page table.
    # a1: user page table, for satp.
    # switch to the user page table.

    csrw satp, a1
    # sfence.vma zero, zero
    sfence.vma

    # put the saved user a0 in sscratch, so we
    # can swap it with our a0 (TRAPFRAME) in the last step.
    ld t0, REG_A0(a0)
    csrw sscratch, t0
    ld t1, EPC(a0)
    csrw sepc, t1
    # restore all but a0 from TRAPFRAME
    ld ra, REG_RA(a0)
    ld sp, REG_SP(a0)
    ld gp, REG_GP(a0)
    ld tp, REG_TP(a0)
    ld t0, REG_T0(a0)
    ld t1, REG_T1(a0)
    ld t2, REG_T2(a0)
    ld s0, REG_S0(a0)
    ld s1, REG_S1(a0)
    ld a1, REG_A1(a0)
    ld a2, REG_A2(a0)
    ld a3, REG_A3(a0)
    ld a4, REG_A4(a0)
    ld a5, REG_A5(a0)
    ld a6, REG_A6(a0)
    ld a7, REG_A7(a0)
    ld s2, REG_S2(a0)
    ld s3, REG_S3(a0)
    ld s4, REG_S4(a0)
    ld s5, REG_S5(a0)
    ld s6, REG_S6(a0)
    ld s7, REG_S7(a0)
    ld s8, REG_S8(a0)
    ld s9, REG_S9(a0)
    ld s10, REG_S10(a0)
    ld s11, REG_S11(a0)
    ld t3, REG_T3(a0)
    ld t4, REG_T4(a0)
    ld t5, REG_T5(a0)
    ld t6, REG_T6(a0)
	# restore user a0, and save TRAPFRAME in sscratch
    csrrw a0, sscratch, a0
    # return to user mode and user pc.
    # usertrapret() set up sstatus and sepc.
    sret

    .globl trapframe
trapframe:
    .space 300
