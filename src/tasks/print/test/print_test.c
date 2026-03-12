#include "print_test.h"

slate_t test_slate;

int main()
{
    clear_and_init_slate(&test_slate);
    LOG_DEBUG("Task pointer: %s", print_task.name);
    ASSERT(strcmp(print_task.name, "print") == 0);
    print_task.task_init(&test_slate);
    print_task.task_dispatch(&test_slate);
    free_slate(&test_slate);
    return 0;
}
