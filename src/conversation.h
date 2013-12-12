/*
 * conversation.h
 *
 *  Created on: Dec 11, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#ifndef CONVERSATION_H_
#define CONVERSATION_H_

#include "sudden_types.h"

void addUserConversationElement(struct user *src, struct user *dst, char *message);

#endif /* CONVERSATION_H_ */
