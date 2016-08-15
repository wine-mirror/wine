/*
 * Red-black search tree support
 *
 * Copyright 2009 Henri Verbeet
 * Copyright 2009 Andrew Riedi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_RBTREE_H
#define __WINE_WINE_RBTREE_H

#define WINE_RB_ENTRY_VALUE(element, type, field) \
    ((type *)((char *)(element) - offsetof(type, field)))

struct wine_rb_entry
{
    struct wine_rb_entry *parent;
    struct wine_rb_entry *left;
    struct wine_rb_entry *right;
    unsigned int flags;
};

struct wine_rb_stack
{
    struct wine_rb_entry ***entries;
    size_t count;
    size_t size;
};

struct wine_rb_functions
{
    void *(*alloc)(size_t size);
    void *(*realloc)(void *ptr, size_t size);
    void (*free)(void *ptr);
    int (*compare)(const void *key, const struct wine_rb_entry *entry);
};

struct wine_rb_tree
{
    const struct wine_rb_functions *functions;
    struct wine_rb_entry *root;
    struct wine_rb_stack stack;
};

typedef void (wine_rb_traverse_func_t)(struct wine_rb_entry *entry, void *context);

#define WINE_RB_FLAG_RED                0x1
#define WINE_RB_FLAG_STOP               0x2

static inline void wine_rb_stack_clear(struct wine_rb_stack *stack)
{
    stack->count = 0;
}

static inline void wine_rb_stack_push(struct wine_rb_stack *stack, struct wine_rb_entry **entry)
{
    stack->entries[stack->count++] = entry;
}

static inline int wine_rb_ensure_stack_size(struct wine_rb_tree *tree, size_t size)
{
    struct wine_rb_stack *stack = &tree->stack;

    if (size > stack->size)
    {
        size_t new_size = stack->size << 1;
        struct wine_rb_entry ***new_entries = tree->functions->realloc(stack->entries,
                new_size * sizeof(*stack->entries));

        if (!new_entries) return -1;

        stack->entries = new_entries;
        stack->size = new_size;
    }

    return 0;
}

static inline int wine_rb_is_red(struct wine_rb_entry *entry)
{
    return entry && (entry->flags & WINE_RB_FLAG_RED);
}

static inline void wine_rb_rotate_left(struct wine_rb_tree *tree, struct wine_rb_entry *e, int change_color)
{
    struct wine_rb_entry *right = e->right;

    if (!e->parent)
        tree->root = right;
    else if (e->parent->left == e)
        e->parent->left = right;
    else
        e->parent->right = right;

    e->right = right->left;
    if (e->right) e->right->parent = e;
    right->left = e;
    right->parent = e->parent;
    e->parent = right;
    if (change_color)
    {
        right->flags &= ~WINE_RB_FLAG_RED;
        right->flags |= e->flags & WINE_RB_FLAG_RED;
        e->flags |= WINE_RB_FLAG_RED;
    }
}

static inline void wine_rb_rotate_right(struct wine_rb_tree *tree, struct wine_rb_entry *e, int change_color)
{
    struct wine_rb_entry *left = e->left;

    if (!e->parent)
        tree->root = left;
    else if (e->parent->left == e)
        e->parent->left = left;
    else
        e->parent->right = left;

    e->left = left->right;
    if (e->left) e->left->parent = e;
    left->right = e;
    left->parent = e->parent;
    e->parent = left;
    if (change_color)
    {
        left->flags &= ~WINE_RB_FLAG_RED;
        left->flags |= e->flags & WINE_RB_FLAG_RED;
        e->flags |= WINE_RB_FLAG_RED;
    }
}

static inline void wine_rb_flip_color(struct wine_rb_entry *entry)
{
    entry->flags ^= WINE_RB_FLAG_RED;
    entry->left->flags ^= WINE_RB_FLAG_RED;
    entry->right->flags ^= WINE_RB_FLAG_RED;
}

static inline void wine_rb_fixup(struct wine_rb_tree *tree, struct wine_rb_stack *stack)
{
    while (stack->count)
    {
        struct wine_rb_entry **entry = stack->entries[stack->count - 1];

        if ((*entry)->flags & WINE_RB_FLAG_STOP)
        {
            (*entry)->flags &= ~WINE_RB_FLAG_STOP;
            return;
        }

        if (wine_rb_is_red((*entry)->right) && !wine_rb_is_red((*entry)->left)) wine_rb_rotate_left(tree, *entry, 1);
        if (wine_rb_is_red((*entry)->left) && wine_rb_is_red((*entry)->left->left)) wine_rb_rotate_right(tree, *entry, 1);
        if (wine_rb_is_red((*entry)->left) && wine_rb_is_red((*entry)->right)) wine_rb_flip_color(*entry);
        --stack->count;
    }
}

static inline struct wine_rb_entry *wine_rb_postorder_head(struct wine_rb_entry *iter)
{
    if (!iter) return NULL;

    for (;;) {
        while (iter->left) iter = iter->left;
        if (!iter->right) return iter;
        iter = iter->right;
    }
}

static inline struct wine_rb_entry *wine_rb_postorder_next(struct wine_rb_entry *iter)
{
    if (!iter->parent) return NULL;
    if (iter == iter->parent->right || !iter->parent->right) return iter->parent;
    return wine_rb_postorder_head(iter->parent->right);
}

static inline void wine_rb_postorder(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    struct wine_rb_entry *iter, *next;

    for (iter = wine_rb_postorder_head(tree->root); iter; iter = next)
    {
        next = wine_rb_postorder_next(iter);
        callback(iter, context);
    }
}

static inline int wine_rb_init(struct wine_rb_tree *tree, const struct wine_rb_functions *functions)
{
    tree->functions = functions;
    tree->root = NULL;

    tree->stack.entries = functions->alloc(16 * sizeof(*tree->stack.entries));
    if (!tree->stack.entries) return -1;
    tree->stack.size = 16;
    tree->stack.count = 0;

    return 0;
}

static inline void wine_rb_for_each_entry(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    wine_rb_postorder(tree, callback, context);
}

static inline void wine_rb_clear(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    /* Note that we use postorder here because the callback will likely free the entry. */
    if (callback) wine_rb_postorder(tree, callback, context);
    tree->root = NULL;
}

static inline void wine_rb_destroy(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    wine_rb_clear(tree, callback, context);
    tree->functions->free(tree->stack.entries);
}

static inline struct wine_rb_entry *wine_rb_get(const struct wine_rb_tree *tree, const void *key)
{
    struct wine_rb_entry *entry = tree->root;
    while (entry)
    {
        int c = tree->functions->compare(key, entry);
        if (!c) return entry;
        entry = c < 0 ? entry->left : entry->right;
    }
    return NULL;
}

static inline int wine_rb_put(struct wine_rb_tree *tree, const void *key, struct wine_rb_entry *entry)
{
    struct wine_rb_entry **iter = &tree->root, *parent = tree->root;
    size_t black_height = 1;

    while (*iter)
    {
        int c;

        if (!wine_rb_is_red(*iter)) ++black_height;

        wine_rb_stack_push(&tree->stack, iter);

        parent = *iter;
        c = tree->functions->compare(key, parent);
        if (!c)
        {
            wine_rb_stack_clear(&tree->stack);
            return -1;
        }
        else if (c < 0) iter = &parent->left;
        else iter = &parent->right;
    }

    /* After insertion, the path length to any node should be <= (black_height + 1) * 2. */
    if (wine_rb_ensure_stack_size(tree, black_height << 1) == -1)
    {
        wine_rb_stack_clear(&tree->stack);
        return -1;
    }

    entry->flags = WINE_RB_FLAG_RED;
    entry->parent = parent;
    entry->left = NULL;
    entry->right = NULL;
    *iter = entry;

    wine_rb_fixup(tree, &tree->stack);
    tree->root->flags &= ~WINE_RB_FLAG_RED;

    return 0;
}

static inline void wine_rb_remove(struct wine_rb_tree *tree, const void *key)
{
    struct wine_rb_entry *entry, *iter, *child, *parent, *w;
    int need_fixup;

    if (!(entry = wine_rb_get(tree, key))) return;

    if (entry->right && entry->left)
        for(iter = entry->right; iter->left; iter = iter->left);
    else
        iter = entry;

    child = iter->left ? iter->left : iter->right;

    if (!iter->parent)
        tree->root = child;
    else if (iter == iter->parent->left)
        iter->parent->left = child;
    else
        iter->parent->right = child;

    if (child) child->parent = iter->parent;
    parent = iter->parent;

    need_fixup = !wine_rb_is_red(iter);

    if (entry != iter)
    {
        *iter = *entry;
        if (!iter->parent)
            tree->root = iter;
        else if (entry == iter->parent->left)
            iter->parent->left = iter;
        else
            iter->parent->right = iter;

        if (iter->right) iter->right->parent = iter;
        if (iter->left)  iter->left->parent = iter;
        if (parent == entry) parent = iter;
    }

    if (need_fixup)
    {
        while (parent && !wine_rb_is_red(child))
        {
            if (child == parent->left)
            {
                w = parent->right;
                if (wine_rb_is_red(w))
                {
                    w->flags &= ~WINE_RB_FLAG_RED;
                    parent->flags |= WINE_RB_FLAG_RED;
                    wine_rb_rotate_left(tree, parent, 0);
                    w = parent->right;
                }
                if (wine_rb_is_red(w->left) || wine_rb_is_red(w->right))
                {
                    if (!wine_rb_is_red(w->right))
                    {
                        w->left->flags &= ~WINE_RB_FLAG_RED;
                        w->flags |= WINE_RB_FLAG_RED;
                        wine_rb_rotate_right(tree, w, 0);
                        w = parent->right;
                    }
                    w->flags = (w->flags & ~WINE_RB_FLAG_RED) | (parent->flags & WINE_RB_FLAG_RED);
                    parent->flags &= ~WINE_RB_FLAG_RED;
                    if (w->right)
                        w->right->flags &= ~WINE_RB_FLAG_RED;
                    wine_rb_rotate_left(tree, parent, 0);
                    child = NULL;
                    break;
                }
            }
            else
            {
                w = parent->left;
                if (wine_rb_is_red(w))
                {
                    w->flags &= ~WINE_RB_FLAG_RED;
                    parent->flags |= WINE_RB_FLAG_RED;
                    wine_rb_rotate_right(tree, parent, 0);
                    w = parent->left;
                }
                if (wine_rb_is_red(w->left) || wine_rb_is_red(w->right))
                {
                    if (!wine_rb_is_red(w->left))
                    {
                        w->right->flags &= ~WINE_RB_FLAG_RED;
                        w->flags |= WINE_RB_FLAG_RED;
                        wine_rb_rotate_left(tree, w, 0);
                        w = parent->left;
                    }
                    w->flags = (w->flags & ~WINE_RB_FLAG_RED) | (parent->flags & WINE_RB_FLAG_RED);
                    parent->flags &= ~WINE_RB_FLAG_RED;
                    if (w->left)
                        w->left->flags &= ~WINE_RB_FLAG_RED;
                    wine_rb_rotate_right(tree, parent, 0);
                    child = NULL;
                    break;
                }
            }
            w->flags |= WINE_RB_FLAG_RED;
            child = parent;
            parent = child->parent;
        }
        if (child) child->flags &= ~WINE_RB_FLAG_RED;
    }

    if (tree->root) tree->root->flags &= ~WINE_RB_FLAG_RED;
}

#endif  /* __WINE_WINE_RBTREE_H */
