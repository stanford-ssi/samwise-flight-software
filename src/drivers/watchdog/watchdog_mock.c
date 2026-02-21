#include "watchdog.h"

struct watchdog watchdog_mk()
{
    struct watchdog mock = {0};
    return mock;
}
void watchdog_init(struct watchdog *wd)
{
    // TODO: Track init calls for test assertions
}
void watchdog_feed(struct watchdog *wd)
{
    // TODO: Track feed count/timing to verify watchdog task behavior
}
