/*
 * network.h
 *
 *  Created on: Dec 3, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#ifndef NETWORK_H_
#define NETWORK_H_

void discoverGroups();
void joinGroup(char *groupName, char *password);
void sendMessage(char *message, char *group);
int getMaximumGroupsNo();
struct group * listMemberGroups();

#endif /* NETWORK_H_ */
