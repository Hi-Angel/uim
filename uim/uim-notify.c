/*
  Copyright (c) 2003-2007 uim Project http://code.google.com/p/uim/

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of authors nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

#include "uim.h"
#include "uim-scm.h"
#include "uim-scm-abbrev.h"
#include "uim-internal.h"

#define UIM_NOTIFY_INTERNAL
#include "uim-notify.h"

#ifndef HAVE_DLFUNC
#define dlfunc dlsym
#endif

static int uim_notify_stderr_init(void);
static void uim_notify_stderr_quit(void);
static int uim_notify_stderr_info(const char *, va_list);
static int uim_notify_stderr_fatal(const char *, va_list);

static int (*uim_notify_init_func)(void) = uim_notify_stderr_init;
static void (*uim_notify_quit_func)(void) = uim_notify_stderr_quit;
static int (*uim_notify_info_func)(const char *, va_list) = uim_notify_stderr_info;
static int (*uim_notify_fatal_func)(const char *, va_list) = uim_notify_stderr_fatal;

static char notify_agent_name[PATH_MAX];

int
uim_notify_load(const char *name)
{
  if (uim_notify_quit_func == NULL) {
    fprintf(stderr, "uim-notify: notify module is not loaded\n");
    return 0;
  }

  uim_notify_quit_func();

  if (strcmp(name, "stderr") == 0) {
    uim_notify_init_func  = uim_notify_stderr_init;
    uim_notify_quit_func  = uim_notify_stderr_quit;
    uim_notify_info_func  = uim_notify_stderr_info;
    uim_notify_fatal_func = uim_notify_stderr_fatal;
  } else if (strcmp(notify_agent_name, name) == 0) {
    return 1;
  } else {
    void *handle;
    char path[PATH_MAX];
    const char *str;

    snprintf(path, sizeof(path), "%s/%s%s%s", NOTIFY_PLUGIN_PATH, NOTIFY_PLUGIN_PREFIX, name, NOTIFY_PLUGIN_SUFFIX);

    handle = dlopen(path, RTLD_NOW);
    if ((str = dlerror()) != NULL) {
      fprintf(stderr, "uim-notify: load failed %s(%s)\n", path, str);
      return 0;
    }
    uim_notify_init_func  = (int (*)(void))(intptr_t)dlfunc(handle, "uim_notify_init");
    if (!uim_notify_init_func) {
      fprintf(stderr, "uim-notify: cannot found 'uim_notify_init()' in %s\n", path);
      return 0;
    }
    uim_notify_quit_func  = (void (*)(void))(intptr_t)dlfunc(handle, "uim_notify_quit");
    if (!uim_notify_quit_func) {
      fprintf(stderr, "uim-notify: cannot found 'uim_notify_quit()' in %s\n", path);
      return 0;
    }
    uim_notify_info_func  = (int (*)(const char *, va_list))(intptr_t)dlfunc(handle, "uim_notify_info");
    if (!uim_notify_info_func) {
      fprintf(stderr, "uim-notify: cannot found 'uim_notify_info()' in %s\n", path);
      return 0;
    }
    uim_notify_fatal_func = (int (*)(const char *, va_list))(intptr_t)dlfunc(handle, "uim_notify_fatal");
    if (!uim_notify_fatal_func) {
      fprintf(stderr, "uim-notify: cannot found 'uim_notify_fatal()' in %s\n", path);
      return 0;
    }

    uim_notify_init_func();

    strlcpy(notify_agent_name, name, sizeof(notify_agent_name));
  }
  return 1;
}

int
uim_notify_init(void)
{
  return uim_notify_init_func();
}

void
uim_notify_quit(void)
{
  uim_notify_quit_func();
  return;
}

int
uim_notify_info(const char *msg_fmt, ...)
{
  int ret;
  va_list ap;

  va_start(ap, msg_fmt);
  ret = uim_notify_info_func(msg_fmt, ap);
  va_end(ap);

  return ret;
}

int
uim_notify_fatal(const char *msg_fmt, ...)
{
  int ret;
  va_list ap;

  va_start(ap, msg_fmt);
  ret = uim_notify_fatal_func(msg_fmt, ap);
  va_end(ap);

  return ret;
}



/*
 * builtin functions
 */
static int
uim_notify_stderr_init(void)
{
  return 1;
}

static void
uim_notify_stderr_quit(void)
{
  return;
}

static int
uim_notify_stderr_info(const char *fmt, va_list ap)
{
  char buf[BUFSIZ];

  vsnprintf(buf, sizeof(buf), fmt, ap);

  return fprintf(stderr, "uim [Info]: %s", buf);
}

static int
uim_notify_stderr_fatal(const char *fmt, va_list ap)
{
  char buf[BUFSIZ];

  vsnprintf(buf, sizeof(buf), fmt, ap);

  return fprintf(stderr, "uim [Fatal]: %s", buf);
}

