/*
 * network.c
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
//#include <sys/types.h>

#include "group.h"

#define INET_ADDRSTRLEN 16
#define BCAST_PORT 12345

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

	int bcastsocket;
	if ((bcastsocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("discoverGroups bcastsocket");
	}
	int broadcastEnable = 1;
	if (setsockopt(bcastsocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
			sizeof(broadcastEnable)) < 0) {
		perror("discoverGroups setsockopt");
	}
	struct sockaddr_in *socketaddress;
	for (cif = ifs; cif != NULL; cif = cif->ifa_next) {
		//avoid loopback interfaces
		if (strcmp(cif->ifa_name, "lo") == 0) {
			continue;
		}
		if (cif->ifa_addr->sa_family == AF_INET) {
			cif->ifa_flags = cif->ifa_flags | IFF_BROADCAST;
			sin = (struct sockaddr_in *) cif->ifa_addr;
			printf("* %s ", inet_ntoa(sin->sin_addr));

			socketaddress = (struct sockaddr_in *) cif->ifa_ifu.ifu_broadaddr;
			printf("with bcast address %s\n",
					inet_ntoa(socketaddress->sin_addr));
			socketaddress->sin_port = htons(12345);
			sendto(bcastsocket, "SUDDENCHAT_DISCOVER_GROUPS",
					strlen("SUDDENCHAT_DISCOVER_GROUPS"), 0,
					(struct sockaddr *) socketaddress, sizeof(struct sockaddr));
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
