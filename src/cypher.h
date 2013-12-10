/*
 * cypher.h
 *
 *  Created on: Dec 10, 2013
 *      Author: David Lakatos <david.lakatos.hu@gmail.com>
 */

#ifndef CYPHER_H_
#define CYPHER_H_

void encode(char* clearText, char* password, char* cypheredText);
void decode(char* cypheredText, char* password, char* clearText);

#endif /* CYPHER_H_ */
