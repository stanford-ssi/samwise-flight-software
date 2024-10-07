# Adding a new state

## Add your state code to `states/mystate.c` and `states/mystate.h`

Here you will define your transition condition. For example, if you always want
to transition, you can write:

```c
sm_state_t my_state_get_next_state(slate_t *slate)
{
    return next_state;
}

sched_state_info_t my_state_info  = {
     .name = "my state",
     .num_tasks = 1,
     .task_list = {&my_task, &blink_task},
     .get_next_state = &my_task_get_next_state}
```

If you need to reference another state (e.g. to transition) or task you should
include the top level list headers `state_machine/tasks.h` and
`state_machine/states.h`, not the headers for each task and state.

## Add your state to `states.h`

First add your state to the extern list:

```c
extern sched_state_info_t init_state_info;
extern sched_state_info_t running_state_info;
extern sched_state_info_t my_state_info;
```

Then, add your state to the array:

```c
static sched_state_info_t* all_states[] = {
    /* state_init */
    &init_state_info,
    /* state_running */
    &running_state_info,
    /* my_state */
    &my_state_info};
```

# Adding a new task

## Add your task code to `tasks/mytask.c` and `tasks/mytask.h`

See the other tasks for details, most importantly you will need something like:

```c
sched_task_t my_task = {.name = "my task",
                        .dispatch_period_ms = 1000,
                        .task_init = &my_task_init,
                        .task_dispatch = &my_task_dispatch,

                        /* Set to an actual value on init */
                        .next_dispatch = 0};
```

## Add your task to `tasks.h`

```c
extern sched_task_t print_task;
extern sched_task_t blink_task;
extern sched_task_t my_task;
```

Now you can use your task in states!
