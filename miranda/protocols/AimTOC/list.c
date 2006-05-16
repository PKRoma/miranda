/*
AOL Instant Messenger Plugin for Miranda IM

Copyright (c) 2003 Robert Rainwater

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

TList *tlist_append(TList * list, void *data)
{
    TList *n;
    TList *new_list = malloc(sizeof(TList));
    TList *attach_to = NULL;

    new_list->next = NULL;
    new_list->data = data;
    for (n = list; n != NULL; n = n->next) {
        attach_to = n;
    }
    if (attach_to == NULL) {
        new_list->prev = NULL;
        return new_list;
    }
    else {
        new_list->prev = attach_to;
        attach_to->next = new_list;
        return list;
    }
}

TList *tlist_remove_link(TList * list, const TList * link)
{
    if (!link)
        return list;

    if (link->next)
        link->next->prev = link->prev;
    if (link->prev)
        link->prev->next = link->next;
    if (link == list)
        list = link->next;
    return list;
}

TList *tlist_remove(TList * list, void *data)
{
    TList *n;

    for (n = list; n != NULL; n = n->next) {
        if (n->data == data) {
            TList *newlist = tlist_remove_link(list, n);
            free(n);
            return newlist;
        }
    }
    return list;
}

void tlist_free(TList * list)
{
    TList *n = list;

    while (n != NULL) {
        TList *next = n->next;
        free(n);
        n = next;
    }
}
