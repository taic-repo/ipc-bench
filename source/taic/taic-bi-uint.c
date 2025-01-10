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

uint64_t client_os = 1;
uint64_t client_proc = 2;
uint64_t server_os = 1;
uint64_t server_proc = 1;
uint64_t handler = 0x109;
volatile uint64_t server_lq_base;
volatile uint64_t client_lq_base;

#define SERVER_TOKEN 0
#define CLIENT_TOKEN 1

volatile unsigned long is_inited[2];
volatile unsigned long has_received[2];

struct Benchmarks bench;

void server_uintr_handler(struct __uintr_frame* ui_frame) {
	volatile uint64_t data = lq_deq(server_lq_base);
	// received notification
	// assert(data == handler);
	lq_register_receiver(server_lq_base, client_os, client_proc, handler);
	is_inited[0] = 1;
	has_received[0] = 1;
}

void client_uintr_handler(struct __uintr_frame* ui_frame) {
	volatile uint64_t data = lq_deq(client_lq_base);
	// received notification
	// assert(data == handler);
	lq_register_receiver(client_lq_base, server_os, server_proc, handler);
	is_inited[1] = 1;
	has_received[1] = 1;
}

void wait(unsigned int token, uint64_t lq_base) {
	// Keep spinning until the notification is received
	while (!has_received[token]) { }
	has_received[token] = 0;
}

void notify(unsigned int token, uint64_t lq_base) {
	// wait until the peer is inited
	while(!is_inited[token]) {}
	is_inited[token] = 0;
	if(token == SERVER_TOKEN) {
		lq_send_intr(lq_base, server_os, server_proc);
	} else {
		lq_send_intr(lq_base, client_os, client_proc);
	}
}

void *client_communicate(void *arg) {
	struct Arguments* args = (struct Arguments*)arg;
	int mem_fd;
	void* taic_base = get_taic(&mem_fd);
	client_lq_base = alloc_lq(taic_base, client_os, client_proc);
	setup_uintr(client_uintr_handler);
	lq_register_sender(client_lq_base, server_os, server_proc);
	lq_register_receiver(client_lq_base, server_os, server_proc, handler);
	is_inited[CLIENT_TOKEN] = 1;
	has_received[CLIENT_TOKEN] = 0;
	int loop;
	for (loop = args->count; loop > 0; --loop) {
		wait(CLIENT_TOKEN, client_lq_base);
		notify(SERVER_TOKEN, client_lq_base);
	}

	free_lq(taic_base, client_lq_base);
	free_taic(taic_base, &mem_fd);

	return NULL;
}

void server_communicate(struct Arguments* args) {
	int mem_fd;
	void* taic_base = get_taic(&mem_fd);
	server_lq_base = alloc_lq(taic_base, server_os, server_proc);
	setup_uintr(server_uintr_handler);
	lq_register_sender(server_lq_base, client_os, client_proc);
	lq_register_receiver(server_lq_base, client_os, client_proc, handler);
	is_inited[SERVER_TOKEN] = 1;
	has_received[SERVER_TOKEN] = 0;
	setup_benchmarks(&bench);

	int message;
	for (message = 0; message < args->count; ++message) {
		notify(CLIENT_TOKEN, server_lq_base);
		wait(SERVER_TOKEN, server_lq_base);
	}

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

// Bidirectional: client <--> server
// 		server --> client
//         \<------/
int main(int argc, char* argv[]) {

	struct Arguments args;

	parse_arguments(&args, argc, argv);

	communicate(&args);

	return EXIT_SUCCESS;
}