//
// Created by Magnus Nordlander on 2023-11-09.
//

#ifndef RP2040_BIANCA_ROUTINESTEPEXITCONDITION_H
#define RP2040_BIANCA_ROUTINESTEPEXITCONDITION_H


#include <cstdint>

typedef enum {
    BREW_START,
    BREW_TIME_ABSOLUTE,
    STEP_TIME,
    // [MOD] Fires when a brew that was running ends. Only takes effect on the step active at
    // that moment - see Automations::onBrewEnded(), which checks for this before falling back
    // to its unconditional reset to step 0.
    BREW_END,
} RoutineStepExitConditionType;

class RoutineStepExitCondition {
public:
    explicit RoutineStepExitCondition(RoutineStepExitConditionType type, float value, uint16_t exitToStep) :
        type(type),
        value(value),
        exitToStep(exitToStep) {}

    RoutineStepExitConditionType type;
    float value;
    uint16_t exitToStep;
};


#endif //RP2040_BIANCA_ROUTINESTEPEXITCONDITION_H
