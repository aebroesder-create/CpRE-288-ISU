/*
 * L5UART.h
 *
 *  Created on: Mar 2, 2026
 *      Author: alecbroe
 */

#ifndef L5UART_H_
#define L5UART_H_

void uart_init(void);
void uart_sendChar(char data);
char uart_receive(void);
void uart_sendStr(const char *data);

#endif /* L5UART_H_ */
