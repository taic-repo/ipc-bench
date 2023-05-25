#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "uintr.h"

volatile unsigned int uintr_received;
unsigned int uintr_fd;

uint64_t uintr_handler(struct __uintr_frame *ui_frame, uint64_t irqs) {
  printf("\t-- User Interrupt handler --\n");

  // read pending bits
  printf("\tPending User Interrupts: %lx\n", irqs);

  uintr_received = 1;

  return 0;
}

void *sender_thread(void *arg) {
  int uipi_index;

  uipi_index = uintr_register_sender(uintr_fd);
  if (uipi_index < 0) {
    printf("Sending IPI from sender thread\n");
    exit(EXIT_FAILURE);
  }

  printf("Sending IPI from sender thread %d\n", uipi_index);
  uipi_send(uipi_index);

  return NULL;
}

int main() {
  printf("Basic test: uipi_sample\n");

  pthread_t pt;
  int ret;

  if (uintr_register_receiver(uintr_handler)) {
    printf("Interrupt handler register error\n");
    exit(EXIT_FAILURE);
  }

  ret = uintr_create_fd(1);
  if (ret < 0) {
    printf("Interrupt vector allocation error\n");
    exit(EXIT_FAILURE);
  }

  uintr_fd = ret;

  printf("Receiver enabled interrupts\n");

  if (pthread_create(&pt, NULL, &sender_thread, NULL)) {
    printf("Error creating sender thread\n");
    exit(EXIT_FAILURE);
  }

  while (!uintr_received)
    ;

  pthread_join(pt, NULL);
  close(uintr_fd);

  printf("Success\n");
  exit(EXIT_SUCCESS);
}