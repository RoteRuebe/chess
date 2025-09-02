#ifndef MAIN_H
#define MAIN_H

#define VERSION_MAJ 1
#define VERSION_MIN 1

static int verbosity = 1;

/* Which things to log
 * 2 -> all
 * 1 -> only log and error
 * 0 -> only error
*/
void log_msg(char* msg, int type);

#endif