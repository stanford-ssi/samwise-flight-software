# Adding support for a new command on the satellite


## 1. Create  a new queue for your task
#####  *slate.h file*
```c
queue_t task1_data;
```

## 2. Add a new command identifier 
#####  *command_switch_task.c file*
This will be the unique identifier of this command, when we see this command_id, this task will be added to the task queue.

```c
// example for command 1 
#define COMMAND1_ID 1
```

## 3. Create a data structure for your command data 
#####  *command_switch_task.c file*

If your command is going to take a data structure that is already defined, you can use the same data struct definition for multiple different commands and skip this step

```c
// example of a data structure definition
struct TASK1_DATA_STRUCT_FORMAT
{	
	int data_int_1;	
	uint8_t data_byteArr_1[300];
};
```

## 4. Check if you need to update max_datastructure_size
#####  *slate.h file*

find the line defining `max_datastructure_size`
```c
#define max_datastructure_size 304 // buffer size should be maxsize of biggest datastructure
```
If you are adding a data structure that will be bigger than that constant, update it with the correct size in bytes. You may choose to run `sizeof(your_data_struct)` on your machine to check how big your data structure is.

## 5. Create a new struct for your command   
#####  *command_switch_task.c file*
```c
// example of a data structure instantiation
struct TASK1_DATA_STRUCT_FORMAT current_data_holder_task1;
```

## 6. Set the max number of commands that can be in the queue waiting to be processed

#####  *command_switch_task.c file*
This defines what is the maximum number of this command calls that will be stored in the queue waiting to be executed. You also might choose to not define new constants for every command and rather use a constant of appropriate length that already exists.
```c
const int TASK1_QUEUE_LENGTH = 32; // max queue length for task 1
```


## 7. Initialize the queue responsible for your task

#####  *command_switch_task.c file*
Note that instead of defining `current_data_holder_task1` in step 5, we can also just define a constant with the size of the appropriate and use it instead of `sizeof(current_data_holder_task1)`
Also don't forget to change `TASK1_QUEUE_LENGTH` to the queue size you want to, see step 6 for more details.

```c
queue_init(&slate->task1_data, sizeof(current_data_holder_task1), TASK1_QUEUE_LENGTH);
```


## 8. Add a case to handle your new command

#####  *command_switch_task.c file*
Find the Switch case statement in `command_switch_dispatch` and add a new statement

Make sure you create the correct data structure for your command:
```c 
struct TASK1_DATA_STRUCT_FORMAT task;  // create a data structure of your type!
```
In this line make sure  to pass the queue pointer that points to your unique command queue `&slate->task1_data` and your command identifier `COMMAND1_ID`
*(this queue was initialized in step 1)*

```c
parse_packets_as_command(slate, task_size, COMMAND1_ID, &slate->task1_data);
```

Example of final case statement:
```c
case COMMAND1_ID:{

	struct TASK1_DATA_STRUCT_FORMAT task;
	uint16_t task_size = sizeof(task);
	parse_packets_as_command(slate, task_size, COMMAND1_ID, &slate->task1_data);

}break;
```

## 9. You're all set!
See merging info below, only needs to be done once

#####  *command_switch_task.c file*
- note: when merging with radio, verify the following constant: `PACKET_BYTE_LENGTH`
- note: when merging with radio, make sure to comment out and verify the following:
- note: when merging with radio, delete the `RADIO_PACKETS_OUT_MAX_LENGTH` constant as it will not be used anymore
```c
queue_init(&slate->rx_queue, PACKET_BYTE_LENGTH * sizeof(uint8_t), RADIO_PACKETS_OUT_MAX_LENGTH);
```

## If you have any questions contact:
- Sasha Luchyn GitHub: @Jackal-Studios
- Michael Dalva Github: @spirou7
