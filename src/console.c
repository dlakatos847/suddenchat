/*
 * console.c
 *
 *  Created on: Okt 17, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include <stdio.h>

#include "network.h"
#include "sudden_types.h"

extern int discoveredUsersNo;
extern int discoveredGroupsNo;
extern struct user discoveredUsers[MAX_DISCOVERED_USERS];
extern struct group discoveredGroups[MAX_DISCOVERED_GROUPS];

void showOptions();
void showPrompt();
void listGroupMemberships();
void listCandidates();

void showConsole() {
	int running = 1;
	char choice[2];

	char groupName[50];
	char password[50];

	while (running) {
		fflush(stdin);

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
		case 'g':
			listGroupMemberships();
			break;
		case 'd':
			discover();
			break;
		case 'l':
			listCandidates();
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
	printf("g - show group memberships\n");
	printf("l - list available users and groups\n");
	printf("d - discover available user and group list\n");
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

void listCandidates() {
	int i;
	printf("Users:\n");
	for (i = 0; i < discoveredUsersNo; ++i) {
		printf(" * %s\n", discoveredUsers[i].name);
	}
	printf("Groups:\n");
	for (i = 0; i < discoveredGroupsNo; ++i) {
		printf(" * %s\n", discoveredGroups[i].name);
	}
}
