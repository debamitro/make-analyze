/*
 * This file was NOT part of GNU make
 */

#include "ctags.h"
#include "makeint.h"

#include <stdio.h>

static FILE * TAGSfile = NULL;
typedef struct {
  const char * name;
  const char * filename;
  unsigned long lineno;
} TAGline;

static int TAGline_compare (const void * a, const void * b)
{
  const TAGline * aTAGline = (const TAGline *)a;
  const TAGline * bTAGline = (const TAGline *)b;

  return strcmp (aTAGline->name, bTAGline->name);
}

typedef struct {
  TAGline * array;
  unsigned int size;
  unsigned int capacity;
} TAGlinearray;

static TAGlinearray TAGlines = { NULL, 0, 0 };

void init_ctags_output (const char * filename)
{
  TAGSfile = fopen (filename,"w");
}

void add_to_ctags (const char * name, const char * filename, unsigned long lineno)
{
  if (TAGSfile == NULL)
    {
      return;
    }

  if (TAGlines.capacity >= TAGlines.size)
    {
      unsigned int biggerCapacity = TAGlines.capacity + 512;

      TAGline * biggerArray  = (TAGline *) xmalloc (biggerCapacity * sizeof (TAGline));

      if (TAGlines.array != NULL)
        {
          for (int i = 0; i < TAGlines.capacity; ++i)
            {
              biggerArray[i] = TAGlines.array[i];
            }

          free (TAGlines.array);
        }

      TAGlines.array = biggerArray;
      TAGlines.capacity = biggerCapacity;
    }

  TAGlines.array[ TAGlines.size ].name = xstrdup (name);
  TAGlines.array[ TAGlines.size ].filename = xstrdup (filename);
  TAGlines.array[ TAGlines.size ].lineno = lineno;

  ++TAGlines.size;
}

void write_out_ctags ()
{
  if (TAGSfile == NULL)
    {
      return;
    }

  qsort (TAGlines.array, TAGlines.size, sizeof (TAGline), TAGline_compare);

  for (int i = 0; i < TAGlines.size; ++i)
    {
      fprintf (TAGSfile, "%s\t%s\t%ld\n",
               TAGlines.array[i].name,
               TAGlines.array[i].filename,
               TAGlines.array[i].lineno);
    }

  fclose (TAGSfile);
}
