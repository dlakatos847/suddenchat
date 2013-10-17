#include <stdio.h>

void showOptions();
void showPrompt();

void showConsole() {
	int running = 1;
	char choice = 0;

	while (running) {
		showOptions();
		//showPrompt();
		choice = getchar();
		getchar();
		switch (choice) {
		case 'q':
			running = 0;
			break;
		case 's':
			break;
		default:
			printf("Not valid input\n");
			break;
		}
	}
}

void showOptions() {
	printf("q - quit\n");
	printf("s - start chat\n");
}

void showPrompt() {
	printf("#> ");
}
