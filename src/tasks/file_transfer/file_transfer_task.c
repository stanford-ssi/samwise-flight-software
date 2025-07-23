/**
 * @author  Marc Aaron Reyes
 * @date    2025-07-21
 */

#include "file_transfer_task.h"
#include "slate.h"

void file_transfer_task_init(slate_t *slate)
{
    LOG_INFO("Pulling initial command packet...");

    if (queue_is_empty(&slate->ft_header_data))
    {
        LOG_DEBUG("Queue seems to be empty, aborting file transfer...");
        // TODO: Change state here
        return;
    }

    bool is_file_created;
    FILE_TRANSFER_DATA ft_init_packet;
    if (queue_try_peek(&slate->ft_header_data, &ft_init_packet))
    {
        /*
        is_file_created = check_file_status(ft_init_packet.file_name);
        if (!send_initial_resp_packet(is_file_created))
        {
            LOG_DEBUG("Failed to send response packet...");
        }
        */
    }
    else
    {
        LOG_DEBUG("Failed to peek header command from queue...");
        // TODO: Change state here
        return;
    }

    LOG_INFO("Initializing environment to receive file packets...");
    strlcpy(slate->curr_file, ft_init_packet.file_name,
            strlen(ft_init_packet.file_name));
}

void file_transfer_task_dispatch(slate_t *slate)
{
}

sched_task_t file_transfer_task = {.name = "file_transfer",
                                   .dispatch_period_ms = 100,
                                   .task_init = &file_transfer_task_init,
                                   .task_dispatch =
                                       &file_transfer_task_dispatch,
                                   .next_dispatch = 0};
