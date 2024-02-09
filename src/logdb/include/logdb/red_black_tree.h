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

#ifndef __LIBLOGDB_RED_BLACK_TREE_H__
#define __LIBLOGDB_RED_BLACK_TREE_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifndef DATA_TYPE
#define DATA_TYPE void *
#endif

#include <stdint.h>
#include <stdio.h>

#include <logdb/misc.h>
#include <logdb/stack.h>

/*  CONVENTIONS:  All data structures for red-black trees have the prefix */
/*                "rb_" to prevent name conflicts. */
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

/* comment out the line below to remove all the debugging assertion */
/* checks from the compiled code.  */
#define DEBUG_ASSERT 1

typedef struct rb_red_blk_node {
  void *key;
  void *info;
  int red; /* if red=0 then the node is black */
  struct rb_red_blk_node *left;
  struct rb_red_blk_node *right;
  struct rb_red_blk_node *parent;
} rb_red_blk_node;

/* Compare(a,b) should return 1 if *a > *b, -1 if *a < *b, and 0 otherwise */
/* Destroy(a) takes a pointer to whatever key might be and frees it accordingly
 */
typedef struct rb_red_blk_tree {
  int (*Compare)(const void *a, const void *b);
  void (*DestroyKey)(void *a);
  void (*DestroyInfo)(void *a);
  void (*PrintKey)(const void *a);
  void (*PrintInfo)(void *a);
  /*  A sentinel is used for root and for nil.  These sentinels are */
  /*  created when RBTreeCreate is caled.  root->left should always */
  /*  point to the node which is the root of the tree.  nil points to a */
  /*  node which should always be black but has aribtrary children and */
  /*  parent and no key or info.  The point of using these sentinels is so */
  /*  that the root and nil nodes do not require special cases in the code */
  rb_red_blk_node *root;
  rb_red_blk_node *nil;
  rb_red_blk_node* it;
  int it_node;
} rb_red_blk_tree;

rb_red_blk_tree *RBTreeCreate(int (*CompFunc)(const void *, const void *),
                              void (*DestFunc)(void *),
                              void (*InfoDestFunc)(void *),
                              void (*PrintFunc)(const void *),
                              void (*PrintInfo)(void *));
rb_red_blk_node *RBTreeInsert(rb_red_blk_tree *, void *key, void *info);
void RBTreePrint(rb_red_blk_tree *);
void RBDelete(rb_red_blk_tree *, rb_red_blk_node *);
void RBTreeDestroy(rb_red_blk_tree *);
rb_red_blk_node *TreePredecessor(rb_red_blk_tree *, rb_red_blk_node *);
rb_red_blk_node *TreeSuccessor(rb_red_blk_tree *, rb_red_blk_node *);
rb_red_blk_node *RBExactQuery(rb_red_blk_tree *, void *);
stk_stack *RBEnumerate(rb_red_blk_tree *tree, void *low, void *high);
size_t rbtree_count(rb_red_blk_tree* tree);
void NullFunction(void *);

void checkRep(rb_red_blk_tree *tree);
void rbtree_it_reset(rb_red_blk_tree *tree);
rb_red_blk_node* rbtree_enumerate_next(rb_red_blk_tree* tree);

LIBDOGECOIN_END_DECL

#endif /* __LIBLOGDB_RED_BLACK_TREE_H__ */
