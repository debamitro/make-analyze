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
#include <sys/stat.h>
#include <sys/types.h>

static struct hash_table goaltreeFiles;

struct filename_id
{
  const char * filename;
  unsigned long id;
  unsigned long dependents;
  unsigned long dependers;
  unsigned int is_considered;
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

static struct filename_id ** filename_to_hash_slot (const char * filename)
{
  struct filename_id key;
  key.filename = filename;
  key.id = 0;
  return  (struct filename_id **) hash_find_slot (&goaltreeFiles, &key);
}

static struct filename_id * filename_to_filename_id (const char * filename)
{
  struct filename_id ** slot = filename_to_hash_slot (filename);
  if (!HASH_VACANT(*slot))
    {
      return (*slot);
    }

  return NULL;
}

static unsigned long
filename_to_id (const char * filename, const int isDependent)
{
  struct filename_id ** slot = filename_to_hash_slot (filename);
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
  newfId->is_considered = 0;
  hash_insert_at (&goaltreeFiles, newfId, slot);

  return newfId->id;
}

static void start_considering_file (const char * filename)
{
  struct filename_id * filenameId = filename_to_filename_id (filename);

  if (filenameId != NULL)
    {
      filenameId->is_considered = 1;
    }
}

static void stop_considering_file (const char * filename)
{
  struct filename_id * filenameId = filename_to_filename_id (filename);

  if (filenameId != NULL)
    {
      filenameId->is_considered = 0;
    }
}

static int considering_file (const char * filename)
{
  struct filename_id * filenameId = filename_to_filename_id (filename);

  if (filenameId != NULL)
    {
      return filenameId->is_considered;
    }

  return 0;
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
          if (considering_file (childItr->file->name))
            {
              /* possible cyclic dependency */
              printf ("cyclic dependency found on %s\n",childItr->file->name);
              continue;
            }

          appendAfter = dep_tree_child_list_add_after (appendAfter, childItr->file, depItr->level+1);
        }

      const unsigned long depfileId = filename_to_id (depItr->child->name, 1);
      start_considering_file (depItr->child->name);
      if (depItr->next != NULL && depItr->next->level > depItr->level)
        {
          /* This node has children, hence it has to be dumped as an object
             with a property whose value is an array of its children */
          print_object_start (depItr->level, f);

          print_n_spaces (depItr->level+1, f);
          fprintf (f, "\"f%ld\":", depfileId);

          print_array_start (0, f);
        }
      else
        {
          /* This is a leaf-level node */
          print_n_spaces (depItr->level+1, f);

          fprintf (f, "\"f%ld\"", depfileId);
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


      stop_considering_file (depItr->child->name);
      struct dep_tree_child_list * discarded = depItr;
      depItr = depItr->next;
      free (discarded);
    }
}

/** Prints the dependency trees of all the goals
 * as nested JavaScript objects and arrays */
void print_goal_tree (struct goaldep * const goals, FILE * f)
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

struct previous_file_list
{
  struct file * file;
  struct previous_file_list * previous;
};

static void push_current_file (struct previous_file_list ** previousFiles,
                               struct file * currentFile)
{
  struct previous_file_list * previousFile = (struct previous_file_list *) xcalloc (sizeof (struct previous_file_list));
  previousFile->file = currentFile;
  previousFile->previous = *previousFiles;
  *previousFiles = previousFile;
}

static struct file * pop_previous_file (struct previous_file_list ** previousFiles)
{
  if (*previousFiles != NULL)
    {
      struct file * previousFile = (*previousFiles)->file;
      struct previous_file_list * previous = (*previousFiles)->previous;
      free (*previousFiles);
      *previousFiles = previous;
      return previousFile;
    }

  return NULL;
}

/** Interactive browser for goals
 * and their dependencies */
void browse_goal_tree (struct goaldep * const goals)
{
  printf ("Goal tree browser\n");
  printf ("=================\n");

  int keeprunning = 1;
  while (keeprunning)
    {
      struct goaldep * itr = goals;

      printf ("Goals:\n");
      int goalNumber = 0;
      for (; itr != NULL; itr = itr->next)
        {
          printf ("%d: %s\n",
                  ++goalNumber,
                  itr->file->name);
        }

      printf ("Which one do you want details on?\n");
      printf ("Select from the above choices or 0 to exit ");
      int selectedGoal;
      scanf ("%d",&selectedGoal);

      if (selectedGoal == 0)
        {
          /* Exit */
          keeprunning = 0;
          break;
        }

      if (selectedGoal > goalNumber)
        {
          printf ("Bad input, let's try again\n");
          continue;
        }

      itr = goals;
      for (goalNumber = 0; goalNumber < selectedGoal-1; ++goalNumber)
        {
          itr = itr->next;
        }

      struct file * currentFile = itr->file;
      struct previous_file_list * previousFiles = NULL;
      while (currentFile != NULL)
        {
          printf ("Dependencies of %s:\n",currentFile->name);
          struct dep * depItr;
          goalNumber = 0;
          for (depItr = currentFile->deps; depItr != NULL; depItr = depItr->next)
            {
              printf ("%d: %s%s\n",
                      ++goalNumber,
                      depItr->file->name,
                      depItr->ignore_mtime ? " [order-only]" : "");
            }

          printf ("Which one do you want details on?\n");
          printf ("Select from the above choices, %d to go back, or 0 to exit ", goalNumber+1);
          int selectedFile;
          scanf ("%d",&selectedFile);

          if (selectedFile == goalNumber+1)
            {
              /* Go back to the previous one */
              currentFile = pop_previous_file (&previousFiles);
              continue;
            }

          if (selectedFile == 0)
            {
              /* Exit */
              keeprunning = 0;
              break;
            }

          if (selectedFile > goalNumber+1 ||
              selectedFile < 0)
            {
              printf ("Bad input, let's try again\n");
              continue;
            }

          /* Let's move to dependency number 'selectedFile' */
          depItr = currentFile->deps;
          for (goalNumber = 0; goalNumber < selectedFile-1; ++goalNumber)
            {
              depItr = depItr->next;
            }

          push_current_file (&previousFiles, currentFile);
          currentFile = depItr->file;
        }
    }
  printf ("Bye!");
}

struct files_bfs_list
{
  struct file * current;
  struct files_bfs_list * next;
};

static void print_goal_tree_children (FILE * f, struct goaldep * const goals)
{
  struct files_bfs_list * bfs_list = NULL;
  struct files_bfs_list * bfs_list_tail = NULL;

  struct goaldep * itr = goals;
  for (;itr != NULL; itr = itr->next)
    {
      struct files_bfs_list * bfs_elem = (struct files_bfs_list *) xcalloc (sizeof (struct files_bfs_list));
      bfs_elem->current = itr->file;
      bfs_elem->next = NULL;

      if (bfs_list == NULL)
        {
          bfs_list = bfs_elem;
          bfs_list_tail = bfs_list;
        }
      else
        {
          bfs_list_tail->next = bfs_elem;
          bfs_list_tail = bfs_elem;
        }
    }

  struct files_bfs_list * itr2 = bfs_list;
  while (itr2 != NULL)
    {
      fprintf (f, "\"%s\":", itr2->current->name);
      if (itr2->current->deps == NULL)
        {
          fprintf (f, "[]");
        }
      else
        {
          fprintf (f, "[");
          struct dep * depItr = itr2->current->deps;
          for (;depItr != NULL; depItr = depItr->next)
            {
              fprintf (f, "\"%s\"",depItr->file->name);
              if (depItr->next != NULL)
                fprintf (f, ",");


              struct files_bfs_list * bfs_elem = (struct files_bfs_list *) xcalloc (sizeof (struct files_bfs_list));
              bfs_elem->current = depItr->file;
              bfs_elem->next = NULL;
              bfs_list_tail->next = bfs_elem;
              bfs_list_tail = bfs_elem;
            }
          fprintf (f, "]");
        }

      if (itr2->next != NULL)
        fprintf (f, ",\n");

      struct files_bfs_list * tmp = itr2->next;
      free (itr2);
      itr2 = tmp;
    }
}

static void print_goal_tree_file_ids (FILE * f)
{
}

static void generate_goaltree_js (struct goaldep * goals,
                                  const char * output_dir);
static void generate_goaltree_html (const char * output_dir);
static void generate_minimalist_tree_js (const char * output_dir);

/**
 * Generate an HTML version of the goal dependency
 * tree viewer, in a directory called output_dir.<process-id>
 */
void print_goal_tree_as_html (struct goaldep * const goals,
                              const char * output_dir)
{
  char * unique_output_dir = (char *) xcalloc (strlen(output_dir) + 32);
  sprintf (unique_output_dir,"%s.%d",output_dir,getpid());
  if (mkdir (unique_output_dir, 511) != 0)
    {
      printf ("Error: couldn't create directory %s for goaltree HTML output\n",unique_output_dir);
      free (unique_output_dir);
      return;
    }

  generate_goaltree_js (goals, unique_output_dir);
  generate_goaltree_html (unique_output_dir);
  generate_minimalist_tree_js (unique_output_dir);

  printf ("Generated HTML-based goal dependency tree viewer at %s/goaltree.html\n",
          unique_output_dir);
  free (unique_output_dir);
}

static void generate_goaltree_js (struct goaldep * const goals,
                                  const char * output_dir)
{
  char * path_to_js_file = (char *) xcalloc (strlen (output_dir) +
                                             strlen("/goaltree.js") + 1);
  sprintf (path_to_js_file, "%s/goaltree.js",output_dir);
  FILE * js_file = fopen (path_to_js_file, "w");

  if (js_file == NULL)
    {
      printf ("Error: couldn't create %s\n",
              path_to_js_file);
      free (path_to_js_file);
      return;
    }

  fprintf (js_file,
           "const data = {\n"
           "  targets : [");
  struct goaldep * itr = goals;
  for (;itr != NULL; itr = itr->next)
    {
      fprintf (js_file, "\"%s\"",itr->file->name);
      if (itr->next != NULL)
        fprintf (js_file, ",");
    }
  fprintf (js_file,
           "],\n"
           "  children : {\n");

  print_goal_tree_children (js_file, goals);

  fprintf (js_file,
           "  },\n"
           "  ids : {\n");

  print_goal_tree_file_ids (js_file);

  fprintf (js_file,
           "  }\n"
           "};\n");

  fprintf (js_file,
           "function setupGoalTree (container)\n"
           "{\n"
           "    let goalTree = MinimalistTree (document.getElementById (container));\n"
           "    goalTree.draw ({\n"
           "        getroots : () => data.targets,\n"
           "        getchildren : (parent) => data.children[parent],\n"
           "//        getlabel : (node) => data.ids[node]\n"
           "        getlabel : (node) => node\n"
           "    });\n"
           "}\n");
  fclose (js_file);

  free (path_to_js_file);
}

/** Generates an HTML file with the contents
 * of html-viewer/goaltree.html
 * inside the directory output_dir
 */
static void generate_goaltree_html (const char * output_dir)
{
  char * path_to_html_file = (char *) xcalloc (strlen (output_dir) +
                                               strlen("/goaltree.html") + 1);
  sprintf (path_to_html_file, "%s/goaltree.html",output_dir);
  FILE * html_file = fopen (path_to_html_file, "w");

  if (html_file == NULL)
    {
      printf ("Error: couldn't create %s\n",
              path_to_html_file);
      free (path_to_html_file);
      return;
    }

  fprintf (html_file,
           "<!doctype html>\n"
           "<html>\n"
           "<head><title>Goal dependency tree viewer</title>\n"
           "<script src=\"minimalist-tree.js\"></script>\n"
           "<script src=\"goaltree.js\"></script>\n"
           "<script>\n"
           "  document.addEventListener('DOMContentLoaded', () => setupGoalTree ('theTree'));\n"
           "</script>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Goal dependency tree</h1>\n"
           "<div id =\"theTree\"></div>\n"
           "</body>"
           "</html>");
  fclose (html_file);
  free (path_to_html_file);
}

/** Generates a JavaScript file with the contents
 * of html-viewer/mininalist-tree.js
 * inside the directory output_dir
 */
static void generate_minimalist_tree_js (const char * output_dir)
{
  char * path_to_js_file = (char *) xcalloc (strlen (output_dir) +
                                             strlen("/minimalist-tree.js") + 1);
  sprintf (path_to_js_file, "%s/minimalist-tree.js",output_dir);
  FILE * js_file = fopen (path_to_js_file, "w");

  if (js_file == NULL)
    {
      printf ("Error: couldn't create %s\n",
              path_to_js_file);
      free (path_to_js_file);
      return;
    }

  fprintf (js_file,
           "function MinimalistTree (container)\n"
           "{\n"
           "  return {\n"
           "    draw : (options) => expandOrCollapseNodes (container, options)\n"
           "  }\n"
           "}\n"
           "\n"
           "function addNode (parent, node, options)\n"
           "{\n"
           "  var li = document.createElement ('li');\n"
           "  li.appendChild (document.createTextNode (options.getlabel(node)));\n"
           "  const children = options.getchildren (node);\n"
           "  if (children != null && children.length > 0)\n"
           "  {\n"
           "    li.addEventListener ('click', (e) => {\n"
           "      expandOrCollapseNodes (li, {\n"
           "        getroots : () => children,\n"
           "        getchildren : options.getchildren,\n"
           "        getlabel : options.getlabel\n"
           "      });\n"
           "      e.stopPropagation ();\n"
           "    });\n"
           "  }\n"
           "  else\n"
           "  {\n"
           "    li.addEventListener ('click', (e) => e.stopPropagation ());\n"
           "  }\n"
           "  parent.appendChild (li);\n"
           "}\n"
           "\n"
           "function expandOrCollapseNodes (parent, options)\n"
           "{\n"
           "  var existingUls = parent.getElementsByTagName ('ul');\n"
           "  if (existingUls.length > 0)\n"
           "  {\n"
           "    // Toggle the state\n"
           "    var styles = window.getComputedStyle (existingUls.item(0));\n"
           "    if (styles.getPropertyValue ('display') == 'none')\n"
           "    {\n"
           "      existingUls.item(0).setAttribute ('style', 'display:block');\n"
           "    }\n"
           "    else\n"
           "    {\n"
           "      existingUls.item(0).setAttribute ('style', 'display:none');\n"
           "     }\n"
           "     return;\n"
           "  }\n"
           "  // Create a new list for the nodes\n"
           "  var ul = document.createElement ('ul');\n"
           "  options.getroots().forEach ( (node) => {\n"
           "    addNode (ul, node, options);\n"
           "  });\n"
           "  parent.appendChild (ul);\n"
           "}\n");

  fclose (js_file);
  free (path_to_js_file);
}
