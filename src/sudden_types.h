#ifndef SUDDEN_TYPES_H_
#define SUDDEN_TYPES_H_

#include <time.h>
#include <pthread.h>

#define MAX_USERNAME_LENGTH 50
#define MAX_GROUPNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 20
#define MAX_IP_LENGTH 15
#define MAX_CONVERSATION_ELEMENT 5
#define MAX_MESSAGE_LENGTH sizeof('G')+MAX_GROUPNAME_LENGTH+sizeof('|')+MAX_CYPHERED_TEXT_MESSAGE_LENGTH
#define MAX_CYPHERED_TEXT_MESSAGE_LENGTH 500
#define MAX_JOINED_GROUPS 5
#define MAX_DISCOVERED_USERS 10
#define MAX_DISCOVERED_GROUPS 10
#define DISCOVERY_TIMEOUT 10

/*
 * declarations
 */
struct user;
struct group;
struct conversation_element;
struct conversation;
struct group_conversation;

struct conversation_element {
	struct user *src;
	struct user *dstUser;
	struct group *dstGroup;
	char message[MAX_MESSAGE_LENGTH + 1];
	int messageId;
	struct conversation_element *next;
	struct conversation_element *prev;
	struct tm timestamp;
	int isUnread;
};

struct conversation {
	pthread_mutex_t lock;
	int messageCount;
	int unreadMessage;
	struct conversation_element *first;
	struct conversation_element *last;
};

struct user {
	char name[MAX_USERNAME_LENGTH + 1];
	char ip[MAX_IP_LENGTH + 1];
	struct conversation conv;
};

struct group {
	char name[MAX_GROUPNAME_LENGTH + 1];
	char password[MAX_PASSWORD_LENGTH + 1];
	struct conversation conv;
};

struct user_collection_entry {
	pthread_mutex_t lock;
	struct user user;
	time_t updateTime;
	struct user_collection_entry *older;
	struct user_collection_entry *younger;
};

struct user_collection {
	pthread_mutex_t lock;
	int entryQuantity;
	struct user_collection_entry oldest;
	struct user_collection_entry youngest;
};

struct group_collection_entry {
	pthread_mutex_t lock;
	struct group group;
	time_t updateTime;
	struct group_collection_entry *older;
	struct group_collection_entry *younger;
};

struct group_collection {
	pthread_mutex_t lock;
	int entryQuantity;
	struct group_collection_entry oldest;
	struct group_collection_entry youngest;
};

#endif
