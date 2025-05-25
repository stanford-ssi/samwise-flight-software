/**
 * @author  Thomas Haile 
 * @date    2025-05-24
 *
 * Command parsing implementation
 */

#include "command_parser.h"
#include "macros.h"

/// @brief Parse packet and dispatch command to appropriate queue
void dispatch_command(slate_t *slate, packet_t *packet)
{
  slate->number_commands_processed++; 

  Command command_id = (Command)packet->data[0];
  LOG_INFO("Command ID Received: %i", command_id);

  switch (command_id)
  {
    /* Payload Commands */
    case TAKE_PHOTO:
    case DOWNLOAD_PHOTO:
    case TAKE_AND_SEND_PHOTO: 
    {
      PAYLOAD_COMMAND_DATA task;
      strncpy(task.serialized_command, packet.data + 1, sizeof(task.serialized_command) - 1);
      task.serialized_command[sizeof(task.serialized_command) - 1] = '\0';

      if (command_id == TAKE_PHOTO)
        queue_try_add(&slate->take_photo_task_data, &task);
      else if (command_id == DOWNLOAD_PHOTO)
        queue_try_add(&slate->download_photo_task_data, &task);
      else if (command_id == TAKE_AND_SEND_PHOTO)
        queue_try_add(&slate->take_and_send_photo_task_data, &task);

      LOG_INFO("Payload: %s", task.serialized_command);
      break; 
    }

    case NO_OP:
    {
      LOG_INFO("Number of Commands Executed: %d", slate->number_commands_processed);
      break; 
    }

    /* Toggle Commands */
    // TODO: ADD HERE 
    
    default:
      LOG_ERROR("Unknown command ID: %i", command_id);
      break;
  }
}