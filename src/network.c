/*
 * network.c
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "group.h"

#define INET_ADDRSTRLEN 16

int groupMembershipNo = 0;
struct group groupMemberships[10];

struct group * listMemberGroups() {
	return groupMemberships;
}

int getMaximumGroupsNo() {
	return sizeof(groupMemberships) / sizeof(struct group);
}

int getMemberGroupsNo() {
	return groupMembershipNo;
}

void joinGroup(char *groupName, char *password) {
	// check the maximum group memberships
	if (groupMembershipNo == sizeof(groupMemberships) / sizeof(struct group)) {
		printf("You reached the maximum group membership number (%d).",
				(int) (sizeof(groupMemberships) / sizeof(struct group)));
		return;
	}

	// look up existing group membership
	int i;
	int alreadyMember = 0;
	for (i = 0; i < groupMembershipNo; ++i) {
		if (strcmp(groupMemberships[i].name, groupName) == 0) {
			strcpy(groupMemberships[i].password, password);
			alreadyMember = 1;
		}
	}

	// if not a member of the group, add them to it
	if (alreadyMember == 0) {
		strcpy(groupMemberships[groupMembershipNo].name, groupName);
		strcpy(groupMemberships[groupMembershipNo].password, password);
		groupMembershipNo++;
	}
}

void discoverGroups() {
	//TODO contd.
	struct ifaddrs *ifs;
	struct ifaddrs *cif;

	if (getifaddrs(&ifs) != 0) {
		perror("getifaddrs");
	}

	//print broadcast addresses
	char buf[INET_ADDRSTRLEN];
	struct sockaddr_in *sin;
	printf("Searching groups on interfaces with addresses (no loopback):\n");
	for (cif = ifs; cif != NULL; cif = cif->ifa_next) {
		//avoid loopback interfaces
		if (strcmp(cif->ifa_name, "lo") == 0) {
			continue;
		}
		if (cif->ifa_addr->sa_family == AF_INET) {
			cif->ifa_flags = cif->ifa_flags | IFF_BROADCAST;
			sin = (struct sockaddr_in *) cif->ifa_addr;
			printf("* %s\n", inet_ntoa(sin->sin_addr));
		}
	}

	//free memory
	freeifaddrs(ifs);

	return;
}

void sendMessage(char *message, char *groupName) {
	//TODO
	return;
}
