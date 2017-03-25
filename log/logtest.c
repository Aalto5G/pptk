#include "log.h"

int main(int argc, char **argv)
{
  log_open("LOGTEST", LOG_LEVEL_DEBUG, LOG_LEVEL_NOTICE);
  log_log(LOG_LEVEL_DEBUG, "MAIN", "debug message");
  log_log(LOG_LEVEL_NOTICE, "MAIN", "notice message");
  return 0;
}
