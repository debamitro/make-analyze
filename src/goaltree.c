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
  unsigned long dependents;
  unsigned long dependers;
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
filename_to_id (const char * filename, const int isDependent)
{
  struct filename_id key;
  key.filename = filename;
  key.id = 0;
  struct filename_id ** slot = (struct filename_id **) hash_find_slot (&goaltreeFiles, &key);
  if (!HASH_VACANT(*slot))
    {
      if (isDependent)
        {
          (*slot)->dependers++;
        }
      return (*slot)->id;
    }

  struct filename_id * newfId = (struct filename_id *) xcalloc (sizeof (struct filename_id));
  newfId->id = ++nFiles;
  newfId->filename = filename;
  newfId->dependers = 1;
  newfId->dependents = 0;
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

struct dep_tree_child_list
{
  struct file * child;
  int level;
  struct dep_tree_child_list * next;
};

static struct dep_tree_child_list * dep_tree_child_list_alloc (struct file * const child,
                                                               const int level)
{
  struct dep_tree_child_list * depTreeChild = (struct dep_tree_child_list *) xcalloc (sizeof (struct dep_tree_child_list));
  depTreeChild->child = child;
  depTreeChild->level = level;
  depTreeChild->next = NULL;

  return depTreeChild;
}

static struct dep_tree_child_list * dep_tree_child_list_add_after (struct dep_tree_child_list * const listItr,
                                                                   struct file * const child,
                                                                   const int level)
{
  struct dep_tree_child_list * newChild = dep_tree_child_list_alloc (child, level);
  if (listItr->next != NULL)
    {
      /* Add after listItr by inserting before listItr->next */
      struct dep_tree_child_list * tmp = listItr->next;
      listItr->next = newChild;
      newChild->next = tmp;
    }
  else
    {
      /* Add after listItr directly */
      listItr->next = newChild;
    }

  return newChild;
}

/** Prints the dependency tree as nested JavaScript objects and arrays */
static void print_file_deps (struct file * depender, const int level, FILE * f)
{
  struct dep_tree_child_list * depItr = dep_tree_child_list_alloc (depender, level);

  while (depItr != NULL)
    {
      struct dep * childItr = depItr->child->deps;

      struct dep_tree_child_list * appendAfter = depItr;
      for (; childItr != NULL; childItr = childItr->next)
        {
          appendAfter = dep_tree_child_list_add_after (appendAfter, childItr->file, depItr->level+1);
        }

      if (depItr->next != NULL && depItr->next->level > depItr->level)
        {
          /* This node has children, hence it has to be dumped as an object
             with a property whose value is an array of its children */
          print_object_start (depItr->level, f);

          print_n_spaces (depItr->level+1, f);
          fprintf (f, "\"f%ld\":", filename_to_id (depItr->child->name, 1));

          print_array_start (0, f);
        }
      else
        {
          /* This is a leaf-level node */
          print_n_spaces (depItr->level+1, f);

          fprintf (f, "\"f%ld\"", filename_to_id (depItr->child->name, 1));
        }

      if (depItr->next == NULL || depItr->next->level < depItr->level)
        {
          /* This is the end of one or multiple lists */
          int numberOfEnded = 0;
          if (depItr->next == NULL)
            {
              numberOfEnded = depItr->level - level;
            }
          else
            {
              numberOfEnded = depItr->level - depItr->next->level;
            }

          if (prettyPrint)
            {
              fprintf (f, "\n");
            }

          int i;
          for (i = numberOfEnded; i > 0; --i)
            {
              print_array_end (i+1, f);

              print_object_end (i, f);
            }

        }

      if (depItr->next != NULL && depItr->next->level <= depItr->level)
        {
          fprintf (f, ",");

          if (prettyPrint)
            {
              fprintf (f, "\n");
            }
        }


      struct dep_tree_child_list * discarded = depItr;
      depItr = depItr->next;
      free (discarded);
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
      print_file_deps (itr->file, 2, f);

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
  print_object_end (0, f);

  /* Work in progress - unfinished
  for (fileId = fileIds; fileId < fileIds_end; fileId++)
    {
      printf ("%ld %ld\n",
              (*fileId)->dependers,
              (*fileId)->id);
    }
  */

  free (fileIds);
  hash_free_items (&goaltreeFiles);
}
