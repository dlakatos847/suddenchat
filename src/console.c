/*
 * console.c
 *
 *  Created on: Okt 17, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>	//termios, TCSANOW, ECHO, ICANON
#include <unistd.h>		//STDIN_FILENO
#include "network.h"
#include "group.h"

void showOptions();
void showPrompt();
void listGroupMemberships();
//void disableBuffering();
//void restoreConsoleSettings();

//static struct termios oldt, newt;
char input[100];

void showConsole() {
	int running = 1;
	char choice[2];

	char groupName[50];
	char password[50];

	//disableBuffering();
	//setbuf(stdout,NULL);

	while (running) {
		fflush(stdin);
		//printf("\x1b[2JTaking control of your console.");
		showOptions();
		showPrompt();
		gets(choice);
		switch (choice[0]) {
		case 'q':
			running = 0;
			break;
		case 'j':
			fflush(stdin);

			printf("Group's name:\n");
			showPrompt();
			gets(groupName);

			printf("Password:\n");
			showPrompt();
			gets(password);

			joinGroup(groupName, password);
			break;
		case 's':
			listGroupMemberships();
			break;
		case 'd':
			discoverGroups();
			break;
		default:
			printf("Not valid input\n\n");
			break;
		}
	}

	//restoreConsoleSettings();
}

void showOptions() {
	printf("Please select from the following options!\n");
	printf("j - join a group\n");
	printf("s - show group memberships\n");
	printf("d - discover groups\n");
	printf("q - quit\n");
}

void showPrompt() {
	printf("#> ");
}

void listGroupMemberships() {
	struct group * memberships = listMemberGroups();
	int groupsNo = getMemberGroupsNo();

	int i;
	for (i = 0; i < groupsNo; ++i) {
		printf("Name: %s, password: %s\n", memberships[i].name,
				memberships[i].password);
	}
}

//void disableBuffering() {
//
//	/*tcgetattr gets the parameters of the current terminal
//	 STDIN_FILENO will tell tcgetattr that it should write the settings
//	 of stdin to oldt*/
//	tcgetattr( STDIN_FILENO, &oldt);
//	/*now the settings will be copied*/
//	newt = oldt;
//
//	/*ICANON normally takes care that one line at a time will be processed
//	 that means it will return if it sees a "\n" or an EOF or an EOL*/
//	newt.c_lflag &= ~(ICANON);
//
//	/*Those new settings will be set to STDIN
//	 TCSANOW tells tcsetattr to change attributes immediately. */
//	tcsetattr( STDIN_FILENO, TCSANOW, &newt);
//}
//
//void restoreConsoleSettings() {
//	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
//}
