#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

int main(int argc, char* argv[]) {
	mylog("SuddenChat started");
	showConsole();
	mylog("SuddenChat stopped");
	return 0;
}
