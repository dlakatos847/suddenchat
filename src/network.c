/*
 * network.c
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "group.h"

int groupMembershipNo = 0;
struct group groups[10];

struct group * listMemberGroups() {
	return groups;
}

int getMaximumGroupsNo() {
	return sizeof(groups) / sizeof(struct group);
}

int getMemberGroupsNo() {
	return groupMembershipNo;
}

void joinGroup(char *groupName, char *password) {
	// check the maximum group memberships
	if (groupMembershipNo == sizeof(groups) / sizeof(struct group)) {
		printf("You reached the maximum group membership number (%d).",
				(int) (sizeof(groups) / sizeof(struct group)));
		return;
	}

	// look up existing group membership
	int i;
	int alreadyMember = 0;
	for (i = 0; i < groupMembershipNo; ++i) {
		if (strcmp(groups[i].name, groupName) == 0) {
			strcpy(groups[i].password, password);
			alreadyMember = 1;
		}
	}

	// if not a member of the group, add them to it
	if (alreadyMember == 0) {
		strcpy(groups[groupMembershipNo].name, groupName);
		strcpy(groups[groupMembershipNo].password, password);
		groupMembershipNo++;
	}
}

void discoverGroups() {
	//TODO
	return;
}

void sendMessage(char *message, char *groupName) {
	//TODO
	return;
}
