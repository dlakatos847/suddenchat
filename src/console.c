/*
 * console.c
 *
 *  Created on: Okt 17, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "network.h"
#include "sudden_types.h"
#include "conversation.h"

int inChatWindow = 0;
struct user *activeChatBuddy;
struct group *activeChatGroup;
extern struct user_collection discoveredUsers;
extern struct group_collection discoveredGroups;
extern struct user myself;
extern struct group_collection associatedGroups;

void showIncomingMessageNotificationFromUser(struct user *user);
void showIncomingMessageNotificationToGroup(struct group *group);
void showOptions();
void listGroupMemberships();
void listCandidates();
void chatWithUser();
void chatWithGroup();
void listUsers();
void listGroups();
void refreshChatWindow(struct conversation *conv);
void showUnreadMessages();

void showConsole() {
	int running = 1;
	char choice[2];

	char groupName[MAX_GROUPNAME_LENGTH];
	char password[MAX_PASSWORD_LENGTH];

	while (running) {
		fflush(stdin);

		showOptions();
		gets(choice);
		switch (choice[0]) {
		case 'q':
			running = 0;
			break;
		case 'j':
			fflush(stdin);

			printf("Group's name:\n");
			gets(groupName);

			printf("Password:\n");
			gets(password);

			joinGroup(groupName, password);
			break;
		case 'g':
			listGroupMemberships();
			break;
		case 'l':
			listCandidates();
			break;
		case 'c':
			chatWithUser();
			break;
		case 'n':
			chatWithGroup();
			break;
		default:
			printf("Not valid input\n\n");
			break;
		}
	}
}

void showOptions() {
	printf("Please select from the following options!\n");
	printf("c - start chat with a user\n");
	printf("n - start chat with a group\n");
	printf("j - join a group\n");
	printf("g - show group memberships\n");
	printf("l - list available users and groups\n");
	printf("q - quit\n");
}

void listGroupMemberships() {
	pthread_mutex_lock(&associatedGroups.lock);

	printf("Groups:\n");
	if (associatedGroups.entryQuantity > 0) {
		struct group_collection_entry *gce = associatedGroups.oldest.younger;
		while (gce != &associatedGroups.youngest) {
			printf("Name: %s, password: %s\n", gce->group.name,
					gce->group.password);
			gce = gce->younger;
		}
	}

	pthread_mutex_unlock(&associatedGroups.lock);
}

void listCandidates() {
	listUsers();
	listGroups();
}

void listUsers() {
	pthread_mutex_lock(&discoveredUsers.lock);

	printf("Users:\n");
	if (discoveredUsers.entryQuantity > 0) {
		struct user_collection_entry *uce = discoveredUsers.oldest.younger;
		while (uce != &discoveredUsers.youngest) {
			printf(" # %-10s ", uce->user.name);
			printf("\t\t%s\n", uce->user.ip);
			uce = uce->younger;
		}
	}

	pthread_mutex_unlock(&discoveredUsers.lock);
}

void listGroups() {
	pthread_mutex_lock(&discoveredGroups.lock);

	printf("Groups:\n");
	if (discoveredGroups.entryQuantity > 0) {
		struct group_collection_entry *gce = discoveredGroups.oldest.younger;
		while (gce != &discoveredGroups.youngest) {
			printf(" # %s\n", gce->group.name);
			gce = gce->younger;
		}
	}

	pthread_mutex_unlock(&discoveredGroups.lock);
}

void chatWithUser() {
	char username[MAX_USERNAME_LENGTH];
	struct user *user = NULL;
	char message[MAX_MESSAGE_LENGTH + 1];

	if (discoveredUsers.entryQuantity > 0) {
		struct user_collection_entry *uce = discoveredUsers.oldest.younger;

		printf("Select user:\n");
		listUsers();

		gets(username);
		while (uce != NULL) {
			if (strcmp(username, uce->user.name) == 0) {
				user = &uce->user;
				break;
			}
			uce = uce->younger;
		}

		if (user == NULL) {
			printf("Invalid user!\n");
			return;
		} else {
			activeChatBuddy = user;
		}

		inChatWindow = 1;

		struct conversation *conv = &(user->conv);
		struct conversation_element *ce = user->conv.first;
		if (ce != NULL) {
			printf("History:\n");
			while (ce != NULL) {
				char time[20];
				strftime(time, sizeof(time), "%H:%M", &ce->timestamp);
				printf("[%s %s]: %s\n", time, ce->src->name, ce->message);
				ce = ce->next;
			}
			conv->unreadMessage = 0;
		}

		pthread_t chatWindowRefresher;
		if (pthread_create(&chatWindowRefresher, NULL,
				(void*) refreshChatWindow, &(user->conv)) < 0) {
			perror("main pthread_create");
			return;
		}

		printf("Exit character is \\q\n");
		fflush(stdin);
		while (inChatWindow) {
			gets(message);
			if (strcmp(message, "\\q") == 0) {
				inChatWindow = 0;
				activeChatBuddy = NULL;
				showUnreadMessages();
			} else {
				addUserConversationElement(&myself, user, message);
				sendMessageToPerson(message, user);
			}
		}
	} else {
		printf("No users present\n");
	}
}

void chatWithGroup() {
	char groupname[MAX_GROUPNAME_LENGTH + 1];
	struct group *group = NULL;
	char message[MAX_MESSAGE_LENGTH + 1];

	if (discoveredGroups.entryQuantity > 0) {
		struct group_collection_entry *gce = discoveredGroups.oldest.younger;

		printf("Select group:\n");
		listGroups();

		gets(groupname);
		while (gce != &discoveredGroups.youngest) {
			if (strcmp(groupname, gce->group.name) == 0) {
				group = &gce->group;
				break;
			}
			gce = gce->younger;
		}

		if (group == NULL) {
			printf("The group %s does not exist", groupname);
			return;
		}

		group = NULL;
		gce = associatedGroups.oldest.younger;
		while (gce != &associatedGroups.youngest) {
			if (strcmp(groupname, gce->group.name) == 0) {
				group = &gce->group;
				break;
			}
			gce = gce->younger;
		}

		if (group == NULL) {
			printf("Missing password\n");
			return;
		}

		inChatWindow = 1;

		struct conversation *conv = &(group->conv);
		struct conversation_element *ce = conv->first;
		if (ce != NULL) {
			printf("History:\n");
			while (ce != NULL) {
				char time[20];
				strftime(time, sizeof(time), "%H:%M", &ce->timestamp);
				printf("[%s %s->%s]: %s\n", time, ce->src->name,
						ce->dstGroup->name, ce->message);
				ce = ce->next;
			}
			conv->unreadMessage = 0;
		}

		pthread_t chatWindowRefresher;
		if (pthread_create(&chatWindowRefresher, NULL,
				(void*) refreshChatWindow, conv) < 0) {
			perror("main pthread_create");
			return;
		}

		printf("Exit character is \\q\n");
		fflush(stdin);
		while (inChatWindow) {
			gets(message);
			if (strcmp(message, "\\q") == 0) {
				inChatWindow = 0;
				activeChatGroup = NULL;
				showUnreadMessages();
			} else {
				addGroupConversationElement(&myself, group, message);
				sendMessageToGroup(message, group);
			}
		}
	} else {
		printf("No groups present\n");
	}
}

void refreshChatWindow(struct conversation *conv) {
	while (inChatWindow) {
		if ((conv->last == NULL) || (conv->unreadMessage <= 0)) {
			sleep(1);
		} else {
			int i;
			struct conversation_element *ce = conv->last;

			pthread_mutex_lock(&conv->lock);

			for (i = conv->unreadMessage - 1; i > 0; --i) {
				ce = ce->prev;
			}
			char time[20];
			strftime(time, sizeof(time), "%H:%M", &ce->timestamp);
			printf("[%s %s->%s]: %s\n", time, ce->src->name,
					ce->dstUser == NULL ?
							ce->dstGroup->name : ce->dstUser->name,
					ce->message);
			conv->unreadMessage = 0;

			pthread_mutex_lock(&conv->lock);
		}
	}
}

void showIncomingMessageNotificationFromUser(struct user *user) {
	if (user->conv.unreadMessage < 2) {
		printf(" * %d unread message from user %s\n", user->conv.unreadMessage,
				user->name);
	} else {
		printf(" * %d unread messages from user %s\n", user->conv.unreadMessage,
				user->name);
	}
}

void showIncomingMessageNotificationToGroup(struct group *group) {
	if (group->conv.unreadMessage < 2) {
		printf(" * %d unread message to group %s\n", group->conv.unreadMessage,
				group->name);
	} else {
		printf(" * %d unread messages to group %s\n", group->conv.unreadMessage,
				group->name);
	}
}

void showUnreadMessages() {
	if (discoveredUsers.entryQuantity > 0) {
		struct user_collection_entry *uce = discoveredUsers.oldest.younger;
		while (uce != NULL) {
			if (uce->user.conv.unreadMessage > 0) {
				showIncomingMessageNotificationFromUser(&uce->user);
			}
			uce = uce->younger;
		}
	}

	if (discoveredGroups.entryQuantity > 0) {
		struct group_collection_entry *gce = discoveredGroups.oldest.younger;
		while (gce != NULL) {
			if (gce->group.conv.unreadMessage > 0) {
				showIncomingMessageNotificationToGroup(&gce->group);
			}
			gce = gce->younger;
		}
	}
}
