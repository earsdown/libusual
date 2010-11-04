/*
 * Binary Heap.
 *
 * Copyright (c) 2009  Marko Kreen, Skype Technologies OÜ
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <usual/heap.h>

struct Heap {
	void **data;

	unsigned allocated;
	unsigned used;

	heap_is_better_f is_better;
	heap_save_pos_f save_pos;

	CxMem *cx;
};

/*
 * Low-level operations.
 */

#define inline __attribute__((always_inline))

static unsigned _heap_get_parent(unsigned i)
{
	return (i - 1) / 2;
}

static unsigned _heap_get_child(unsigned i, unsigned child_nr)
{
	return 2*i + 1 + child_nr;
}

static bool _heap_is_better(struct Heap *h, unsigned i1, unsigned i2)
{
	return h->is_better(h->data[i1], h->data[i2]);
}

static void _heap_set(struct Heap *h, unsigned i, void *ptr)
{
	h->data[i] = ptr;
	if (h->save_pos)
		h->save_pos(ptr, i);
}

static void _heap_swap(struct Heap *h, unsigned i1, unsigned i2)
{
	void *tmp = h->data[i1];
	_heap_set(h, i1, h->data[i2]);
	_heap_set(h, i2, tmp);
}

static void _heap_bubble_up(struct Heap *h, unsigned i)
{
	unsigned p;
	while (i > 0) {
		p = _heap_get_parent(i);
		if (!_heap_is_better(h, i, p))
			break;
		_heap_swap(h, i, p);
		i = p;
	}
}

static void _heap_bubble_down(struct Heap *h, unsigned i)
{
	unsigned c = _heap_get_child(i, 0);
	while (c < h->used) {
		if (c + 1 < h->used) {
			if (_heap_is_better(h, c + 1, c))
				c = c + 1;
		}
		if (!_heap_is_better(h, c, i))
			break;
		_heap_swap(h, i, c);
		i = c;
		c = _heap_get_child(i, 0);
	}
}

static void rebalance(struct Heap *h, unsigned pos)
{
	if (pos == 0) {
		_heap_bubble_down(h, pos);
	} else if (pos == h->used - 1) {
		_heap_bubble_up(h, pos);
	} else if (_heap_is_better(h, pos, _heap_get_parent(pos))) {
		_heap_bubble_up(h, pos);
	} else {
		_heap_bubble_down(h, pos);
	}
}

/*
 * Actual API.
 */


struct Heap *heap_create(heap_is_better_f is_better_cb, heap_save_pos_f save_pos_cb, CxMem *cx)
{
	struct Heap *h;
	
	h = cx_alloc0(cx, sizeof(*h));
	if (!h)
		return NULL;

	h->save_pos = save_pos_cb;
	h->is_better = is_better_cb;
	h->cx = cx;

	return h;
}

void heap_destroy(struct Heap *h)
{
	cx_free(h->cx, h->data);
	cx_free(h->cx, h);
}

bool heap_reserve(struct Heap *h, unsigned extra)
{
	void *tmp;
	unsigned newalloc;

	if (h->used + extra < h->allocated)
		return true;

	newalloc = h->allocated * 2;
	if (newalloc < 32)
		newalloc = 32;
	if (newalloc < h->used + extra)
		newalloc = h->used + extra;

	tmp = cx_realloc(h->cx, h->data, newalloc * sizeof(void *));
	if (!tmp)
		return false;
	h->data = tmp;
	h->allocated = newalloc;
	return true;
}

void *heap_top(struct Heap *h)
{
	return (h->used > 0) ? h->data[0] : NULL;
}

bool heap_push(struct Heap *h, void *ptr)
{
	unsigned pos;

	if (h->used >= h->allocated) {
		if (!heap_reserve(h, 1))
			return false;
	}

	pos = h->used++;
	_heap_set(h, pos, ptr);
	_heap_bubble_up(h, pos);
	return true;
}

void *heap_remove(struct Heap *h, unsigned pos)
{
	unsigned last;
	void *obj;

	if (pos >= h->used)
		return NULL;

	obj = h->data[pos];

	last = --h->used;
	if (pos < last) {
		_heap_set(h, pos, h->data[last]);
		rebalance(h, pos);
	}
	h->data[last] = NULL;
	return obj;
}

void *heap_pop(struct Heap *h)
{
	return heap_remove(h, 0);
}

