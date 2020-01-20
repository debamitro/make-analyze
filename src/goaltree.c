/*
 * Copyright 2019 Debamitro Chakraborti
 * This file was NOT part of GNU make
 *
 * Make-analyze is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "makeint.h"
#include "filedef.h"
#include "dep.h"
#include "goaltree.h"
#include "hash.h"

#include <stdio.h>

static struct hash_table goaltreeFiles;

struct filename_id
{
  const char * filename;
  unsigned long id;
};

static unsigned long nFiles = 0;

static unsigned long
file_hash_1 (const void *key)
{
  return_STRING_HASH_1 (((const struct filename_id *)key)->filename);
}

static unsigned long
file_hash_2 (const void *key)
{
  return_STRING_HASH_2 (((const struct filename_id *)key)->filename);
}

static int
file_hash_cmp (const void *x, const void *y)
{
  return_STRING_COMPARE (((const struct filename_id *)x)->filename,
                         ((const struct filename_id *)y)->filename);
}

static unsigned long
filename_to_id (const char * filename)
{
  struct filename_id key;
  key.filename = filename;
  key.id = 0;
  struct filename_id ** slot = (struct filename_id **) hash_find_slot (&goaltreeFiles, &key);
  if (!HASH_VACANT(*slot))
    {
      return (*slot)->id;
    }

  struct filename_id * newfId = (struct filename_id *) xcalloc (sizeof (struct filename_id));
  newfId->id = ++nFiles;
  newfId->filename = filename;
  hash_insert_at (&goaltreeFiles, newfId, slot);

  return newfId->id;
}

static int prettyPrint = 0;

static void print_n_spaces (const int n, FILE * f)
{
  if (!prettyPrint) return;

  int i;
  for (i = 0; i < n; ++i)
    fprintf (f, " ");
}

/** Prints the opening brace of a JavaScript object */
static void print_object_start (const int nSpaces, FILE * f)
{
  print_n_spaces (nSpaces, f);

  fprintf (f, "{");
  if (prettyPrint)
    {
      fprintf (f, "\n");
    }
}

/** Prints the closing brace of a JavaScript object */
static void print_object_end (const int nSpaces, FILE * f)
{
  print_n_spaces (nSpaces, f);

  fprintf (f, "}");
  if (prettyPrint)
    {
      fprintf (f, "\n");
    }
}

/** Prints the opening bracket of a JavaScript array */
static void print_array_start (const int nSpaces, FILE * f)
{
  print_n_spaces (nSpaces, f);

  fprintf (f, "[");
  if (prettyPrint)
    {
      fprintf (f, "\n");
    }
}

/** Prints the closing bracket of a JavaScript array */
static void print_array_end (const int nSpaces, FILE * f)
{
  print_n_spaces (nSpaces, f);

  fprintf (f, "]");
  if (prettyPrint)
    {
      fprintf (f, "\n");
    }
}

/** Prints the dependency tree as nested JavaScript objects and arrays */
static void print_deps_recursive (struct dep * firstDep, int level, FILE * f)
{
  struct dep * depItr = firstDep;
  for (; depItr != NULL; depItr = depItr->next)
    {
      if (depItr->file->deps != NULL)
        {
          print_object_start (level, f);

          print_n_spaces (level+1, f);
          fprintf (f, "\"f%ld\":", filename_to_id (depItr->file->name));

          print_array_start (0, f);

          print_deps_recursive (depItr->file->deps, level+2, f);

          print_array_end (level+1, f);

          print_object_end (level, f);
        }
      else
        {
          print_n_spaces (level, f);

          fprintf (f, "\"f%ld\"", filename_to_id (depItr->file->name));
        }

      if (depItr->next != NULL)
        fprintf (f, ",");

      if (prettyPrint)
        {
          fprintf (f, "\n");
        }
    }
}

void print_goal_tree (struct goaldep * goals, FILE * f)
{
  struct goaldep * itr = goals;
  hash_init (&goaltreeFiles, 1000, file_hash_1, file_hash_2, file_hash_cmp);

  print_object_start (0, f);

  print_n_spaces (1, f);
  fprintf (f,"\"targets\":");

  print_array_start (0,f );

  for (; itr != NULL; itr = itr->next)
    {
      print_object_start (2, f);

      print_n_spaces (3, f);
      fprintf (f, "\"f%ld\":", filename_to_id(itr->file->name));
      print_array_start (0, f);

      print_deps_recursive (itr->file->deps, 4, f);

      print_array_end (3, f);

      print_object_end (2, f);

      if (itr->next != NULL)
        fprintf (f, ",");

      if (prettyPrint)
        {
          fprintf (f, "\n");
        }
    }
  print_array_end (1, f);

  fprintf (f, ",\n");

  struct filename_id ** fileIds = (struct filename_id **)hash_dump (&goaltreeFiles, NULL, NULL);
  struct filename_id ** fileIds_end = fileIds + goaltreeFiles.ht_fill;
  struct filename_id ** fileId;
  struct filename_id ** fileIds_end_minus1 = fileIds + goaltreeFiles.ht_fill - 1;
  fprintf (f, "\"ids\":");
  print_object_start (1,f);
  for (fileId = fileIds; fileId < fileIds_end; fileId++)
    {
      print_n_spaces (1, f);
      fprintf (f, "\"f%ld\":\"%s\"",
               (*fileId)->id,
               (*fileId)->filename);
      if (fileId != fileIds_end_minus1)
        {
          fprintf (f,",");
        }

      if (prettyPrint)
        {
          fprintf (f,"\n");
        }
    }
  print_object_end (1, f);
  free (fileIds);
  print_object_end (0, f);
  hash_free_items (&goaltreeFiles);
}
