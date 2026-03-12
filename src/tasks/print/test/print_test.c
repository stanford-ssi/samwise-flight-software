#include "print_test.h"

slate_t test_slate;

int main()
{
    if (clear_and_init_slate(&test_slate) != 0)
    {
        LOG_ERROR("Failed to initialize slate for test! Aborting test.");
        return 1;
    }

    LOG_DEBUG("Task pointer: %s", print_task.name);
    ASSERT(strcmp(print_task.name, "print") == 0);
    print_task.task_init(&test_slate);
    print_task.task_dispatch(&test_slate);

    free_slate(&test_slate);
    return 0;
}
