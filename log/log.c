#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

atomic_uint global_log_file_level = ATOMIC_VAR_INIT(LOG_LEVEL_INFO);
atomic_uint global_log_console_level = ATOMIC_VAR_INIT(LOG_LEVEL_NOTICE);

struct globals {
  FILE *f;
  const char *progname;
};

struct globals globals;

static inline const char *log_level_string(enum log_level level)
{
  switch (level)
  {
    case LOG_LEVEL_EMERG: return "EMERG";
    case LOG_LEVEL_ALERT: return "ALERT";
    case LOG_LEVEL_CRIT: return "CRIT";
    case LOG_LEVEL_ERR: return "ERR";
    case LOG_LEVEL_WARNING: return "WARNING";
    case LOG_LEVEL_NOTICE: return "NOTICE";
    case LOG_LEVEL_INFO: return "INFO";
    case LOG_LEVEL_DEBUG: return "DEBUG";
    default: return "UNKNOWN";
  }
}

void log_impl_vlog(enum log_level level, const char *compname, const char *file, size_t line, const char *function, const char *buf, va_list ap)
{
  struct timeval tv;
  struct tm tm;
  char msgbuf[16384] = {0};
  char linebuf[16384] = {0};
  char timebuf[256] = {0};
  const char *progname = globals.progname;
  if (progname == NULL)
  {
    progname = "A.OUT";
  }
  gettimeofday(&tv, NULL);
  localtime_r(&tv.tv_sec, &tm);
  strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %H:%M:%S", &tm);
  vsnprintf(msgbuf, sizeof(msgbuf), buf, ap);
  snprintf(linebuf, sizeof(linebuf), "%s.%.6d {%s} [%s] (%s) <%s:%s:%zu> %s", timebuf, (int)tv.tv_usec, globals.progname, compname, log_level_string(level), file, function, line, msgbuf);
  if (globals.f && level <= atomic_load(&global_log_file_level))
  {
    fprintf(globals.f, "%s\n", linebuf);
    fflush(globals.f);
  }
  if (level <= atomic_load(&global_log_console_level))
  {
    fprintf(stdout, "%s\n", linebuf);
    fflush(stdout);
  }
}


void log_impl_log(enum log_level level, const char *compname, const char *file, size_t line, const char *function, const char *buf, ...)
{
  va_list ap;
  va_start(ap, buf);
  log_impl_vlog(level, compname, file, line, function, buf, ap);
  va_end(ap);
}

void log_open(const char *progname, enum log_level file_level, enum log_level console_level)
{
  char logfile[256] = {0};
  char lowerbuf[256] = {0};
  char *it;
  snprintf(lowerbuf, sizeof(lowerbuf), "%s", progname);
  for (it = lowerbuf; *it; it++)
  {
    *it = tolower(*it);
  }
  snprintf(logfile, sizeof(logfile), "%s.log", lowerbuf);
  globals.f = fopen(logfile, "a");
  atomic_init(&global_log_file_level, file_level);
  atomic_init(&global_log_console_level, console_level);
  globals.progname = progname;
}
