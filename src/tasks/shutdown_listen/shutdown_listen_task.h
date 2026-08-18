/**
 * @author  Luis C
 * @date    2026-08-12
 *
 * Receive-only listener used exclusively by the shutdown state.
 *
 * While the satellite is in shutdown (communication blackout, FCC 47 CFR
 * §97.113(a)(4)) it must not transmit or take any action. This task keeps the
 * radio in receive mode and inspects incoming packets, acting ONLY on the
 * REACTIVATE command. Nothing is ever transmitted, and no other command is
 * dispatched. Reactivation requires ground authorization: three consecutive
 * authenticated REACTIVATE commands, mirroring the three-command shutdown
 * trigger.
 */

#pragma once

#include "macros.h"
#include "slate.h"
#include "state_machine.h"

// LED color for the shutdown listener - dim indigo (same family as shutdown)
#define SHUTDOWN_LISTEN_TASK_COLOR 25, 0, 45

// Consecutive REACTIVATE commands required to leave shutdown.
#define REACTIVATE_CMD_THRESHOLD 3

void shutdown_listen_task_init(slate_t *slate);
void shutdown_listen_task_dispatch(slate_t *slate);

extern sched_task_t shutdown_listen_task;
