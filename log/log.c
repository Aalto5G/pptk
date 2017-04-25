#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

atomic_uint global_log_file_level = ATOMIC_VAR_INIT(LOG_LEVEL_INFO);
atomic_uint global_log_console_level = ATOMIC_VAR_INIT(LOG_LEVEL_NOTICE);

static inline uint64_t gettime64(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000UL*1000UL + tv.tv_usec;
}

struct globals {
  FILE *f;
  const char *progname;
  pthread_mutex_t mtx;
  uint64_t last_time64;
  uint32_t burst;
  uint32_t increment;
  uint32_t max;
  uint32_t interval;
  uint32_t missed_events;
};

struct globals globals = {
  .mtx = PTHREAD_MUTEX_INITIALIZER,
  .last_time64 = 0,
  .burst = 1000,
  .increment = 1000,
  .max = 1000,
  .interval = 1000*1000,
  .missed_events = 0,
};

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
  pthread_mutex_lock(&globals.mtx);
  if (globals.burst == 0)
  {
    if (globals.missed_events == 0)
    {
      gettimeofday(&tv, NULL);
      localtime_r(&tv.tv_sec, &tm);
      strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %H:%M:%S", &tm);
      snprintf(
        linebuf, sizeof(linebuf),
        "%s.%.6d {%s} [%s] (%s) <%s:%s:%zu> starting to miss events",
        timebuf, (int)tv.tv_usec, progname, "LOG", "MISSED",
        __FILE__, __FUNCTION__, (size_t)__LINE__);
      if (globals.f)
      {
        fprintf(globals.f, "%s\n", linebuf);
        fflush(globals.f);
      }
      fprintf(stdout, "%s\n", linebuf);
      fflush(stdout);
    }
    globals.missed_events++;
    pthread_mutex_unlock(&globals.mtx);
    return;
  }
  if (progname == NULL)
  {
    progname = "A.OUT";
  }
  gettimeofday(&tv, NULL);
  localtime_r(&tv.tv_sec, &tm);
  strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %H:%M:%S", &tm);
  if (gettime64() - globals.last_time64 > globals.interval)
  {
    globals.last_time64 = gettime64();
    globals.burst += globals.increment;
    if (globals.burst > globals.max)
    {
      globals.burst = globals.max;
    }
    if (globals.missed_events > 0)
    {
      snprintf(
        linebuf, sizeof(linebuf),
        "%s.%.6d {%s} [%s] (%s) <%s:%s:%zu> missed %u events",
        timebuf, (int)tv.tv_usec, progname, "LOG", "MISSED",
        __FILE__, __FUNCTION__, (size_t)__LINE__,
        globals.missed_events);
      if (globals.f)
      {
        fprintf(globals.f, "%s\n", linebuf);
        fflush(globals.f);
      }
      fprintf(stdout, "%s\n", linebuf);
      fflush(stdout);
    }
    globals.missed_events = 0;
  }
  globals.burst--;
  vsnprintf(msgbuf, sizeof(msgbuf), buf, ap);
  snprintf(linebuf, sizeof(linebuf), "%s.%.6d {%s} [%s] (%s) <%s:%s:%zu> %s", timebuf, (int)tv.tv_usec, progname, compname, log_level_string(level), file, function, line, msgbuf);
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
  pthread_mutex_unlock(&globals.mtx);
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
