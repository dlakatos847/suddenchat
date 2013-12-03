/*
 * main.c
 *
 *  Created on: Okt 17, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "console.h"

int main(int argc, char* argv[]) {
	printf("SuddenChat started\n");
	showConsole();
	printf("SuddenChat stopped with error code %d\n", errno);
	return 0;
}
