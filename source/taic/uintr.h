#ifndef _UINTR_H
#define _UINTR_H

#include <stdint.h>

#ifdef __ASSEMBLER__
#define __ASM_STR(x) x
#else
#define __ASM_STR(x) #x
#endif

#define csr_swap(csr, val)                                  \
	({                                                        \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrw %0, " __ASM_STR(csr) ", %1" \
												 : "=r"(__v)                        \
												 : "rK"(__v)                        \
												 : "memory");                       \
		__v;                                                    \
	})

#define csr_read(csr)                                                          \
	({                                                                           \
		register unsigned long __v;                                                \
		__asm__ __volatile__("csrr %0, " __ASM_STR(csr) : "=r"(__v) : : "memory"); \
		__v;                                                                       \
	})

#define csr_write(csr, val)                            \
	({                                                   \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrw " __ASM_STR(csr) ", %0" \
												 :                             \
												 : "rK"(__v)                   \
												 : "memory");                  \
	})

#define csr_read_set(csr, val)                              \
	({                                                        \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrs %0, " __ASM_STR(csr) ", %1" \
												 : "=r"(__v)                        \
												 : "rK"(__v)                        \
												 : "memory");                       \
		__v;                                                    \
	})

#define csr_set(csr, val)                              \
	({                                                   \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrs " __ASM_STR(csr) ", %0" \
												 :                             \
												 : "rK"(__v)                   \
												 : "memory");                  \
	})

#define csr_read_clear(csr, val)                            \
	({                                                        \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrc %0, " __ASM_STR(csr) ", %1" \
												 : "=r"(__v)                        \
												 : "rK"(__v)                        \
												 : "memory");                       \
		__v;                                                    \
	})

#define csr_clear(csr, val)                            \
	({                                                   \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrc " __ASM_STR(csr) ", %0" \
												 :                             \
												 : "rK"(__v)                   \
												 : "memory");                  \
	})

/* User Trap Setup */
#define CSR_USTATUS 0x000
#define CSR_UIE 0x004
#define CSR_UTVEC 0x005

/* User Trap Handling */
#define CSR_USCRATCH 0x040
#define CSR_UEPC 0x041
#define CSR_UCAUSE 0x042
#define CSR_UTVAL 0x043
#define CSR_UIP 0x044

/* ustatus CSR bits */
#define USTATUS_UIE 0x00000001
#define USTATUS_UPIE 0x00000010

#define IRQ_U_SOFT 0
#define IRQ_U_TIMER 4
#define IRQ_U_EXT 8

#define MIE_USIE (1 << IRQ_U_SOFT)
#define MIE_UTIE (1 << IRQ_U_TIMER)
#define MIE_UEIE (1 << IRQ_U_EXT)


struct __uintr_frame {
	/*   0 */ uint64_t ra;
	/*   8 */ uint64_t sp;
	/*  16 */ uint64_t gp;
	/*  24 */ uint64_t tp;
	/*  32 */ uint64_t t0;
	/*  40 */ uint64_t t1;
	/*  48 */ uint64_t t2;
	/*  56 */ uint64_t s0;
	/*  64 */ uint64_t s1;
	/*  72 */ uint64_t a0;
	/*  80 */ uint64_t a1;
	/*  88 */ uint64_t a2;
	/*  96 */ uint64_t a3;
	/* 104 */ uint64_t a4;
	/* 112 */ uint64_t a5;
	/* 120 */ uint64_t a6;
	/* 128 */ uint64_t a7;
	/* 136 */ uint64_t s2;
	/* 144 */ uint64_t s3;
	/* 152 */ uint64_t s4;
	/* 160 */ uint64_t s5;
	/* 168 */ uint64_t s6;
	/* 176 */ uint64_t s7;
	/* 184 */ uint64_t s8;
	/* 192 */ uint64_t s9;
	/* 200 */ uint64_t s10;
	/* 208 */ uint64_t s11;
	/* 216 */ uint64_t t3;
	/* 224 */ uint64_t t4;
	/* 232 */ uint64_t t5;
	/* 240 */ uint64_t t6;
};

extern char uintrvec[];
extern char uintrret[];

extern void __handler_entry(struct __uintr_frame* frame, void* handler) {
	csr_clear(CSR_UIP, MIE_USIE);
	void (*__handler)(struct __uintr_frame * frame) = handler;
	__handler(frame);
	csr_set(CSR_UIE, MIE_USIE);
}

#define __asm_syscall(...)                                               \
	__asm__ __volatile__("ecall\n\t" : "+r"(a0) : __VA_ARGS__ : "memory"); \
	return a0;

static inline long __syscall1(long n, long a) {
	register long a7 __asm__("a7") = n;
	register long a0 __asm__("a0") = a;
	__asm_syscall("r"(a7), "0"(a0))
}

static inline long __syscall3(long n, long a, long b, long c) {
	register long a7 __asm__("a7") = n;
	register long a0 __asm__("a0") = a;
	register long a1 __asm__("a1") = b;
	register long a2 __asm__("a2") = c;
	__asm_syscall("r"(a7), "0"(a0), "r"(a1), "r"(a2))
}

#define __NR_uintr_enable 244
void setup_uintr(void* handler, uint64_t lq_dix) {
	int ret = __syscall1(__NR_uintr_enable, lq_dix);
	if (ret < 0) {
		printf("Failed to enable uintr\n");
		exit(1);
	}
	// set user interrupt entry
	csr_write(CSR_UTVEC, uintrvec);
	csr_write(CSR_USCRATCH, handler);
	// enable U-mode interrupt handler
	csr_set(CSR_USTATUS, USTATUS_UIE);
	csr_set(CSR_UIE, MIE_USIE);
}

#endif