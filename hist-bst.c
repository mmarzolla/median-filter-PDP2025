/****************************************************************************
 *
 * hist-bst.c -- Dynamic histogram based on BST
 *
 * Copyright (C) 2021--2025 Moreno Marzolla
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

/*
 * Implementation of histograms using augmented Binary Search Trees.
 * This implementation stores the histogram values in a BST, where
 * each node contains a pair (key, count) representing the fact that
 * there are `count` occurrences of `key` in the histogram. The BST is
 * NOT kept balanced. Each node is augmented with the counter of the
 * number of occurrences of all keys in the subtree rooted at that
 * node. This allows computation of the median in time proportional to
 * the height of the tree, which is O(log n) in the average case.
 *
 * The cost of the operations is as follows (n is the number of unique
 * keys in the tree):
 *
 * - insertion O(log n) on average
 * - deletion O(log n) on average
 * - median computation O(log n) on average
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include "hist.h"

typedef struct HistNode {
    data_t key;
    int count; /* number of occurrences of "key" */
    int counts; /* number of occorrences of all keys in the subtree rooted at this node */
    struct HistNode *parent, *left, *right;
} HistNode;

struct Hist {
    HistNode *root;
};

#ifndef NDEBUG
static void hist_check_rec( const HistNode *n )
{
    if (n != NULL) {
        int c = n->count;
        if (n->left) {
            c += n->left->counts;
            assert(n->left->key <= n->key);
            assert(n->left->parent == n);
            hist_check_rec(n->left);
        }
        if (n->right) {
            c += n->right->counts;
            assert(n->right->key > n->key);
            assert(n->right->parent == n);
            hist_check_rec(n->right);
        }
        assert(c == n->counts);
    }
}
#endif

static void hist_check( const Hist* H )
{
    return ;
#ifndef NDEBUG
    if (H->root)
        assert(H->root->parent == NULL);
    /*
    printf("\n\nhist_check\n");
    hist_pretty_print(H);
    printf("----------\n\n");
    */
    hist_check_rec(H->root);
#endif
}


/* return 1 if count changed, 0 if not */
static int update_counts( HistNode *n )
{
    if (n == NULL)
        return 0;
    else {
        const int old_counts = n->counts;
        int new_counts = n->count;
        if (n->left != NULL) new_counts += n->left->counts;
        if (n->right != NULL) new_counts += n->right->counts;
        n->counts = new_counts;
        /* printf("Key %" PRIu32 " count=%d old_counts=%d new_counts=%d\n", n->key, n->count, old_counts, new_counts); */
        return (old_counts != new_counts);
    }
}

static void update_counts_to_root( HistNode *n )
{
    while (update_counts(n)) {
        n = n->parent;
    }
}


static HistNode *hist_new_node( data_t k, int count,
                                HistNode *parent,
                                HistNode *left, HistNode *right)
{
    HistNode *n = (HistNode*)malloc(sizeof(*n));
    assert(n != NULL);
    n->key = k;
    n->count = count;
    n->counts = -1; /* ATTENTION: this is intentionally different than n->counts */
    n->left = left;
    n->right = right;
    n->parent = parent;
    return n;
}

Hist *hist_create( void )
{
    Hist *H = (Hist*)malloc(sizeof(*H));
    assert(H != NULL);

    H->root = NULL;
    return H;
}

static void hist_clear_rec(HistNode *n)
{
    if (n != NULL) {
        hist_clear_rec(n->left);
        hist_clear_rec(n->right);
        free(n);
    }
}

void hist_clear(Hist *H)
{
    assert(H != NULL);

    hist_clear_rec(H->root);
    H->root = NULL;
    hist_check(H);
}

void hist_destroy(Hist *H)
{
    hist_clear(H);
    free(H);
}


/* Insert c>=0 additional instances of key `k` in the subtree rooted at
   `n`. */
static HistNode *hist_insert_rec(HistNode *n, HistNode *p, data_t k, int c)
{
    if (n == NULL) {
        n = hist_new_node(k, c, p, NULL, NULL);
    } else {
        if (k < n->key) {
            n->left = hist_insert_rec(n->left, n, k, c);
        } else if (k > n->key) {
            n->right = hist_insert_rec(n->right, n, k, c);
        } else {
            n->count += c;
        }
    }
    /* it is not possible to call update_counts_to_root() since node
       might be not yet connected to the tree */
    update_counts(n);
    return n;
}

/* Insert c>=0 new occurrences of key `k` in the histogram */
void hist_insert(Hist *H, data_t k, int c)
{
    assert(H != NULL);
    assert(c>=0);

    if (c > 0) {
        H->root = hist_insert_rec(H->root, NULL, k, c);
        /* hist_pretty_print(H); */
        hist_check(H);
    }
}

/* Return a pointer to the node containing `v`, or NULL */
static HistNode *hist_lookup(const Hist *H, data_t v)
{
    HistNode *n = H->root;
    while (n != NULL && n->key != v) {
        if (v < n->key)
            n = n->left;
        else
            n = n->right;
    }
    return n;
}

static HistNode *hist_minimum(HistNode *n)
{
    assert(n != NULL);

    while (n->left != NULL) {
        n = n->left;
    }
    return n;
}

int hist_get(const Hist *H, data_t k)
{
    HistNode *n = hist_lookup(H, k);
    return (n == NULL ? 0 : n->count);
}

static void hist_transplant(Hist *T, HistNode *u, HistNode *v)
{
    assert(T != NULL);
    assert(u != NULL);

    if (u->parent == NULL) {
        T->root = v;
    } else {
        if (u == u->parent->left) {
            u->parent->left = v;
        } else {
            u->parent->right = v;
        }
    }
    if (v != NULL) {
        v->parent = u->parent;
    }
}

/* remove c>=0 occurrences of key v from the histogram. There must be at
   least c occurrence of v in the histogram. */
void hist_delete(Hist *H, data_t v, int c)
{
    HistNode *n = hist_lookup(H, v);
    assert(c>=0);

    if (n == NULL)
        return;

    n->count -= c;
    assert(n->count >= 0);

    if (n->count > 0)
        update_counts_to_root(n);
    else {
        HistNode *update_from;

        if (n->left == NULL) {
            update_from = (n->parent != NULL ? n->parent : n->right);
            hist_transplant(H, n, n->right);
        } else if (n->right == NULL) {
            update_from = (n->parent != NULL ? n->parent : n->left);
            hist_transplant(H, n, n->left);
        } else {
            HistNode *min_of_right = hist_minimum(n->right);
            update_from = min_of_right;
            assert(min_of_right != NULL);
            if (min_of_right->parent != n) {
                update_from = min_of_right->parent;
                hist_transplant(H, min_of_right, min_of_right->right);
                min_of_right->right = n->right;
                min_of_right->right->parent = min_of_right;
            }
            hist_transplant(H, n, min_of_right);
            min_of_right->left = n->left;
            min_of_right->left->parent = min_of_right;
        }
        free(n);
        update_counts_to_root(update_from);
    }
    hist_check(H);
}

static void hist_print_rec( const HistNode *n )
{
    if (n != NULL) {
        hist_print_rec(n->left);
        printf("val = %" PRIu32 " count = %d\n", n->key, n->count);
        hist_print_rec(n->right);
    }
}

void hist_print( const Hist *H )
{
    assert(H != NULL);

    hist_print_rec(H->root);
}

static void hist_pretty_print_rec( const HistNode *n, int depth )
{
    if (n != NULL) {
        int i;
        hist_pretty_print_rec(n->right, depth+1);
        for (i=0; i<depth; i++) {
            printf("   ");
        }
        printf("%" PRIu32 "[%d,%d]\n", n->key, n->count, n->counts);
        hist_pretty_print_rec(n->left, depth+1);
    }
}

void hist_pretty_print( const Hist *H )
{
    assert(H != NULL);
    hist_pretty_print_rec(H->root, 0);
}

int hist_is_empty(const Hist *H)
{
    assert(H != NULL);

    return ( (H->root == NULL) || (H->root->counts == 0) );
}

void hist_add_rec( Hist *H, const HistNode *n)
{
    if (n != NULL) {
        hist_insert(H, n->key, n->count);
        hist_add_rec(H, n->left);
        hist_add_rec(H, n->right);
    }
}

void hist_add(Hist *H1, const Hist *H2)
{
    hist_add_rec(H1, H2->root);
}


void hist_sub_rec( Hist *H, const HistNode *n)
{
    if (n != NULL) {
        hist_delete(H, n->key, n->count);
        hist_sub_rec(H, n->left);
        hist_sub_rec(H, n->right);
    }
}

void hist_sub(Hist *H1, const Hist *H2)
{
    hist_sub_rec(H1, H2->root);
}

data_t hist_median(const Hist *H)
{
    int target;
    const HistNode *n = H->root;

    assert(n != NULL); /* can not find median of empty set */

    target = H->root->counts / 2;
    while (1) {
        int counts_left = 0;
        assert(n != NULL);
        assert(n->counts >= target);
        /*
        printf("target=%d count=%d counts=%d left_counts=%d right_counts=%d\n",
               target,
               n->count,
               n->counts,
               (n->left ? n->left->counts : -1),
               (n->right ? n->right->counts : -1));
        */
        if (n->left != NULL)
            counts_left = n->left->counts;

        if (counts_left > target)
            n = n->left;
        else if (target < counts_left + n->count)
            return n->key;
        else {
            target -= counts_left;
            target -= n->count;
            n = n->right;
        }
    }
}
