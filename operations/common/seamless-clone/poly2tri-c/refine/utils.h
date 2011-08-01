/*
 * This file is a part of Poly2Tri-C - The C port of the Poly2Tri library
 * Porting to C done by (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __POLY2TRI_C_REFINE_UTILS_H__
#define	__POLY2TRI_C_REFINE_UTILS_H__

#ifdef	__cplusplus
extern "C"
{
#endif

#include <glib.h>

  /* The code for the Hash Set is partially based on the example given at
   * http://developer.gnome.org/glib/2.29/glib-Hash-Tables.html
   */

  typedef GHashTable     P2tRHashSet;
  typedef GHashTableIter P2trHashSetIter;

#define p2tr_hash_set_set_new(hash_func, equal_func, destroy) g_hash_table_new_full ((hash_func), (equal_func), (destroy),NULL)
#define p2tr_hash_set_insert(set,element) g_hash_table_insert ((set), (element), (element))
#define p2tr_hash_set_contains(set,element) g_hash_table_lookup_extended ((set), (element), NULL, NULL)
#define p2tr_hash_set_remove(set,element) g_hash_table_remove ((set), (element))

#define p2tr_hash_set_iter_init(iter,hash_set) g_hash_table_iter_init ((iter),(hash_set))
#define p2tr_hash_set_iter_next(iter,val) g_hash_table_iter_next ((iter),(val),NULL)
#define p2tr_hash_set_iter_remove(iter) g_hash_table_iter_remove ((iter))

#define g_list_cyclic_prev(list,elem) (((elem)->prev != NULL) ? (elem)->prev : g_list_last ((elem)))
#define g_list_cyclic_next(list,elem) (((elem)->next != NULL) ? (elem)->next : g_list_first ((elem)))

#define foreach(iter,list) for ((iter) = (list); (iter) != NULL; (iter) = (iter)->next)

#ifdef	__cplusplus
}
#endif

#endif	/* UTILS_H */
