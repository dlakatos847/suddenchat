/*
 * network.h
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "sudden_types.h"

/*
 * definitions
 */
#define INET_ADDRSTRLEN 16
#define HELLO_REQ_PORT 52000
#define HELLO_RSP_PORT 52001
#define MESSAGE_PORT 52002
#define MAX_DISCOVERED_GROUPS 10
#define MAX_NAME_LENGTH (MAX_USERNAME_LENGTH>MAX_GROUPNAME_LENGTH?MAX_USERNAME_LENGTH:MAX_GROUPNAME_LENGTH)
#define DISCOVERY_REQ_LENGTH 24
#define DISCOVERY_REQ_MESSAGE "SUDDENCHAT_DISCOVERY_REQ"
#define CONVERSATION_REQ_MESSAGE "CONVERSATION_REQ_MESSAGE"

void collectDiscoveryAnswers();
void answerDiscoveryRequests();
void discover();
void joinGroup(char *groupName, char *password);
void sendMessageToUser(char *message, struct user *user);
void sendMessageToGroup(char *message, struct group *group);
int getMaximumGroupsNo();
struct group * listMemberGroups();
void receiveMessages();
void cleanDiscoveryStructures();
void networkInit();
void networkDestroy();

#endif /* NETWORK_H_ */
