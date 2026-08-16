#include "burn_wire_state.h"
#include "device_status.h"
#include "flash.h"
#include "logger.h"

extern sched_state_t burn_wire_state;

state_id_t burn_wire_get_next_state(slate_t *slate)
{
    /*
     * Read the deploy detect pins directly: the telemetry task does not run in
     * this state, so the slate would otherwise be stale. Cache the readings so
     * the beacon reports them once we reach the running state.
     */
    bool panel_A_deployed = is_flex_panel_A_deployed();
    bool panel_B_deployed = is_flex_panel_B_deployed();
    slate->panel_A_deployed = panel_A_deployed;
    slate->panel_B_deployed = panel_B_deployed;

    if (panel_A_deployed && panel_B_deployed)
    {
        LOG_INFO("burn_wire: both panels successfully deployed");
        return STATE_RUNNING;
    }

    uint32_t burn_wire_attempts = get_burn_wire_attempts();
    if (burn_wire_attempts >= MAX_BURN_WIRE_ATTEMPTS)
    {
        if (panel_A_deployed != panel_B_deployed)
        {
            /*
             * Only one side released - something is mechanically wrong. We
             * cannot do anything about it up here, so log it and carry on into
             * the running state so the radio comes up and the ground can see
             * the two panel bits in the beacon.
             */
            LOG_ERROR("burn_wire: asymmetric deploy, A=%d B=%d. panel did not "
                      "deploy after %d attempts",
                      panel_A_deployed, panel_B_deployed, burn_wire_attempts);
        }
        else
        {
            LOG_ERROR("burn_wire: neither panel deployed after %d attempts",
                      burn_wire_attempts);
        }
        return STATE_RUNNING;
    }

    LOG_INFO("burn_wire: retrying (A=%d B=%d, attempt %d/%d)", panel_A_deployed,
             panel_B_deployed, burn_wire_attempts, MAX_BURN_WIRE_ATTEMPTS);
    return STATE_BURN_WIRE;
}

sched_state_t burn_wire_state = {.name = "burn_wire",
                                 .id = STATE_BURN_WIRE,
                                 .num_tasks = 1,
                                 .task_list = {&burn_wire_task},
                                 .get_next_state = &burn_wire_get_next_state};
