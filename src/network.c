/*
 * network.c
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include "network.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "sudden_types.h"

/*
 * variables
 */
int associatedUsersNo = 0;
int associatedGroupsNo = 0;
struct user associatedUsers[MAX_DISCOVERED_USERS];
struct group associatedGroups[MAX_DISCOVERED_GROUPS];
int discoveredUsersNo = 0;
int discoveredGroupsNo = 0;
struct user discoveredUsers[MAX_DISCOVERED_USERS];
struct group discoveredGroups[MAX_DISCOVERED_GROUPS];
extern struct user myself;

/*
 * function declarations
 */
void collectDiscoveryAnswers();

struct group * listMemberGroups() {
	return associatedGroups;
}

/*
 * discover Suddenchat users and groups on network
 */
void discover() {
	// interface linked list
	struct ifaddrs *ifs;
	// current interface
	struct ifaddrs *cif;
	// bcast enable bit
	int broadcastEnable = 1;
	// socket
	int bcastSocket;
	// socket address
	struct sockaddr_in bcastSocketAddress;

	discoveredUsersNo = 0;
	discoveredGroupsNo = 0;

	// *** initialization
	// enumerate interfaces
	if (getifaddrs(&ifs) != 0) {
		perror("getifaddrs");
		return;
	}
	// initialize socket
	if ((bcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("discoverGroups bcastsocket");
		return;
	}
	if (setsockopt(bcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
			sizeof(broadcastEnable)) < 0) {
		perror("discoverGroups setsockopt");
		return;
	}

	while (1) {
		for (cif = ifs; cif != NULL; cif = cif->ifa_next) {
			// avoid loopback interfaces
			if (strcmp(cif->ifa_name, "lo") == 0) {
				continue;
			}

			// only IPv4
			if (cif->ifa_addr->sa_family == AF_INET) {
				// get socket address to broadcast
				memcpy(&bcastSocketAddress,
						(struct sockaddr_in *) cif->ifa_ifu.ifu_broadaddr,
						sizeof(*(cif->ifa_ifu.ifu_broadaddr)));
				// set hello port to BCAST_PORT
				bcastSocketAddress.sin_port = htons(HELLO_REQ_PORT);
				// send hello message
				sendto(bcastSocket, DISCOVERY_REQ_MESSAGE,
				DISCOVERY_REQ_LENGTH + 1, 0,
						(struct sockaddr *) &bcastSocketAddress,
						sizeof(bcastSocketAddress));
			}
		}
		sleep(1);
	}
	// free memory: linked list
	freeifaddrs(ifs);
}

/*
 * thread function; waits for the user and group name responses
 */
void collectDiscoveryAnswers() {
	int serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// amount of bytes read
	int readBytes;
	struct sockaddr_in serverSocketAddress;
	struct sockaddr_in clientSocketAddress;
	int serverSocketAddressLength = sizeof(serverSocketAddress);
	// receive group names to this buffer
	char messageBuffer[MAX_NAME_LENGTH + 2];
	// iteration variable
	int i, isNew;
	// name
	char *name = (char*) messageBuffer + 1;

	// check if the socket went wrong
	if (serverSocket < 0) {
		perror("collectGroups serverSocket");
		pthread_exit(NULL);
	}

	// zero out the socket address structure
	memset((char *) &serverSocketAddress, 0, sizeof(serverSocketAddress));

	// initialize the socket address
	serverSocketAddress.sin_family = AF_INET;
	serverSocketAddress.sin_port = htons(HELLO_RSP_PORT);
	serverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if (bind(serverSocket, (struct sockaddr*) &serverSocketAddress,
			sizeof(serverSocketAddress)) == -1) {
		perror("bind");
		pthread_exit(NULL);
	}

	// receive network communication
	while (1) {
		if ((readBytes = recvfrom(serverSocket, messageBuffer,
				sizeof(messageBuffer), 0,
				(struct sockaddr *) &clientSocketAddress,
				&serverSocketAddressLength)) < 0) {
			perror("collectDiscovery recvfrom");
			continue;
		}

		// it's a user
		if (messageBuffer[0] == 'U') {
			// check discovered user or group for duplicates
			isNew = 1;
			for (i = 0; i < discoveredUsersNo; ++i) {
				if (strcmp(discoveredUsers[i].name, name) == 0) {
					isNew = 0;
					break;
				}
			}

			if (isNew) {
				strcpy(discoveredUsers[discoveredUsersNo].name, name);
				inet_ntop(AF_INET, &clientSocketAddress.sin_addr.s_addr,
						discoveredUsers[discoveredUsersNo].ip, IP_LENGTH);
				discoveredUsersNo++;
			}
		}
		// it's a group
		else if (messageBuffer[0] == 'G') {
			// check discovered user or group for duplicates
			isNew = 1;
			for (i = 0; i < discoveredGroupsNo; ++i) {
				if (strcmp(discoveredGroups[i].name, name) == 0) {
					isNew = 0;
					break;
				}
			}

			if (isNew) {
				strcpy(discoveredGroups[discoveredGroupsNo].name, name);
				discoveredGroupsNo++;
			}
		}
		// it's ... well, something we don't care about
		else {

		}
	}
}

void answerDiscoveryRequests() {
	int serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// amount of bytes read
	int readBytes;
	struct sockaddr_in serverSocketAddress;
	struct sockaddr_in clientSocketAddress;
	socklen_t clientSocketAddressSize = sizeof(clientSocketAddress);
	int serverSocketAddressLength = sizeof(serverSocketAddress);
	// receive group names to this buffer
	char messageBuffer[DISCOVERY_REQ_LENGTH + 1];
	// iteration variable
	int i;

	// check if the socket went wrong
	if (serverSocket < 0) {
		perror("collectGroups serverSocket");
		pthread_exit(NULL);
	}

	// zero out the socket address structure
	memset((char *) &serverSocketAddress, 0, sizeof(serverSocketAddress));

	// initialize the socket address
	serverSocketAddress.sin_family = AF_INET;
	serverSocketAddress.sin_port = htons(HELLO_REQ_PORT);
	serverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	//bind socket to port
	if (bind(serverSocket, (struct sockaddr*) &serverSocketAddress,
			sizeof(serverSocketAddress)) == -1) {
		perror("bind");
		pthread_exit(NULL);
	}

	// answer discovery requests
	while (1) {
		if ((readBytes = recvfrom(serverSocket, messageBuffer,
				sizeof(messageBuffer), 0,
				(struct sockaddr *) &clientSocketAddress,
				&serverSocketAddressLength)) < 0) {
			perror("collectDiscovery recvfrom");
			continue;
		}

		char answer[1 + strlen(myself.name) + 1];
		answer[0] = 'U';
		strcpy(answer + 1, myself.name);
		clientSocketAddress.sin_port = htons(HELLO_RSP_PORT);

		// it's a discovery request
		if (strcmp(messageBuffer, DISCOVERY_REQ_MESSAGE) == 0) {
			sendto(serverSocket, answer, sizeof(answer), 0,
					(struct sockaddr*) &clientSocketAddress,
					clientSocketAddressSize);
		}
	}
}

int getMaximumGroupsNo() {
	return sizeof(associatedGroups) / sizeof(struct group);
}

int getMemberGroupsNo() {
	return associatedGroupsNo;
}

void joinGroup(char *groupName, char *password) {
// check the maximum group memberships
	if (associatedGroupsNo == sizeof(associatedGroups) / sizeof(struct group)) {
		printf("You reached the maximum group membership number (%d).",
				(int) (sizeof(associatedGroups) / sizeof(struct group)));
		return;
	}

// look up existing group membership
	int i;
	int alreadyMember = 0;
	for (i = 0; i < associatedGroupsNo; ++i) {
		if (strcmp(associatedGroups[i].name, groupName) == 0) {
			strcpy(associatedGroups[i].password, password);
			alreadyMember = 1;
		}
	}

// if not a member of the group, add them to it
	if (alreadyMember == 0) {
		strcpy(associatedGroups[associatedGroupsNo].name, groupName);
		strcpy(associatedGroups[associatedGroupsNo].password, password);
		associatedGroupsNo++;
	}
}

/*
 * send a message to the user
 */
void sendMessageToPerson(char *message, int messageLength, char *username) {
// the socket
	int senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// the socket address
	struct sockaddr_in senderSocketAddress;

//TODO
	char *ip;

// quit if the socket has problems
	if (senderSocket < 0) {
		perror("sendMessageToPerson socket");
		return;
	}

//set socket address
	senderSocketAddress.sin_family = AF_INET;
	senderSocketAddress.sin_port = htons(MESSAGE_PORT);
	if (inet_aton(ip, &senderSocketAddress.sin_addr) == 0) {
		perror("sendMessageToPerson inet_aton");
		return;
	}

// send the message
	sendto(senderSocket, message, messageLength, 0,
			(struct sockaddr*) &senderSocketAddress,
			sizeof(senderSocketAddress));
}

//TODO
void sendMessageToGroup(char *message, char *groupName) {
	return;
}
