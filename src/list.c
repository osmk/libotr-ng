#include <stdlib.h>

#include "list.h"

list_element_t *list_new()
{
	list_element_t *n = malloc(sizeof(list_element_t));
	if (!n)
		return n;

	n->data = NULL;
	n->next = NULL;

	return n;
}

void list_free_full(list_element_t * head)
{
	list_element_t *current = head;
	while (current) {
		list_element_t *next = current->next;
		free(current->data);
		free(current);
		current = next;
	}
}

void list_free_all(list_element_t * head)
{
	list_element_t *current = head;
	while (current) {
		list_element_t *next = current->next;
		free(current);
		current = next;
	}
}

list_element_t *list_add(void *data, list_element_t * head)
{
	list_element_t *n = list_new();
	if (!n)
		return NULL;

	n->data = data;

	list_element_t *last = list_get_last(head);
	if (!last)
		return n;

	last->next = n;
	return head;
}

list_element_t *list_get_last(list_element_t * head)
{
	if (!head)
		return NULL;

	list_element_t *cursor = head;
	while (cursor->next)
		cursor = cursor->next;

	return cursor;
}

list_element_t *list_get_by_value(const void *wanted, list_element_t * head)
{
	list_element_t *cursor = head;

	while (cursor) {
		if (cursor->data == wanted)
			return cursor;

		cursor = cursor->next;
	}

	return NULL;
}

list_element_t *list_remove_element(const list_element_t * wanted,
				    list_element_t * head)
{
	if (head == wanted) {
		return wanted->next;
	}

	list_element_t *cursor = head;
	while (cursor->next) {
		if (cursor->next == wanted) {
			cursor->next = wanted->next;
			break;
		}

		cursor = cursor->next;
	}

	return head;
}

int list_len(list_element_t *head)
{

        list_element_t *cursor = head;
        int size = 0;

        while (cursor != NULL) {
                ++size;
                cursor = cursor->next;
        }

        return size;
}
