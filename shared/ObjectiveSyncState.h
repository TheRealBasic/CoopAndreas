#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace ObjectiveSync
{
    struct State
    {
        uint16_t missionId = 0;
        int32_t timerMs = 0;
        uint16_t objectivePhaseIndex = 0;
        uint16_t checkpointIndex = 0;
        uint16_t checkpointCount = 0;
        uint8_t timerVisible = 0;
        uint8_t timerFrozen = 0;
        uint8_t timerDirection = 0;
        uint8_t passFailPending = 0;
        uint8_t playerControlState = 0;
        uint8_t movementLocked = 0;
        uint8_t firingLocked = 0;
        uint8_t cameraLocked = 0;
        uint8_t hudHidden = 0;
        uint8_t cutscenePhase = 0;
        uint32_t cutsceneSessionToken = 0;
        char objective[8]{};
    };

    struct ApplyResult
    {
        bool changed = false;
        bool progressionChanged = false;
    };

    inline bool SameObjective(const char* lhs, const char* rhs)
    {
        return std::memcmp(lhs, rhs, 8) == 0;
    }

    inline bool UpdateObjective(State& state, const char* text)
    {
        char nextObjective[8]{};
        if (text && text[0])
        {
            strncpy_s(nextObjective, text, sizeof(nextObjective));
        }

        if (SameObjective(state.objective, nextObjective))
        {
            return false;
        }

        std::memcpy(state.objective, nextObjective, sizeof(state.objective));
        if (state.objectivePhaseIndex < UINT16_MAX)
        {
            ++state.objectivePhaseIndex;
        }
        return true;
    }

    inline void ResetForMissionStart(State& state, uint16_t missionId)
    {
        state = {};
        state.missionId = missionId;
    }

    inline bool ApplyTimerVisibility(State& state, bool visible)
    {
        const uint8_t nextVisible = visible ? 1 : 0;
        if (state.timerVisible == nextVisible)
        {
            return false;
        }

        state.timerVisible = nextVisible;
        return true;
    }

    inline ApplyResult ApplyOpcode(State& state, uint16_t opcode, const int* params, uint16_t paramCount, const char* text)
    {
        ApplyResult result{};

        switch (opcode)
        {
        case 0x0417: // start_mission
        {
            const uint16_t missionId = (params && paramCount > 0) ? static_cast<uint16_t>(std::max(params[0], 0)) : 0;
            ResetForMissionStart(state, missionId);
            result.changed = true;
            result.progressionChanged = true;
            break;
        }
        case 0x01B4: // set_player_control
            if (params && paramCount > 1)
            {
                const uint8_t nextState = params[1] ? 1 : 0;
                if (state.playerControlState != nextState)
                {
                    state.playerControlState = nextState;
                    result.changed = true;
                }

                const uint8_t nextMovementLocked = nextState ? 0 : 1;
                if (state.movementLocked != nextMovementLocked)
                {
                    state.movementLocked = nextMovementLocked;
                    result.changed = true;
                }
            }
            break;
        case 0x0881: // set_player_fire_button
            if (params && paramCount > 1)
            {
                const uint8_t nextFiringLocked = params[1] ? 0 : 1;
                if (state.firingLocked != nextFiringLocked)
                {
                    state.firingLocked = nextFiringLocked;
                    result.changed = true;
                }
            }
            break;
        case 0x0E60: // set_camera_control
            if (params && paramCount > 0)
            {
                const uint8_t nextCameraLocked = params[0] ? 0 : 1;
                if (state.cameraLocked != nextCameraLocked)
                {
                    state.cameraLocked = nextCameraLocked;
                    result.changed = true;
                }
            }
            break;
        case 0x0826: // display_hud
            if (params && paramCount > 0)
            {
                const uint8_t nextHudHidden = params[0] ? 0 : 1;
                if (state.hudHidden != nextHudHidden)
                {
                    state.hudHidden = nextHudHidden;
                    result.changed = true;
                }
            }
            break;
        case 0x00BA: // print_big
        case 0x00BC: // print_now
        case 0x03E5: // print_help
            if (UpdateObjective(state, text))
            {
                if (state.checkpointIndex < UINT16_MAX)
                {
                    ++state.checkpointIndex;
                }
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x06D5: // create_checkpoint
            if (state.checkpointCount < UINT16_MAX)
            {
                ++state.checkpointCount;
                result.changed = true;
            }
            if (state.checkpointIndex == 0)
            {
                state.checkpointIndex = 1;
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x07F3: // set_checkpoint_coords
            if (state.checkpointIndex < UINT16_MAX)
            {
                ++state.checkpointIndex;
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x014E: // display_onscreen_timer
        case 0x03C3: // display_onscreen_timer_with_string
            result.changed |= ApplyTimerVisibility(state, true);
            if (params && paramCount > 1)
            {
                const uint8_t nextDirection = static_cast<uint8_t>(std::clamp(params[1], 0, 255));
                if (state.timerDirection != nextDirection)
                {
                    state.timerDirection = nextDirection;
                    result.changed = true;
                }
            }
            if (state.timerFrozen != 0)
            {
                state.timerFrozen = 0;
                result.changed = true;
            }
            break;
        case 0x014F: // clear_onscreen_timer
            result.changed |= ApplyTimerVisibility(state, false);
            if (state.timerFrozen != 0)
            {
                state.timerFrozen = 0;
                result.changed = true;
            }
            if (state.timerMs != 0)
            {
                state.timerMs = 0;
                result.changed = true;
            }
            break;
        case 0x0396: // freeze_onscreen_timer
            if (params && paramCount > 0)
            {
                const uint8_t nextFrozen = params[0] ? 1 : 0;
                if (state.timerFrozen != nextFrozen)
                {
                    state.timerFrozen = nextFrozen;
                    result.changed = true;
                }
            }
            break;
        case 0x0890: // set_timer_beep_countdown_time
            if (params && paramCount > 1)
            {
                const int32_t nextTimerMs = std::max(params[1], 0) * 1000;
                if (state.timerMs != nextTimerMs)
                {
                    state.timerMs = nextTimerMs;
                    result.changed = true;
                }
            }
            break;
        case 0x0318: // mission_passed
            if (state.passFailPending == 0)
            {
                state.passFailPending = 1;
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x045C: // mission_failed
            if (state.passFailPending == 0)
            {
                state.passFailPending = 2;
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        default:
            break;
        }

        return result;
    }
}
