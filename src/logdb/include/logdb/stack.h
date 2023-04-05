/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
 Copyright (c) 2023 bluezr
 Copyright (c) 2023 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef __LIBLOGDB_STACK_H__
#define __LIBLOGDB_STACK_H__

#include <logdb/misc.h>

/*  CONVENTIONS:  All data structures for stacks have the prefix */
/*                "stk_" to prevent name conflicts. */
/*                                                                      */
/*                Function names: Each word in a function name begins with */
/*                a capital letter.  An example funcntion name is  */
/*                CreateRedTree(a,b,c). Furthermore, each function name */
/*                should begin with a capital letter to easily distinguish */
/*                them from variables. */
/*                                                                     */
/*                Variable names: Each word in a variable name begins with */
/*                a capital letter EXCEPT the first letter of the variable */
/*                name.  For example, int newLongInt.  Global variables have */
/*                names beginning with "g".  An example of a global */
/*                variable name is gNewtonsConstant. */

/*  if DATA_TYPE is undefined then stack.h and stack.c will be code for */
/*  stacks of void *, if they are defined then they will be stacks of the */
/*  appropriate data_type */

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#ifndef DATA_TYPE
#define DATA_TYPE void *
#endif

#include <stdint.h>
#include <stdio.h>

typedef struct stk_stack_node {
  DATA_TYPE info;
  struct stk_stack_node *next;
} stk_stack_node;

typedef struct stk_stack {
  stk_stack_node *top;
  stk_stack_node *tail;
} stk_stack;

stk_stack *StackJoin(stk_stack *stack1, stk_stack *stack2);
stk_stack *StackCreate();
void StackPush(stk_stack *theStack, DATA_TYPE newInfoPointer);
void *StackPop(stk_stack *theStack);
int StackNotEmpty(stk_stack *);
void StackDestroy(stk_stack *theStack,void DestFunc(void * a));

LIBDOGECOIN_END_DECL

#endif /* __LIBLOGDB_STACK_H__ */
