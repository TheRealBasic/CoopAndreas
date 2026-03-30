#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace ObjectiveSync
{
    enum class ObjectiveTextSemantics : uint8_t
    {
        None = 0,
        Replace = 1,
        Clear = 2
    };

    struct State
    {
        enum : uint8_t
        {
            VEHICLE_TASK_NONE = 0,
            VEHICLE_TASK_ENTER_VEHICLE = 1,
            VEHICLE_TASK_PURSUIT = 2,
            VEHICLE_TASK_DESTROY_TARGET = 3
        };

        enum : uint8_t
        {
            PURSUIT_STATE_NONE = 0,
            PURSUIT_STATE_ACTIVE = 1,
            PURSUIT_STATE_ESCAPING = 2,
            PURSUIT_STATE_COMPLETE = 3,
            PURSUIT_STATE_FAILED = 4
        };

        enum : uint8_t
        {
            DESTROY_ESCAPE_STATE_NONE = 0,
            DESTROY_ESCAPE_STATE_DESTROY_ACTIVE = 1,
            DESTROY_ESCAPE_STATE_DESTROYED = 2,
            DESTROY_ESCAPE_STATE_ESCAPE_ACTIVE = 3,
            DESTROY_ESCAPE_STATE_ESCAPED = 4,
            DESTROY_ESCAPE_STATE_FAILED = 5
        };
        enum : uint8_t
        {
            TARGET_OBJECTIVE_NONE = 0,
            TARGET_OBJECTIVE_KILL = 1,
            TARGET_OBJECTIVE_FLEE = 2,
            TARGET_OBJECTIVE_PROTECT = 3
        };
        enum : uint8_t
        {
            TARGET_LIFECYCLE_NONE = 0,
            TARGET_LIFECYCLE_SPAWN = 1,
            TARGET_LIFECYCLE_ACTIVE = 2,
            TARGET_LIFECYCLE_DOWNED = 3,
            TARGET_LIFECYCLE_DEAD = 4,
            TARGET_LIFECYCLE_ESCAPED = 5
        };
        enum : uint8_t
        {
            STEALTH_STATE_UNDETECTED = 0,
            STEALTH_STATE_SUSPICIOUS = 1,
            STEALTH_STATE_ALERTED = 2,
            STEALTH_STATE_FAILED = 3
        };
        enum : uint8_t
        {
            DETECTION_SOURCE_NONE = 0,
            DETECTION_SOURCE_NPC_VISION = 1 << 0,
            DETECTION_SOURCE_NPC_HEARING = 1 << 1,
            DETECTION_SOURCE_SCRIPTED_ALARM = 1 << 2
        };
        enum : uint8_t
        {
            OBJECTIVE_MODIFIER_NONE = 0,
            OBJECTIVE_MODIFIER_STEALTH_COMPROMISED = 1 << 0,
            OBJECTIVE_MODIFIER_STEALTH_ALERTED = 1 << 1,
            OBJECTIVE_MODIFIER_STEALTH_FAILED = 1 << 2
        };

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
        uint8_t aimingLocked = 0;
        uint8_t firingLocked = 0;
        uint8_t cameraLocked = 0;
        uint8_t hudHidden = 0;
        uint8_t cutscenePhase = 0;
        uint32_t cutsceneSessionToken = 0;
        uint32_t objectiveTextToken = 0;
        uint8_t objectiveTextSemantics = static_cast<uint8_t>(ObjectiveTextSemantics::None);
        char objective[8]{};
        uint8_t respawnEligible = 0;
        uint8_t participantDeathCount = 0;
        uint8_t participantIncapacitationCount = 0;
        uint8_t respawnCount = 0;
        uint8_t missionFailThreshold = 1;
        uint8_t incapacitationFailThreshold = UINT8_MAX;
        uint8_t vehicleTaskState = VEHICLE_TASK_NONE;
        uint8_t pursuitState = PURSUIT_STATE_NONE;
        uint8_t destroyEscapeState = DESTROY_ESCAPE_STATE_NONE;
        uint16_t vehicleTaskSequence = 0;
        uint8_t targetObjectiveType = TARGET_OBJECTIVE_NONE;
        uint8_t targetLifecycleState = TARGET_LIFECYCLE_NONE;
        int32_t targetEntityNetworkId = -1;
        uint8_t targetEntityType = 0;
        uint16_t targetStateSequence = 0;
        uint8_t stealthState = STEALTH_STATE_UNDETECTED;
        uint8_t detectionSourceMask = DETECTION_SOURCE_NONE;
        uint16_t stealthStateSequence = 0;
        uint8_t objectiveModifierFlags = OBJECTIVE_MODIFIER_NONE;
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

    inline uint32_t BuildObjectiveTextToken(const char* text)
    {
        if (!text || !text[0])
        {
            return 0;
        }

        constexpr uint32_t offset = 2166136261u;
        constexpr uint32_t prime = 16777619u;
        uint32_t hash = offset;
        for (int i = 0; i < 8 && text[i]; ++i)
        {
            hash ^= static_cast<uint8_t>(text[i]);
            hash *= prime;
        }
        return hash;
    }

    inline bool ApplyObjectiveTextEvent(State& state, ObjectiveTextSemantics semantics, const char* text)
    {
        char nextObjective[8]{};
        if (semantics == ObjectiveTextSemantics::Replace && text && text[0])
        {
            strncpy_s(nextObjective, text, sizeof(nextObjective));
        }

        const uint32_t nextToken = (semantics == ObjectiveTextSemantics::Replace) ? BuildObjectiveTextToken(nextObjective) : 0;
        if (state.objectiveTextSemantics == static_cast<uint8_t>(semantics)
            && SameObjective(state.objective, nextObjective)
            && state.objectiveTextToken == nextToken)
        {
            return false;
        }

        state.objectiveTextSemantics = static_cast<uint8_t>(semantics);
        state.objectiveTextToken = nextToken;
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

    inline bool ApplyStealthStateTransition(State& state, uint8_t nextState, uint8_t sourceMask)
    {
        nextState = static_cast<uint8_t>(std::clamp<int>(nextState, State::STEALTH_STATE_UNDETECTED, State::STEALTH_STATE_FAILED));
        sourceMask &= static_cast<uint8_t>(State::DETECTION_SOURCE_NPC_VISION
            | State::DETECTION_SOURCE_NPC_HEARING
            | State::DETECTION_SOURCE_SCRIPTED_ALARM);

        if (state.stealthState == nextState)
        {
            if (sourceMask == 0 || (state.detectionSourceMask & sourceMask) == sourceMask)
            {
                return false;
            }

            state.detectionSourceMask |= sourceMask;
            state.objectiveModifierFlags =
                state.stealthState == State::STEALTH_STATE_FAILED ? State::OBJECTIVE_MODIFIER_STEALTH_FAILED :
                state.stealthState == State::STEALTH_STATE_ALERTED ? (State::OBJECTIVE_MODIFIER_STEALTH_COMPROMISED | State::OBJECTIVE_MODIFIER_STEALTH_ALERTED) :
                state.stealthState == State::STEALTH_STATE_SUSPICIOUS ? State::OBJECTIVE_MODIFIER_STEALTH_COMPROMISED :
                State::OBJECTIVE_MODIFIER_NONE;
            return true;
        }

        if (nextState < state.stealthState)
        {
            return false;
        }

        state.stealthState = nextState;
        state.detectionSourceMask |= sourceMask;
        state.objectiveModifierFlags =
            state.stealthState == State::STEALTH_STATE_FAILED ? State::OBJECTIVE_MODIFIER_STEALTH_FAILED :
            state.stealthState == State::STEALTH_STATE_ALERTED ? (State::OBJECTIVE_MODIFIER_STEALTH_COMPROMISED | State::OBJECTIVE_MODIFIER_STEALTH_ALERTED) :
            state.stealthState == State::STEALTH_STATE_SUSPICIOUS ? State::OBJECTIVE_MODIFIER_STEALTH_COMPROMISED :
            State::OBJECTIVE_MODIFIER_NONE;
        if (state.stealthStateSequence < UINT16_MAX)
        {
            ++state.stealthStateSequence;
        }
        return true;
    }

    inline ApplyResult ApplyOpcode(State& state, uint16_t opcode, const int* params, uint16_t paramCount, const char* text)
    {
        ApplyResult result{};
        auto promoteTargetState = [&](uint8_t objectiveType, uint8_t lifecycleState, uint8_t entityType, int32_t entityNetworkId)
        {
            const bool changed = state.targetObjectiveType != objectiveType
                || state.targetLifecycleState != lifecycleState
                || state.targetEntityType != entityType
                || state.targetEntityNetworkId != entityNetworkId;
            if (!changed)
            {
                return;
            }

            state.targetObjectiveType = objectiveType;
            state.targetLifecycleState = lifecycleState;
            state.targetEntityType = entityType;
            state.targetEntityNetworkId = entityNetworkId;
            if (state.targetStateSequence < UINT16_MAX)
            {
                ++state.targetStateSequence;
            }
            result.changed = true;
            result.progressionChanged = true;
        };

        switch (opcode)
        {
        case 0x05E2: // task_kill_char_on_foot
        case 0x0634: // task_kill_char_on_foot_while_ducking
            if (params && paramCount > 1)
            {
                promoteTargetState(State::TARGET_OBJECTIVE_KILL, State::TARGET_LIFECYCLE_ACTIVE, 1, params[1]);
            }
            break;
        case 0x0603: // task_go_to_coord_any_means
            if (params && paramCount > 0)
            {
                promoteTargetState(State::TARGET_OBJECTIVE_FLEE, State::TARGET_LIFECYCLE_ACTIVE, 1, params[0]);
            }
            break;
        case 0x05BF: // task_look_at_char
            if (params && paramCount > 1)
            {
                promoteTargetState(State::TARGET_OBJECTIVE_PROTECT, State::TARGET_LIFECYCLE_ACTIVE, 1, params[1]);
            }
            break;
        case 0x05CA: // task_enter_car_as_passenger
        case 0x05CB: // task_enter_car_as_driver
            if (state.vehicleTaskState != State::VEHICLE_TASK_ENTER_VEHICLE)
            {
                state.vehicleTaskState = State::VEHICLE_TASK_ENTER_VEHICLE;
                if (state.vehicleTaskSequence < UINT16_MAX)
                {
                    ++state.vehicleTaskSequence;
                }
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x0713: // task_drive_by (pursuit/combat vehicle phase)
            if (state.vehicleTaskState != State::VEHICLE_TASK_PURSUIT
                || state.pursuitState != State::PURSUIT_STATE_ACTIVE)
            {
                state.vehicleTaskState = State::VEHICLE_TASK_PURSUIT;
                state.pursuitState = State::PURSUIT_STATE_ACTIVE;
                if (state.destroyEscapeState == State::DESTROY_ESCAPE_STATE_NONE
                    || state.destroyEscapeState == State::DESTROY_ESCAPE_STATE_DESTROYED)
                {
                    state.destroyEscapeState = State::DESTROY_ESCAPE_STATE_ESCAPE_ACTIVE;
                }
                if (state.vehicleTaskSequence < UINT16_MAX)
                {
                    ++state.vehicleTaskSequence;
                }
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x0672: // task_destroy_car
            if (state.vehicleTaskState != State::VEHICLE_TASK_DESTROY_TARGET
                || state.destroyEscapeState != State::DESTROY_ESCAPE_STATE_DESTROY_ACTIVE)
            {
                state.vehicleTaskState = State::VEHICLE_TASK_DESTROY_TARGET;
                state.destroyEscapeState = State::DESTROY_ESCAPE_STATE_DESTROY_ACTIVE;
                if (state.vehicleTaskSequence < UINT16_MAX)
                {
                    ++state.vehicleTaskSequence;
                }
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
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

                const uint8_t nextAimingLocked = nextState ? 0 : 1;
                if (state.aimingLocked != nextAimingLocked)
                {
                    state.aimingLocked = nextAimingLocked;
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
            if (ApplyObjectiveTextEvent(state, text && text[0] ? ObjectiveTextSemantics::Replace : ObjectiveTextSemantics::Clear, text))
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
            if (state.vehicleTaskState == State::VEHICLE_TASK_PURSUIT && state.pursuitState != State::PURSUIT_STATE_COMPLETE)
            {
                state.pursuitState = State::PURSUIT_STATE_ESCAPING;
                result.changed = true;
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
        case 0x0610: // set_hearing_range
            if (ApplyStealthStateTransition(state, State::STEALTH_STATE_SUSPICIOUS, State::DETECTION_SOURCE_NPC_HEARING))
            {
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x0753: // set_alertness
            if (params && paramCount > 1)
            {
                const int alertness = std::max(params[1], 0);
                uint8_t nextStealthState = State::STEALTH_STATE_UNDETECTED;
                if (alertness >= 2)
                {
                    nextStealthState = State::STEALTH_STATE_ALERTED;
                }
                else if (alertness >= 1)
                {
                    nextStealthState = State::STEALTH_STATE_SUSPICIOUS;
                }

                if (ApplyStealthStateTransition(state, nextStealthState, State::DETECTION_SOURCE_NPC_VISION))
                {
                    result.changed = true;
                    result.progressionChanged = true;
                }
            }
            break;
        case 0x0147: // give_car_alarm
            if (ApplyStealthStateTransition(state, State::STEALTH_STATE_ALERTED, State::DETECTION_SOURCE_SCRIPTED_ALARM))
            {
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x0318: // mission_passed
            if (state.passFailPending == 0)
            {
                state.passFailPending = 1;
                if (state.destroyEscapeState == State::DESTROY_ESCAPE_STATE_DESTROY_ACTIVE)
                {
                    state.destroyEscapeState = State::DESTROY_ESCAPE_STATE_DESTROYED;
                }
                else if (state.destroyEscapeState == State::DESTROY_ESCAPE_STATE_ESCAPE_ACTIVE)
                {
                    state.destroyEscapeState = State::DESTROY_ESCAPE_STATE_ESCAPED;
                }
                if (state.pursuitState == State::PURSUIT_STATE_ACTIVE || state.pursuitState == State::PURSUIT_STATE_ESCAPING)
                {
                    state.pursuitState = State::PURSUIT_STATE_COMPLETE;
                }
                result.changed = true;
                result.progressionChanged = true;
            }
            break;
        case 0x045C: // mission_failed
            if (state.passFailPending == 0)
            {
                state.passFailPending = 2;
                state.pursuitState = State::PURSUIT_STATE_FAILED;
                state.destroyEscapeState = State::DESTROY_ESCAPE_STATE_FAILED;
                ApplyStealthStateTransition(state, State::STEALTH_STATE_FAILED, State::DETECTION_SOURCE_NONE);
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
