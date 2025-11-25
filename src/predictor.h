#pragma once

#include "lib/types.h"
#include "raylib.h"

#include "constants.h"
#include "simulation.h"
#include <stdio.h>

typedef struct {
    Vector2 positions[PREDICT_LENGTH];
} Prediction;

typedef struct {
    Prediction *predictions;
    const SimulationState *simulation;
} PredictorState;

void predictor_init(PredictorState *predictor, const SimulationState *simulation);
void predictor_update(PredictorState *predictor, Planet *additional_planet, f32 delta_time);

#ifdef PREDICTOR_IMPLEMENTATION

void predictor_init(PredictorState *predictor, const SimulationState *simulation) {
    if (predictor->predictions != NULL) {
        arrfree(predictor->predictions);
        predictor->predictions = NULL;
    }

    predictor->simulation = simulation;
}

void predictor_update(PredictorState *predictor, Planet *new_planet, f32 delta_time) {
    SimulationState simulation_copy = (SimulationState) {
        .parameters = predictor->simulation->parameters,
        .planets = NULL
    };

    arrsetcap(simulation_copy.planets, arrlen(predictor->simulation->planets));
    for (usize i = 0; i < arrlenu(predictor->simulation->planets); i++) {
        arrpush(simulation_copy.planets, predictor->simulation->planets[i]);
    }

    if (new_planet != NULL) arrpush(simulation_copy.planets, *new_planet);
    arrsetlen(predictor->predictions, arrlen(simulation_copy.planets));

    for (usize step = 0; step < PREDICT_LENGTH; step++) {
        simulation_update(&simulation_copy, delta_time);

        for (usize i = 0; i < arrlenu(predictor->predictions); i++) {
            predictor->predictions[i].positions[step] = simulation_copy.planets[i].position;
        }
    }

    arrfree(simulation_copy.planets);
}

#endif

