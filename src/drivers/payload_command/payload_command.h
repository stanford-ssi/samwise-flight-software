/**
 * @author Marc Aaron Reyes
 * @date 2025-05-21
 *
 */
#include "slate.h"

typedef bool (*command_fn_t)();

enum
{

};

typedef struct
{
    char argument_name;

} command_arg_t;

typedef struct
{
    int num_args;
    command_arg_t *args;
} command_args_list_t;

typedef struct
{
    const char *name;
    command_fn_t fn;
    command_args_t *args;
} command_t;

bool init_payload(slate_t *slate);
uint16_t payload_send_command(slate_t *slate, const uint8_t *command,
                              uint8_t *report);
