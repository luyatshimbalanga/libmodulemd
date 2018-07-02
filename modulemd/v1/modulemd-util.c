/*
 * This file is part of libmodulemd
 * Copyright (C) 2017-2018 Stephen Gallagher
 *
 * Fedora-License-Identifier: MIT
 * SPDX-2.0-License-Identifier: MIT
 * SPDX-3.0-License-Identifier: MIT
 *
 * This program is free software.
 * For more information on the license, see COPYING.
 * For more information on free software, see <https://www.gnu.org/philosophy/free-sw.en.html>.
 */

#include "modulemd.h"
#include <string.h>
#include "private/modulemd-util.h"
#include <locale.h>

GHashTable *
_modulemd_hash_table_deep_str_copy (GHashTable *orig)
{
  GHashTable *new;
  GHashTableIter iter;
  gpointer key, value;

  g_return_val_if_fail (orig, NULL);

  new = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  g_hash_table_iter_init (&iter, orig);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      g_hash_table_insert (
        new, g_strdup ((const gchar *)key), g_strdup ((const gchar *)value));
    }

  return new;
}

GHashTable *
_modulemd_hash_table_deep_obj_copy (GHashTable *orig)
{
  GHashTable *new;
  GHashTableIter iter;
  gpointer key, value;

  g_return_val_if_fail (orig, NULL);

  new =
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  g_hash_table_iter_init (&iter, orig);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      g_hash_table_insert (
        new, g_strdup ((const gchar *)key), g_object_ref ((GObject *)value));
    }

  return new;
}

GHashTable *
_modulemd_hash_table_deep_variant_copy (GHashTable *orig)
{
  GHashTable *new;
  GHashTableIter iter;
  gpointer key, value;

  g_return_val_if_fail (orig, NULL);

  new = g_hash_table_new_full (
    g_str_hash, g_str_equal, g_free, modulemd_variant_unref);

  g_hash_table_iter_init (&iter, orig);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      g_hash_table_insert (
        new, g_strdup ((const gchar *)key), g_variant_ref ((GVariant *)value));
    }

  return new;
}

gint
_modulemd_strcmp_sort (gconstpointer a, gconstpointer b)
{
  return g_strcmp0 (*(const gchar **)a, *(const gchar **)b);
}

GPtrArray *
_modulemd_ordered_str_keys (GHashTable *htable, GCompareFunc compare_func)
{
  GPtrArray *keys;
  GHashTableIter iter;
  gpointer key, value;

  keys = g_ptr_array_new_full (g_hash_table_size (htable), g_free);

  g_hash_table_iter_init (&iter, htable);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      g_ptr_array_add (keys, g_strdup ((const gchar *)key));
    }
  g_ptr_array_sort (keys, compare_func);

  return keys;
}

static gint
_modulemd_int64_sort (gconstpointer a, gconstpointer b)
{
  gint retval = *(const gint64 *)a - *(const gint64 *)b;
  return retval;
}

GList *
_modulemd_ordered_int64_keys (GHashTable *htable)
{
  g_autoptr (GList) hash_keys = NULL;
  GList *unsorted_keys = NULL;
  GList *sorted_keys = NULL;

  /* Get the keys from the hash table */
  hash_keys = g_hash_table_get_keys (htable);

  /* Make a copy of the keys that we can modify */
  unsorted_keys = g_list_copy (hash_keys);

  /* Sort the keys numerically */
  sorted_keys = g_list_sort (unsorted_keys, _modulemd_int64_sort);

  return sorted_keys;
}

void
modulemd_variant_unref (void *ptr)
{
  g_variant_unref ((GVariant *)ptr);
}

gboolean
modulemd_validate_nevra (const gchar *nevra)
{
  g_autofree gchar *tmp = g_strdup (nevra);
  gsize len = strlen (nevra);
  gchar *i;
  gchar *endptr;

  /* Since the "name" portion of a NEVRA can have an infinite number of
   * hyphens, we need to parse from the end backwards.
   */
  i = &tmp[len - 1];

  /* Everything after the last '.' must be the architecture */
  while (i >= tmp)
    {
      if (*i == '.')
        {
          break;
        }
      i--;
    }

  if (i < tmp)
    {
      /* We hit the start of the string without hitting '.' */
      return FALSE;
    }

  /*
   * TODO: Compare the architecture suffix with a list of known-valid ones
   * This needs to come from an external source that's kept up to date or
   * this will regularly break.
   */

  /* Process the "release" tag */
  while (i >= tmp)
    {
      if (*i == '-')
        {
          break;
        }
      i--;
    }

  if (i < tmp)
    {
      /* We hit the start of the string without hitting '-' */
      return FALSE;
    }

  /* No need to validate Release; it's fairly arbitrary */

  /* Process the version */
  while (i >= tmp)
    {
      if (*i == ':')
        {
          break;
        }
      i--;
    }
  if (i < tmp)
    {
      /* We hit the start of the string without hitting ':' */
      return FALSE;
    }

  /* Process the epoch */
  *i = '\0';
  while (i >= tmp)
    {
      if (*i == '-')
        {
          break;
        }
      i--;
    }
  if (i < tmp)
    {
      /* We hit the start of the string without hitting '-' */
      return FALSE;
    }

  /* Validate that this section is a number */
  g_ascii_strtoll (i, &endptr, 10);
  if (endptr == i)
    {
      /* String conversion failed */
      return FALSE;
    }

  /* No need to specifically parse the name section here */

  return TRUE;
}


modulemd_tracer *
modulemd_trace_init (const gchar *function_name)
{
  modulemd_tracer *self = g_malloc0_n (1, sizeof (modulemd_tracer));
  self->function_name = g_strdup (function_name);

  g_debug ("TRACE: Entering %s", self->function_name);

  return self;
}


void
modulemd_trace_free (modulemd_tracer *tracer)
{
  g_debug ("TRACE: Exiting %s", tracer->function_name);
  g_clear_pointer (&tracer->function_name, g_free);
  g_free (tracer);
}


ModulemdTranslationEntry *
_get_locale_entry (ModulemdTranslation *translation, const gchar *_locale)
{
  ModulemdTranslationEntry *entry = NULL;
  g_autofree gchar *locale = NULL;

  if (!translation)
    return NULL;

  g_return_val_if_fail (MODULEMD_IS_TRANSLATION (translation), NULL);

  if (_locale)
    {
      if (g_strcmp0 (_locale, "C") == 0 || g_strcmp0 (_locale, "C.UTF-8") == 0)
        {
          /* If the locale is "C" or "C.UTF-8", always return the standard value */
          return NULL;
        }

      locale = g_strdup (_locale);
    }
  else
    {
      /* If the locale was NULL, use the locale of this process */
      locale = g_strdup (setlocale (LC_MESSAGES, NULL));
    }

  entry = modulemd_translation_get_entry_by_locale (translation, locale);

  return entry;
}
