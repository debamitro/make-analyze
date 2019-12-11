/*
 * This file was NOT part of GNU make
 */


void init_ctags_output (const char * filename);
void add_to_ctags (const char * name, const char * filename, unsigned long lineno);
void write_out_ctags ();
