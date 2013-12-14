/*
 * conversation.c
 *
 *  Created on: Dec 11, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "sudden_types.h"

extern struct user myself;
extern struct user *activeChatBuddy;
extern struct group *activeChatGroup;

void addUserConversationElement(struct user *src, struct user *dst,
		char *message) {
	struct user *user = (src == &myself ? dst : src);
	struct conversation *conv = &(user->conv);
	time_t now = time(0);
	struct tm* tm_now = localtime(&now);

	pthread_mutex_lock(&conv->lock);

	struct conversation_element *newElement =
			(struct conversation_element *) malloc(
					sizeof(struct conversation_element));
	memset(newElement, 0, sizeof(struct conversation_element));
	strcpy(newElement->message, message);
	newElement->src = src;
	newElement->dstUser = dst;
	memcpy(&newElement->timestamp, tm_now, sizeof(struct tm));

	// if first message
	if (conv->last == NULL) {
		conv->first = conv->last = newElement;
		conv->messageCount = 1;
		conv->unreadMessage = 1;
	}
	// otherwise
	else {
		// if the conversation message count hit maximum
		if (conv->messageCount >= MAX_CONVERSATION_ELEMENT) {
			struct conversation_element *deleteObject = conv->first;
			conv->first = conv->first->next;
			conv->messageCount--;
			free(deleteObject);
		}

		conv->last->next = newElement;
		conv->messageCount++;
		conv->last = newElement;
		conv->unreadMessage++;
	}

	if (src != &myself && src != activeChatBuddy) {
		showIncomingMessageNotificationFromUser(user);
	}

	pthread_mutex_unlock(&conv->lock);
}

void addGroupConversationElement(struct user *src, struct group *dst,
		char *message) {
	struct conversation *conv = &(dst->conv);
	time_t now = time(0);

	// check if it's a duplicate
	if (conv->last != NULL && strcmp(message, conv->last->message) == 0) {
		return;
	}

	pthread_mutex_lock(&conv->lock);

	struct conversation_element *newElement =
			(struct conversation_element *) malloc(
					sizeof(struct conversation_element));
	memset(newElement, 0, sizeof(struct conversation_element));
	strcpy(newElement->message, message);
	newElement->src = src;
	newElement->dstUser = NULL;
	newElement->dstGroup = dst;
	memcpy(&newElement->timestamp, localtime(&now), sizeof(struct tm));

	// if first message
	if (conv->last == NULL) {
		conv->first = conv->last = newElement;
		conv->messageCount = 1;
		conv->unreadMessage = 1;
	}
	// otherwise
	else {
		// network duplicate
		if (strcmp(conv->last->message, message) == 0) {
			pthread_mutex_unlock(&conv->lock);
			return;
		}
		// if the conversation message count hit maximum
		if (conv->messageCount >= MAX_CONVERSATION_ELEMENT) {
			struct conversation_element *deleteObject = conv->first;
			conv->first = conv->first->next;
			conv->messageCount--;
			free(deleteObject);
		}

		conv->last->next = newElement;
		conv->messageCount++;
		conv->last = newElement;
		conv->unreadMessage++;
	}

	if (src != &myself && dst != activeChatGroup) {
		showIncomingMessageNotificationToGroup(dst);
	}

	pthread_mutex_unlock(&conv->lock);
}
