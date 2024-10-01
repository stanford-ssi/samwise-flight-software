# Adding a new state

## Add your state header to `states/`

Here you will define your transition condition. For example, if you never want
to transition, you can write:

```c
static sm_state_t my_state_get_next_state(slate_t *slate)
{
    return next_state;
}
```

Note that you do **not** need to include the scheduler headers!

## Add your state to `states.h`

First, add your state to the enum:

```c
enum sm_state
{
    state_init, /* Entered by default at boot */
    state_running,

    /* My new state! */
    my_state,

    /* Auto-updates to the number of sates */
    num_states
};
```

Then, add your state to the array:

```c
static sched_state_info_t all_states[] = {
    /* state_init */
    {.name = "init",
     .num_tasks = 0,
     .task_list = {},
     .get_next_state = &init_get_next_state},

    /* state_running */
    {.name = "running",
     .num_tasks = 2,
     .task_list = {&print_task, &blink_task},
     .get_next_state = &running_get_next_state},
    
    {.name = "my state",
     .num_tasks = 1,
     .task_list = {&my_task, &blink_task},
     .get_next_state = &my-task_get_next_state}};
```

# Adding a new task
