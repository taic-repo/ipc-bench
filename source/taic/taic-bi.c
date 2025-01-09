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


uint64_t client_os = 1;
uint64_t client_proc = 2;
uint64_t server_os = 1;
uint64_t server_proc = 1;
uint64_t handler = 0x108;

#define SERVER_TOKEN 0
#define CLIENT_TOKEN 1

volatile unsigned long is_inited[2];

struct Benchmarks bench;

void wait(unsigned int token, uint64_t lq_base) {
	// Keep spinning until the notification is received
	volatile uint64_t data = lq_deq(lq_base);
	while (data != handler) {
		data = lq_deq(lq_base);
	}
	if(token == SERVER_TOKEN) {
		// printf("Server received notification\n");
		lq_register_receiver(lq_base, client_os, client_proc, handler);
	} else {
		// printf("Client received notification\n");
		lq_register_receiver(lq_base, server_os, server_proc, handler);
	}
	is_inited[token] = 1;
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
	uint64_t lq_idx = alloc_lq(taic_base, client_os, client_proc);
	uint64_t lq_base = idx2base(taic_base, lq_idx);
	lq_register_sender(lq_base, server_os, server_proc);
	lq_register_receiver(lq_base, server_os, server_proc, handler);
	is_inited[CLIENT_TOKEN] = 1;
	int loop;
	for (loop = args->count; loop > 0; --loop) {
		wait(CLIENT_TOKEN, lq_base);
		notify(SERVER_TOKEN, lq_base);
	}

	free_lq(taic_base, lq_base);
	free_taic(taic_base, &mem_fd);

	return NULL;
}

void server_communicate(struct Arguments* args) {
	int mem_fd;
	void* taic_base = get_taic(&mem_fd);
	uint64_t lq_idx = alloc_lq(taic_base, server_os, server_proc);
	uint64_t lq_base = idx2base(taic_base, lq_idx);
	lq_register_sender(lq_base, client_os, client_proc);
	lq_register_receiver(lq_base, client_os, client_proc, handler);
	is_inited[SERVER_TOKEN] = 1;
	setup_benchmarks(&bench);

	int message;
	for (message = 0; message < args->count; ++message) {
		notify(CLIENT_TOKEN, lq_base);
		wait(SERVER_TOKEN, lq_base);
	}

	// The message size is always one (it's just a signal)
	args->size = 1;
	evaluate(&bench, args);
	free_lq(taic_base, lq_base);
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