/*
Stack header file

Author: Edvin Lindholm
CS-user: c19elm@cs.umu.se
Date: lör 10 okt 2020
*/

#ifndef STACK_H

// Includes, typedef
#include <stdbool.h>
typedef struct stack stack;

/**
 * Creates a new empty stack.
 *
 * @return			Pointer to the stack.
 */
stack *stack_create(void);

/**
 * Checks if a stack is empty.
 *
 * @param s			Pointer to the stack.
 * @return			True if empty otherwise false.
 */
bool stack_empty(const stack *s);

/**
 * Pushes element onto stack.
 *
 * @param s			Pointer to the stack.
 * @param v 		Pointer to value to element.
 * @return			Pointer to the stack.
 */
stack *stack_push(stack *s, char *v);

/**
 * Frees memory allocated for element and pops it of from stack.
 *
 * @param s			Pointer to the stack.
 * @return			Pointer to the stack.
 */
char *stack_pop(stack *s);

/**
 * Checks value of top element in stack.
 *
 * @param s			Pointer to the stack.
 * @return			Value of the element.
 */
char *stack_top(const stack *s);

/**
 * Returns All memory allocated for stack.
 *
 * @param s			Pointer to the stack.
 */
void stack_kill(stack *s);

#endif
