/* utils.h */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Report an error and exit. */

#define fatal() \
  do { fprintf (stderr, "%s: %s: %d: %s: %s\n", \
    argv[0], __FILE__, __LINE__,\
		__PRETTY_FUNCTION__, strerror (errno)); \
    exit (EXIT_FAILURE);} while (0)

#endif/* UTILS_H */

