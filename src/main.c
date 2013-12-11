/*
 * main.c
 *
 *  Created on: Okt 17, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "console.h"
#include "network.h"
#include "sudden_types.h"

struct user myself;

int main(int argc, char* argv[]) {
	printf("SuddenChat started\n");

	printf("Name: ");
	gets(myself.name);

	pthread_t discoveryReader;
	if (pthread_create(&discoveryReader, NULL, (void*) collectDiscoveryAnswers,
	NULL) < 0) {
		perror("main pthread_create");
		exit(-1);
	}

	pthread_t discoveryResponder;
	if (pthread_create(&discoveryResponder, NULL,
			(void*) answerDiscoveryRequests, NULL) < 0) {
		perror("main pthread_create");
		exit(-1);
	}

	pthread_t discoverer;
	if (pthread_create(&discoverer, NULL, (void*) discover, NULL) < 0) {
		perror("main pthread_create");
		exit(-1);
	}

	// show CLI
	showConsole();

	printf("SuddenChat stopped with error code %d\n", errno);
	return 0;
}
