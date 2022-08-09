#ifndef _MEMORY_CONFIG_H_
#define _MEMORY_CONFIG_H_

#define VIRT_OFFSET             0x3F00000000L
#define UART                    0x10000000L
#define UART_V                  (UART + VIRT_OFFSET)

// virtio mmio interface
#define VIRTIO0                 0x10001000
#define VIRTIO0_V               (VIRTIO0 + VIRT_OFFSET)

// local interrupt controller, which contains the timer.
#define CLINT                   0x02000000L
#define CLINT_V                 (CLINT + VIRT_OFFSET)

#define PLIC                    0x0c000000L
#define PLIC_V                  (PLIC + VIRT_OFFSET)

#define PLIC_PRIORITY           (PLIC_V + 0x0)
#define PLIC_PENDING            (PLIC_V + 0x1000)
#define PLIC_MENABLE(hart)      (PLIC_V + 0x2000 + (hart) * 0x100)
#define PLIC_SENABLE(hart)      (PLIC_V + 0x2080 + (hart) * 0x100)
#define PLIC_MPRIORITY(hart)    (PLIC_V + 0x200000 + (hart) * 0x2000)
#define PLIC_SPRIORITY(hart)    (PLIC_V + 0x201000 + (hart) * 0x2000)
#define PLIC_MCLAIM(hart)       (PLIC_V + 0x200004 + (hart) * 0x2000)
#define PLIC_SCLAIM(hart)       (PLIC_V + 0x201004 + (hart) * 0x2000)


#define PHYSICAL_ADDRESS_BASE 0x80000000UL
#define PHYSICAL_MEMORY_SIZE 0x08000000UL
#define PHYSICAL_MEMORY_TOP (PHYSICAL_ADDRESS_BASE + PHYSICAL_MEMORY_SIZE)
#define KERNEL_STACK_SIZE 0x10000

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PHYSICAL_PAGE_NUM (PHYSICAL_MEMORY_SIZE >> PAGE_SHIFT)

#define KERNEL_PROCESS_SP_TOP (1UL << 36)

#define PROCESS_SIGNAL_BASE (KERNEL_PROCESS_SP_TOP + (1UL << 30))
#define SOCKET_BUFFER_BASE (PROCESS_SIGNAL_BASE + (1UL << 30))
#define SIGNAL_HANDLER_SP_BASE (SOCKET_BUFFER_BASE + (1UL << 30))
#define FILE_SYSTEM_CLUSTER_BITMAP_BASE (SIGNAL_HANDLER_SP_BASE + (1UL << 30))

#define KERNEL_ADDRESS_TOP (1UL << 38)
#define TRAMPOLINE_BASE (KERNEL_ADDRESS_TOP - (PAGE_SIZE << 1))
#define SIGNAL_TRAMPOLINE_BASE (TRAMPOLINE_BASE - PAGE_SIZE)
#define USER_STACK_TOP SIGNAL_TRAMPOLINE_BASE
#define USER_STACK_BOTTOM (USER_STACK_TOP - (1UL << 30))

//New, Heap Address, this limit User Stack Space to 2^32 Bytes
#define USER_HEAP_TOP USER_STACK_BOTTOM
#define USER_HEAP_BOTTOM (USER_HEAP_TOP - (1UL << 32))

#define USER_BUFFER_BASE 0x01000000
#endif