/*
Simple stack implementation

Author: Edvin Lindholm
CS-user: c19elm@cs.umu.se
Date: lör 10 okt 2020
*/


// Includes
#include <stdlib.h>
#include <stdio.h>
#include "stack.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stack.h"

struct stack {
	int top;
    char **array;
};
/**
 * stack_empty(void) - Create an empty stack.
 *
 * Returns: A pointer to the new stack.
 */
stack *stack_create(void) {
    stack *s = calloc(1,sizeof(stack));
	if (s == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
    s->top = 0;
	return s;
}
/**
 * stack_empty(stack *s) - Check if a stack is empty.
 * @s: Stack to check.
 *
 * Returns: True if stack is empty, otherwise false.
 */

bool stack_empty(const stack *s) {
	return (s->top == 0);
}
/**
 * stack_push(stack *s, char* str) - adds a char to the stack

 * @s: stack to push to
 * @str: the string that will be pushed
 *
 * Returns: pointer to the new stack
*/
stack *stack_push(stack *s, char* str) {
    s->top = s->top + 1;
    s->array = realloc(s->array, s->top * sizeof(char*));
	if (s->array == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
    s->array[s->top-1] = str;
    return s;
}
/**
 * stack_pop(stack *s) - pops the top element of the stack and returns it
 * @s: stack to pop
 *
 * Return the top element of the stack
*/
char *stack_pop(stack *s) {
    char* str = s->array[s->top-1];
    s->top--;
    s->array = realloc(s->array,sizeof(char*)*s->top);
    return str;
}
/**
 * stack_top(stack *s) - Return the top value
 * @s: stack to check
 *
 * Returns the top element of the stack
*/
char *stack_top(const stack *s) {
	char *strCpy = malloc(sizeof(char*));
	if (strCpy == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	strcpy(strCpy,s->array[s->top-1]);
    return strCpy;
}

/**
 * stack_kill(stack *s) - Destroy a given stack.
 * @s: Stack to destroy.
 *
 * Return all dynamic memory used by the stack and its elements.
 *
 * Returns: Nothing.
 */
void stack_kill(stack *s) {
	while (!stack_empty(s)) {
		char *str = stack_pop(s);
		free(str);
	}
    free(s->array);
	free(s);
}
