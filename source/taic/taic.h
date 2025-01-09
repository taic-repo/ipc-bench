#ifndef _TAIC_H
#define _TAIC_H

#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "uintr.h"

#define TAIC_BASE 0x1000000
#define TAIC_SIZE 0x1000000
#define LQ_NUM 8

void* get_taic(int* mem_fd) {
    // printf("%s %d start taic test......\n", __FUNCTION__, __LINE__);
	*mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (*mem_fd == -1) {
		perror("open failed");
		exit(1);
	}
	// printf("%s %d open /dev/mem success......\n", __FUNCTION__, __LINE__);
	void *taic_base = mmap(NULL, TAIC_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, *mem_fd, TAIC_BASE);
	if (taic_base == NULL) {
		perror("mmap failed");
		close(*mem_fd);
		exit(1);
	}
    // printf("%s %d mmap success taic_base: %lx\n", __FUNCTION__, __LINE__, (uint64_t)taic_base);
    return taic_base;
}

void free_taic(void* taic_base, int* mem_fd) {
    munmap(taic_base, TAIC_SIZE);
    close(*mem_fd);
}

uint64_t alloc_lq(void* taic_base, uint64_t os, uint64_t proc) {
    volatile uint64_t *alloc = (uint64_t *)taic_base;
    *alloc = os;
    *alloc = proc;
    uint64_t idx = *alloc;
    return idx;
}

uint64_t idx2base(void* taic_base, uint64_t idx) {
    uint32_t gq_idx = (idx >> 32) & 0xffffffff;
    uint32_t lq_idx = idx & 0xffffffff;
    uint64_t base = (uint64_t)taic_base + 0x1000 + (gq_idx * LQ_NUM + lq_idx) * 0x1000;
    return base;
}

void free_lq(void* taic_base, uint64_t lq_base) {
    volatile uint64_t *free = (uint64_t *)((uint64_t)taic_base + 0x08);
    uint64_t idx = (lq_base - 0x1000 - (uint64_t)taic_base) / 0x1000;
    uint64_t gq_idx = (idx / LQ_NUM);
    uint64_t lq_idx = idx % LQ_NUM;
    uint64_t idx64 = (gq_idx << 32) | lq_idx;
    *free = idx64;
}

void lq_enq(uint64_t lq_base, uint64_t data) {
    volatile uint64_t *enq = (uint64_t *)lq_base;
    *enq = data;
}

uint64_t lq_deq(uint64_t lq_base) {
    volatile uint64_t *deq = (uint64_t *)(lq_base + 0x08);
    uint64_t data = *deq;
    return data;
}

void lq_register_sender(uint64_t lq_base, uint64_t recv_os, uint64_t recv_proc) {
    volatile uint64_t *reg = (uint64_t *)(lq_base + 0x18);
    *reg = recv_os;
    *reg = recv_proc;
}

void lq_register_receiver(uint64_t lq_base, uint64_t send_os, uint64_t send_proc, uint64_t handler) {
    volatile uint64_t *reg = (uint64_t *)(lq_base + 0x28);
    *reg = send_os;
    *reg = send_proc;
    *reg = handler;
}

void lq_send_intr(uint64_t lq_base, uint64_t recv_os, uint64_t recv_proc) {
    volatile uint64_t *send = (uint64_t *)(lq_base + 0x30);
    *send = recv_os;
    *send = recv_proc;
}

#endif