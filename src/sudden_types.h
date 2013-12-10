#define MAX_USERNAME_LENGTH 50
#define MAX_GROUPNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 20
#define IP_LENGTH 15

struct user {
	char name[MAX_USERNAME_LENGTH + 1];
	char ip[IP_LENGTH + 1];
};

struct group {
	char name[MAX_GROUPNAME_LENGTH + 1];
	char password[MAX_PASSWORD_LENGTH + 1];
};
