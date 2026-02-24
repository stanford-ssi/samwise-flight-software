#include "state_registry.h"
#include <stddef.h>

static sched_state_t *registry[STATE_COUNT] = {NULL};
static sched_state_t *ordered_states[STATE_COUNT] = {NULL};
static size_t num_registered = 0;

void state_registry_register(state_id_t id, sched_state_t *state)
{
    if (id >= 0 && id < STATE_COUNT && registry[id] == NULL)
    {
        registry[id] = state;
        ordered_states[num_registered++] = state;
    }
}

sched_state_t *state_registry_get(state_id_t id)
{
    if (id < 0 || id >= STATE_COUNT)
        return NULL;
    return registry[id];
}

size_t state_registry_count(void)
{
    return num_registered;
}

sched_state_t *state_registry_get_by_index(size_t i)
{
    if (i < num_registered)
        return ordered_states[i];
    return NULL;
}
