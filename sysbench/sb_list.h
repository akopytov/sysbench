/* Copyright (C) 2004 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SB_LIST_H

#define SB_LIST_H

typedef struct sb_list_item_t
{
    struct sb_list_item_t *next_p;
    struct sb_list_item_t *prev_p;
}
sb_list_item_t;

typedef sb_list_item_t sb_list_item;
typedef sb_list_item_t sb_list_t ;

#ifndef offsetof
# define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

#define SB_LIST_DECLARE(name) \
    SB_LIST_T name = { &(name), &(name) }

#define SB_LIST_INIT(head_p) \
    do { \
        (head_p)->next_p = (head_p); \
        (head_p)->prev_p = (head_p); \
    } while (0)

#define SB_LIST_ITEM_INIT(item_p) \
    SB_LIST_INIT(item_p)

#define SB_LIST_ADD(item_p, head_p) \
    do { \
        (item_p)->next_p = (head_p)->next_p; \
        (item_p)->prev_p = (head_p); \
        (head_p)->next_p = (item_p); \
        (item_p)->next_p->prev_p = (item_p); \
    } while (0)

#define SB_LIST_ADD_TAIL(item_p, head_p) \
    do { \
        (item_p)->prev_p = (head_p)->prev_p; \
        (item_p)->next_p = (head_p); \
        (head_p)->prev_p = (item_p); \
        (item_p)->prev_p->next_p = (item_p); \
    } while (0);

#define SB_LIST_DELETE(old_item_p) \
    do { \
        (old_item_p)->next_p->prev_p = (old_item_p)->prev_p; \
        (old_item_p)->prev_p->next_p = (old_item_p)->next_p; \
    } while (0)

#define SB_LIST_REPLACE(item_p, old_item_p) \
    do { \
        (item_p)->next_p = (old_item_p)->next_p; \
        (item_p)->prev_p = (old_item_p)->prev_p; \
        (item_p)->next_p->prev_p = (item_p); \
        (item_p)->prev_p->next_p = (item_p); \
    } while (0)

#define SB_LIST_IS_EMPTY(head_p) \
    ((head_p)->next_p == (head_p))

#define SB_LIST_ITEM_IN_LIST(item_p) \
    ((item_p)->next_p != (item_p))

#define SB_LIST_ITEM_FIRST(item_p, head_p) \
  ((item_p)->prev_p != (head_p))

#define SB_LIST_ITEM_LAST(item_p, head_p) \
  ((item_p)->next_p != (head_p))

#define SB_LIST_ITEM_NEXT(item_p) \
  ((item_p)->next_p)

#define SB_LIST_ITEM_PREV(item_p)               \
  ((item_p)->prev_p)
    
#define SB_LIST_ENTRY(ptr, type, member)            \
((type *)((char *)(ptr) - offsetof(type, member)))

#define SB_LIST_FOR_EACH(pos_p, head_p) \
    for (pos_p = (head_p)->next_p; pos_p != (head_p); pos_p = pos_p->next_p)

#define SB_LIST_FOR_EACH_SAFE(pos_p, temp_p, head_p) \
    for (pos_p = (head_p)->next_p, temp_p = (pos_p)->next_p; pos_p != (head_p); \
        pos_p = temp_p, temp_p = (pos_p)->next_p)

#define SB_LIST_FOR_EACH_REV_SAFE(pos_p, temp_p, head_p) \
    for (pos_p = (head_p)->prev_p, temp_p = (pos_p)->prev_p; pos_p != (head_p); \
        pos_p = temp_p, temp_p = (pos_p)->prev_p)

#endif /* SB_LIST_H */
