/*
 * Termios command line History and Editing.
 *
 * Copyright (c) 1986-2003 may safely be consumed by a BSD or GPL license.
 * Written by:   Vladimir Oleynik <dzo@simtreas.ru>
 *
 * Used ideas:
 *      Adam Rogoyski    <rogoyski@cs.utexas.edu>
 *      Dave Cinege      <dcinege@psychosis.com>
 *      Jakub Jelinek (c) 1995
 *      Erik Andersen    <andersen@codepoet.org> (Majorly adjusted for busybox)
 *
 * This code is 'as is' with no warranty.
 *
 *
 */

#ifndef CMDEDIT_H
#define CMDEDIT_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This is the internal header file for the original cmdedit.c.
 * See framework_cli.h for the public API's.
 */

#ifdef CONFIG_FEATURE_COMMAND_SAVEHISTORY
void    load_history ( const char *fromfile );
void    save_history ( const char *tofile );
#endif

#if defined(__cplusplus)
}
#endif

#endif /* CMDEDIT_H */
