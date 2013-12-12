/*
 * cypher.c
 *
 *  Created on: Dec 10, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include <stdlib.h>
#include <string.h>

void encode(char *clearText, char *password, char *cypheredText) {
	int i;
	for (i = 0; i < strlen(clearText); ++i) {
		cypheredText[i] = clearText[i] ^ password[i % strlen(password)];
	}
	cypheredText[strlen(clearText)] = 0;
}

void decode(char *cypheredText, char *password, char *clearText) {
	int i;
	for (i = 0; i < strlen(cypheredText); ++i) {
		clearText[i] = cypheredText[i] ^ password[i % strlen(password)];
	}
	clearText[strlen(cypheredText)] = 0;
}
