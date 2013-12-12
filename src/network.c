/*
 * network.c
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include "network.h"

#include <arpa/inet.h>
#include <time.h>
#include <asm-generic/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>

#include "sudden_types.h"
#include "conversation.h"
#include "cypher.h"

/*
 * variables
 */
struct group_collection associatedGroups;
extern struct user myself;
struct user_collection discoveredUsers;
struct group_collection discoveredGroups;

/*
 * function declarations
 */
void collectDiscoveryAnswers();
void getIpFromSocketAddress(struct sockaddr_in *socketAddress, char *ip);
struct user* getUserFromIp(char *senderIp);
void handleFirstMessageFromUser(struct user *user, char *message);
void cleanDiscoveryStructures();

void networkInit() {
	memset(&discoveredUsers, 0, sizeof(discoveredUsers));
	memset(&discoveredGroups, 0, sizeof(discoveredGroups));
	memset(&associatedGroups, 0, sizeof(associatedGroups));
	pthread_mutex_init(&discoveredUsers.lock, NULL);
	pthread_mutex_init(&discoveredGroups.lock, NULL);

	discoveredUsers.oldest.younger = &discoveredUsers.youngest;
	discoveredUsers.youngest.older = &discoveredUsers.oldest;
	discoveredGroups.oldest.younger = &discoveredGroups.youngest;
	discoveredGroups.youngest.older = &discoveredGroups.oldest;
	associatedGroups.oldest.younger = &associatedGroups.youngest;
	associatedGroups.youngest.older = &associatedGroups.oldest;
}

void networkDestroy() {
	pthread_mutex_destroy(&discoveredUsers.lock);
	pthread_mutex_destroy(&discoveredGroups.lock);

	if (discoveredUsers.entryQuantity > 0) {
		struct user_collection_entry *uce = discoveredUsers.oldest.younger;
		while (uce != &discoveredUsers.youngest) {
			pthread_mutex_destroy(&uce->lock);
			uce = uce->younger;
		}
	}

	if (discoveredGroups.entryQuantity > 0) {
		struct group_collection_entry *gce = discoveredGroups.oldest.younger;
		while (gce != &discoveredGroups.youngest) {
			pthread_mutex_destroy(&gce->lock);
			gce = gce->younger;
		}
	}

	if (associatedGroups.entryQuantity > 0) {
		struct group_collection_entry *gce = associatedGroups.oldest.younger;
		while (gce != &associatedGroups.youngest) {
			pthread_mutex_destroy(&gce->lock);
			gce = gce->younger;
		}
	}
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
		sleep(2);
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
			pthread_mutex_lock(&discoveredUsers.lock);

			isNew = 1;

			// check self echo
			if (strcmp(name, myself.name) == 0) {
				pthread_mutex_unlock(&discoveredUsers.lock);
				continue;
			}

			// check discovered user for duplicates
			if (discoveredUsers.entryQuantity > 0) {
				struct user_collection_entry *uce =
						discoveredUsers.oldest.younger;
				while (uce != &discoveredUsers.youngest) {
					if (strcmp(uce->user.name, name) == 0) {
						uce->updateTime = time(NULL);
						isNew = 0;
						break;
					}
					uce = uce->younger;
				}
			}

			if (isNew) {
				struct user_collection_entry *uce = malloc(
						sizeof(struct user_collection_entry));
				memset(uce, 0, sizeof(struct user_collection_entry));
				discoveredUsers.youngest.older->younger = uce;
				uce->younger = &discoveredUsers.youngest;
				uce->older = discoveredUsers.youngest.older;
				discoveredUsers.youngest.older = uce;
				pthread_mutex_init(&uce->lock, NULL);
				uce->updateTime = time(NULL);
				strcpy(uce->user.name, name);
				inet_ntop(AF_INET, &clientSocketAddress.sin_addr.s_addr,
						uce->user.ip, MAX_IP_LENGTH);
				discoveredUsers.entryQuantity++;
				uce->updateTime = time(NULL);
			}

			pthread_mutex_unlock(&discoveredUsers.lock);
		}
		// it's a group
		else if (messageBuffer[0] == 'G') {
			pthread_mutex_lock(&discoveredGroups.lock);

			isNew = 1;

			// check discovered user or group for duplicates
			struct group_collection_entry *gce = discoveredGroups.oldest.younger;
			while (gce != &discoveredGroups.youngest) {
				if (strcmp(gce->group.name, name) == 0) {
					gce->updateTime = time(NULL);
					isNew = 0;
					break;
				}
				gce = gce->younger;
			}

			if (isNew) {
				gce = malloc(sizeof(struct user_collection_entry));
				memset(gce, 0, sizeof(struct user_collection_entry));
				discoveredGroups.youngest.older->younger = gce;
				gce->younger = &discoveredGroups.youngest;
				gce->older = discoveredGroups.youngest.older;
				discoveredGroups.youngest.older = gce;
				pthread_mutex_init(&gce->lock, NULL);
				strcpy(gce->group.name, name);
				discoveredGroups.entryQuantity++;
				gce->updateTime = time(NULL);
				pthread_mutex_init(&gce->lock, NULL);
				strcpy(gce->group.name, name);
				discoveredGroups.entryQuantity++;
			}

			pthread_mutex_unlock(&discoveredGroups.lock);
		}
	}
}

void cleanDiscoveryStructures() {
	//!!!
	return;

	while (1) {
		pthread_mutex_lock(&discoveredUsers.lock);

		time_t now = time(NULL);

		if (discoveredUsers.entryQuantity > 0) {
			struct user_collection_entry *uce = discoveredUsers.oldest.younger,
					*uaux;
			int outdated = 0;
			while (uce != &discoveredUsers.youngest) {
				outdated =
						((now - uce->updateTime > DISCOVERY_TIMEOUT)
								&& (uce->user.conv.messageCount == 0)) ? 1 : 0;
				if (outdated) {
					// delete discovered user
					uce->older->younger = uce->younger;
					uce->younger->older = uce->older;
					uaux = uce;
					discoveredUsers.entryQuantity--;
					pthread_mutex_destroy(&uce->lock);
				}
				uce = uce->younger;
				if (outdated) {
					free(uaux);
				}
			}
		}

		pthread_mutex_unlock(&discoveredUsers.lock);

		pthread_mutex_lock(&discoveredGroups.lock);

		if (discoveredGroups.entryQuantity > 0) {
			struct group_collection_entry
					*gce = discoveredGroups.oldest.younger, *gaux;
			int outdated = 0;
			while (gce != NULL) {
				outdated = (now - gce->updateTime > DISCOVERY_TIMEOUT) ? 1 : 0;
				if (outdated) {
					// delete discovered user
					gce->older->younger = gce->younger;
					gce->younger->older = gce->older;
					gaux = gce;
					discoveredUsers.entryQuantity--;
					pthread_mutex_destroy(&gce->lock);
				}
				gce = gce->younger;
				if (outdated) {
					free(gaux);
				}
			}
		}

		pthread_mutex_unlock(&discoveredGroups.lock);

		sleep(1);
	}
}

/*
 * thread function; answers discovery requests
 */
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

// it's a discovery request
		if (strcmp(messageBuffer, DISCOVERY_REQ_MESSAGE) == 0) {
			int i;
			char userNameAnswer[MAX_USERNAME_LENGTH];
			userNameAnswer[0] = 'U';
			strcpy(userNameAnswer + 1, myself.name);
			clientSocketAddress.sin_port = htons(HELLO_RSP_PORT);

			sendto(serverSocket, userNameAnswer, sizeof(userNameAnswer), 0,
					(struct sockaddr*) &clientSocketAddress,
					clientSocketAddressSize);

			pthread_mutex_lock(&discoveredGroups.lock);

			if (associatedGroups.entryQuantity > 0) {
				char groupNameAnswer[MAX_GROUPNAME_LENGTH];
				groupNameAnswer[0] = 'G';
				struct group_collection_entry *gce =
						associatedGroups.oldest.younger;
				while (gce != &associatedGroups.youngest) {
					strcpy(groupNameAnswer + 1, gce->group.name);
					sendto(serverSocket, groupNameAnswer,
							sizeof(groupNameAnswer), 0,
							(struct sockaddr*) &clientSocketAddress,
							clientSocketAddressSize);
					gce = gce->younger;
				}
			}

			pthread_mutex_unlock(&discoveredGroups.lock);
		}
// protocol error
		else {
			perror("answerDiscoveryRequests protocol error");
		}
	}
}

/*
 * tread function; receives messages
 */
void receiveMessages() {
	int serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// amount of bytes read
	int readBytes;
	struct sockaddr_in serverSocketAddress;
	struct sockaddr_in clientSocketAddress;
	socklen_t clientSocketAddressSize = sizeof(clientSocketAddress);
	int serverSocketAddressLength = sizeof(serverSocketAddress);
// receive group names to this buffer
	char messageBuffer[MAX_MESSAGE_LENGTH + 1];
// iteration variable
	int i;
	struct user *user;
	char senderIp[MAX_IP_LENGTH + 1];

// check if the socket went wrong
	if (serverSocket < 0) {
		perror("receiveMessages serverSocket");
		pthread_exit(NULL);
	}

// zero out the socket address structure
	memset((char *) &serverSocketAddress, 0, sizeof(serverSocketAddress));

// initialize the socket address
	serverSocketAddress.sin_family = AF_INET;
	serverSocketAddress.sin_port = htons(MESSAGE_PORT);
	serverSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

//bind socket to port
	if (bind(serverSocket, (struct sockaddr*) &serverSocketAddress,
			sizeof(serverSocketAddress)) == -1) {
		perror("receiveMessages bind");
		pthread_exit(NULL);
	}

	while (1) {
		if ((readBytes = recvfrom(serverSocket, messageBuffer,
				sizeof(messageBuffer), 0,
				(struct sockaddr *) &clientSocketAddress,
				&serverSocketAddressLength)) < 0) {
			perror("receiveMessages recvfrom");
			continue;
		}

		getIpFromSocketAddress(&clientSocketAddress, senderIp);

		if ((user = getUserFromIp(senderIp)) == NULL) {
			continue;
		}

// user chat
		if (messageBuffer[0] == 'U') {
			addUserConversationElement(user, &myself, messageBuffer + 1);
		}
// group chat
		else if (messageBuffer[0] == 'G') {
			char *message = messageBuffer + 1;
			char *groupName;
			char *cypheredText;
			char clearTextMessage[MAX_CYPHERED_TEXT_MESSAGE_LENGTH];
			struct group *group = NULL;

			groupName = message;
			cypheredText = groupName;
			while (cypheredText[-1] != '|') {
				cypheredText += 1;
			}
			cypheredText[-1] = 0;

			struct group_collection_entry *gce = associatedGroups.oldest.younger;
			while (gce != NULL) {
				if (strcmp(gce->group.name, groupName) == 0) {
					group = &gce->group;
					break;
				}
				gce = gce->younger;
			}

			if (group != NULL) {
				decode(cypheredText, group->password, clearTextMessage);
				addGroupConversationElement(user, group, clearTextMessage);
			}
		}
// protocol error
		else {
			perror("receiveMessages protocol error");
			continue;
		}
	}
}

void getIpFromSocketAddress(struct sockaddr_in *socketAddress, char *ip) {
	inet_ntop(AF_INET, &socketAddress->sin_addr.s_addr, ip, MAX_IP_LENGTH);
}

struct user* getUserFromIp(char *senderIp) {
	struct user *user;
	int i;

	struct user_collection_entry *uce = discoveredUsers.oldest.younger;
	while (uce != NULL) {
		if (strcmp(uce->user.ip, senderIp) == 0) {
			return &uce->user;
		}
	}

	return NULL;
}

void handleFirstMessageFromUser(struct user *user, char *message) {
	struct conversation_element *newElement =
			(struct conversation_element *) malloc(
					sizeof(struct conversation_element));
	memset(newElement, 0, sizeof(struct conversation_element));
	strcpy(newElement->message, message);
	user->conv.first = user->conv.last = newElement;
	user->conv.messageCount = 1;
}

int getMaximumGroupsNo() {
	return sizeof(associatedGroups) / sizeof(struct group);
}

void joinGroup(char *groupName, char *password) {
	// look up existing group membership
	int alreadyMember = 0;
	struct group_collection_entry *gce = associatedGroups.oldest.younger;
	if (associatedGroups.entryQuantity > 0) {
		while (gce != &associatedGroups.youngest) {
			if (strcmp(gce->group.name, groupName) == 0) {
				strcpy(gce->group.password, password);
				alreadyMember = 1;
				break;
			}
			gce = gce->younger;
		}
	}

	// if not a member of the group, add them to it
	if (alreadyMember == 0) {
		gce = malloc(sizeof(struct group_collection_entry));

		associatedGroups.youngest.older->younger = gce;
		gce->younger = &associatedGroups.youngest;
		gce->older = associatedGroups.youngest.older;
		associatedGroups.youngest.older = gce;
		associatedGroups.entryQuantity++;
		strcpy(gce->group.name, groupName);
		strcpy(gce->group.password, password);
		pthread_mutex_init(&gce->lock, NULL);
	}
}

void leaveGroup(char *groupName) {
	// look up existing group membership
	int alreadyMember = 0;
	struct group_collection_entry *gce = associatedGroups.oldest.younger;
	while (gce != NULL) {
		if (strcmp(gce->group.name, groupName) == 0) {
			gce->older->younger = gce->younger;
			gce->younger->older = gce->older;
			associatedGroups.entryQuantity--;
			free(gce);
			break;
		}
		gce = gce->younger;
	}
}

/*
 * send a message to the user
 */
void sendMessageToPerson(char *message, struct user *user) {
	char realMessage[strlen(message) + 1];
	int realMessageLength = 1 + strlen(message) + 1;
	realMessage[0] = 'U';
	strcpy(realMessage + 1, message);
	realMessage[realMessageLength] = 0;

// the socket
	int senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// the socket address
	struct sockaddr_in senderSocketAddress;

// quit if the socket has problems
	if (senderSocket < 0) {
		perror("sendMessageToPerson socket");
		return;
	}

//set socket address
	senderSocketAddress.sin_family = AF_INET;
	senderSocketAddress.sin_port = htons(MESSAGE_PORT);
	if (inet_aton(user->ip, &senderSocketAddress.sin_addr) == 0) {
		perror("sendMessageToPerson inet_aton");
		return;
	}

// send the message
	sendto(senderSocket, realMessage, realMessageLength, 0,
			(struct sockaddr*) &senderSocketAddress,
			sizeof(senderSocketAddress));
}

void sendMessageToGroup(char *message, struct group *group) {
	char cypheredMessage[strlen(message) + 1];
	char finalMessage[1 + strlen(group->name) + 1 + strlen(message) + 1];
	char *var = finalMessage;
	var[0] = 'G';
	var += 1;
	strcpy(var, group->name);
	var += strlen(group->name);
	var[0] = '|';
	var += 1;
	encode(message, group->password, cypheredMessage);
	strcpy(var, cypheredMessage);

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
			// set message port
			bcastSocketAddress.sin_port = htons(MESSAGE_PORT);
			// send message
			sendto(bcastSocket, (char*) finalMessage,
					strlen((char*) finalMessage), 0,
					(struct sockaddr *) &bcastSocketAddress,
					sizeof(bcastSocketAddress));
		}
	}
// free memory: linked list
	freeifaddrs(ifs);
}
