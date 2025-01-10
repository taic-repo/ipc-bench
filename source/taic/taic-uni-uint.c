#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/common.h"
#include "uintr.h"
#include "taic.h"
#include <assert.h>

volatile unsigned long has_received = 0;
volatile unsigned int received_count = 0;
volatile unsigned int is_inited = 0;
uint64_t client_os = 1;
uint64_t client_proc = 2;
uint64_t server_os = 1;
uint64_t server_proc = 1;

volatile uint64_t server_lq_base;
uint64_t handler = 0x109;

struct Benchmarks bench;

void uintr_handler(struct __uintr_frame* ui_frame) {
	volatile uint64_t data = lq_deq(server_lq_base);
	// received notification
	// assert(data == handler);
	lq_register_receiver(server_lq_base, client_os, client_proc, handler);
	received_count++;
	is_inited = 1;
	has_received = 1;
}

void *client_communicate(void *arg) {
	struct Arguments* args = (struct Arguments*)arg;
	int mem_fd;
	void* taic_base = get_taic(&mem_fd);
	uint64_t lq_base = alloc_lq(taic_base, client_os, client_proc);
	lq_register_sender(lq_base, server_os, server_proc);

	int loop;
	for (loop = args->count; loop > 0; --loop) {
		while (!is_inited) { }
		is_inited = 0;
		has_received = 0;
		// printf("Sending notification\n");
		lq_send_intr(lq_base, server_os, server_proc);
		// Send notification
		while (!has_received){
			// Keep spinning until this notification is received.
			// printf("Waiting for server receive notification\n");			
		}
	}
	free_lq(taic_base, lq_base);
	free_taic(taic_base, &mem_fd);

	return NULL;
}

void server_communicate(struct Arguments* args) {
	int mem_fd;
	void* taic_base = get_taic(&mem_fd);
	server_lq_base = alloc_lq(taic_base, server_os, server_proc);
	setup_uintr(uintr_handler);
	lq_register_receiver(server_lq_base, client_os, client_proc, handler);

	setup_benchmarks(&bench);
	is_inited = 1;

	while (received_count < args->count) {}

	// The message size is always one (it's just a signal)
	args->size = 1;
	evaluate(&bench, args);
	free_lq(taic_base, server_lq_base);
	free_taic(taic_base, &mem_fd);
}

void communicate(struct Arguments* args) {

	pthread_t pt;

	// Create another thread
	if (pthread_create(&pt, NULL, &client_communicate, args)) {
		throw("Error creating sender thread");
	}

	server_communicate(args);

}

// unidirectional: client --> to server
int main(int argc, char* argv[]) {

	struct Arguments args;

	parse_arguments(&args, argc, argv);

	communicate(&args);

	return EXIT_SUCCESS;
}