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
          int i;
          for (i = 0; i < TAGlines.capacity; ++i)
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

  int i;
  for (i = 0; i < TAGlines.size; ++i)
    {
      fprintf (TAGSfile, "%s\t%s\t%ld\n",
               TAGlines.array[i].name,
               TAGlines.array[i].filename,
               TAGlines.array[i].lineno);
    }

  fclose (TAGSfile);
}
