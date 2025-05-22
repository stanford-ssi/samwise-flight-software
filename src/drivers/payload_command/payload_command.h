/**
 * @author Marc Aaron Reyes
 * @date 2025-05-21
 *
 */
#include "slate.h"

typedef int (*command_fn_t)();

typedef struct
{
    const char *name;
    command_fn_t fn;
} command_t;

bool init_payload(slate_t *slate);
uint16_t payload_send_command(slate_t *slate, const uint8_t *command,
                              uint8_t *report);
