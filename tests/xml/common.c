/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2011 Jon Nordby <jononor@gmail.com>
 */

/* Common code used when testing XML serialization */

/* Fails assert if actual != expected and prints the two strings
 *
 * Note: XML strings tested as literal strings.
 * Vunerable to changes in whitespace and other insigificant things.
 *
 * It would be better to use a proper xml parser and/or xpath,
 * but we don't want to have more dependencies than
 * GEGL itself, to ensure that the tests are always available. */
static void
assert_equivalent_xml(const gchar *actual, const gchar *expected)
{
	gboolean equal;

	g_assert(actual);
	g_assert(expected);

    equal = (g_strcmp0(actual, expected) == 0);
    if (!equal) {
       g_print("\n\"%s\"\n!= (not equal)\n\"%s\"\n", actual, expected);
    }
    g_assert(equal);
}
