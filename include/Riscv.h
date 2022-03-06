#ifndef _RISCV_H_
#define _RISCV_H_

#include <Type.h>

#define HART_TOTAL_NUMBER 4

// which hart (core) is this?
static inline u64 r_mhartid() {
    u64 x;
    asm volatile("mv %0, tp" : "=r" (x) );
    return x;
}

// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.

static inline u64 r_mstatus() {
  u64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void w_mstatus(u64 x) {
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_mepc(u64 x) {
  asm volatile("csrw mepc, %0" : : "r" (x));
}

// Supervisor Status Register, sstatus

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

static inline u64 r_sstatus() {
    u64 x;
    asm volatile("csrr %0, sstatus" : "=r" (x) );
    return x;
}

static inline void w_sstatus(u64 x) {
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

// Supervisor Interrupt Pending
static inline u64 r_sip() {
    u64 x;
    asm volatile("csrr %0, sip" : "=r" (x) );
    return x;
}

static inline void w_sip(u64 x) {
    asm volatile("csrw sip, %0" : : "r" (x));
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software
static inline u64 r_sie() {
    u64 x;
    asm volatile("csrr %0, sie" : "=r" (x) );
    return x;
}

static inline void w_sie(u64 x) {
	asm volatile("csrw sie, %0" : : "r" (x));
}

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software
static inline u64 r_mie() {
    u64 x;
    asm volatile("csrr %0, mie" : "=r" (x) );
    return x;
}

static inline void w_mie(u64 x) {
	asm volatile("csrw mie, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_sepc(u64 x) {
  	asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline u64 r_sepc() {
    u64 x;
    asm volatile("csrr %0, sepc" : "=r" (x) );
	return x;
}

// Machine Exception Delegation
static inline u64 r_medeleg() {
    u64 x;
    asm volatile("csrr %0, medeleg" : "=r" (x) );
    return x;
}

static inline void w_medeleg(u64 x) {
	asm volatile("csrw medeleg, %0" : : "r" (x));
}

// Machine Interrupt Delegation
static inline u64 r_mideleg() {
    u64 x;
    asm volatile("csrr %0, mideleg" : "=r" (x) );
    return x;
}

static inline void w_mideleg(u64 x) {
	asm volatile("csrw mideleg, %0" : : "r" (x));
}

// Supervisor Trap-Vector Base Address
// low two bits are mode.
static inline void w_stvec(u64 x) {
	asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline u64 r_stvec() {
	u64 x;
	asm volatile("csrr %0, stvec" : "=r" (x) );
	return x;
}

// Machine-mode interrupt vector
static inline void w_mtvec(u64 x) {
	asm volatile("csrw mtvec, %0" : : "r" (x));
}

// use riscv's sv39 page table scheme.
#define SATP_SV39 (8L << 60)

#define MAKE_SATP(pagetable) (SATP_SV39 | (((u64)pagetable) >> 12))

// supervisor address translation and protection;
// holds the address of the page table.
static inline void w_satp(u64 x) {
	asm volatile("csrw satp, %0" : : "r" (x));
}

static inline u64 r_satp() {
	u64 x;
	asm volatile("csrr %0, satp" : "=r" (x) );
	return x;
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void w_sscratch(u64 x) {
	asm volatile("csrw sscratch, %0" : : "r" (x));
}

static inline void w_mscratch(u64 x) {
	asm volatile("csrw mscratch, %0" : : "r" (x));
}

// Supervisor Trap Cause
static inline u64 r_scause() {
	u64 x;
	asm volatile("csrr %0, scause" : "=r" (x) );
	return x;
}

// Supervisor Trap Value
static inline u64 r_stval() {
	u64 x;
	asm volatile("csrr %0, stval" : "=r" (x) );
	return x;
}

// Machine-mode Counter-Enable
static inline void w_mcounteren(u64 x) {
	asm volatile("csrw mcounteren, %0" : : "r" (x));
}

static inline u64 r_mcounteren() {
	u64 x;
	asm volatile("csrr %0, mcounteren" : "=r" (x) );
	return x;
}

// supervisor-mode cycle counter
static inline u64 r_time() {
	u64 x;
  	// asm volatile("csrr %0, time" : "=r" (x) );
  	// this instruction will trap in SBI
	asm volatile("rdtime %0" : "=r" (x) );
	return x;
}

// enable device interrupts
static inline void intr_on() {
	w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void intr_off() {
	w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

// are device interrupts enabled?
static inline bool intr_get() {
	u64 x = r_sstatus();
	return (x & SSTATUS_SIE) != 0;
}

static inline u64 r_sp() {
	u64 x;
	asm volatile("mv %0, sp" : "=r" (x) );
	return x;
}

// read and write tp, the thread pointer, which holds
// this core's hartid (core number), the index into cpus[].
static inline u64 r_tp() {
	u64 x;
	asm volatile("mv %0, tp" : "=r" (x) );
	return x;
}

static inline void w_tp(u64 x) {
	asm volatile("mv tp, %0" : : "r" (x));
}

static inline u64 r_ra() {
	u64 x;
	asm volatile("mv %0, ra" : "=r" (x) );
	return x;
}

static inline u64 r_fp() {
	u64 x;
	asm volatile("mv %0, s0" : "=r" (x) );
	return x;
}

// flush the TLB.
static inline void sfence_vma() {
	// the zero, zero means flush all TLB entries.
	// asm volatile("sfence.vma zero, zero");
	asm volatile("sfence.vma");
}

#endif