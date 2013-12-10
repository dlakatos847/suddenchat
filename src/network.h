/*
 * network.h
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#ifndef NETWORK_H_
#define NETWORK_H_

/*
 * definitions
 */
#define INET_ADDRSTRLEN 16
#define HELLO_REQ_PORT 52000
#define HELLO_RSP_PORT 52001
#define MESSAGE_PORT 52002
#define MAX_DISCOVERED_USERS 100
#define MAX_DISCOVERED_GROUPS 10
#define MAX_NAME_LENGTH (MAX_USERNAME_LENGTH>MAX_GROUPNAME_LENGTH?MAX_USERNAME_LENGTH:MAX_GROUPNAME_LENGTH)
#define DISCOVERY_REQ_LENGTH 24
#define DISCOVERY_REQ_MESSAGE "SUDDENCHAT_DISCOVERY_REQ"

void collectDiscoveryAnswers();
void joinGroup(char *groupName, char *password);
void sendMessageToGroup(char *message, char *group);
int getMaximumGroupsNo();
struct group * listMemberGroups();

#endif /* NETWORK_H_ */
