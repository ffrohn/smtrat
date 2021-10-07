#pragma once

#include "../datastructures/representation.h"

namespace smtrat::cadcells::representation {
    enum CellHeuristic {
        BIGGEST_CELL, CHAIN_EQ, LOWEST_DEGREE_BARRIERS, LOWEST_DEGREE_BARRIERS_EQ
    };

    enum CoveringHeuristic {
        DEFAULT_COVERING
    };

    enum DelineationHeuristic {
        CHAIN
    };

    template<CellHeuristic H>
    struct cell {
        template<typename T>
        static std::optional<datastructures::CellRepresentation<T>> compute(datastructures::SampledDerivationRef<T>& der);
    };

    template<CoveringHeuristic H>
    struct covering {
        template<typename T>
        static std::optional<datastructures::CoveringRepresentation<T>> compute(const std::vector<datastructures::SampledDerivationRef<T>>& ders);
    };

    template<DelineationHeuristic H>
    struct delineation {
        template<typename T>
        static std::optional<datastructures::DelineationRepresentation<T>> compute(datastructures::DelineatedDerivationRef<T>& der);
    };
}

#include "heuristics_cell.h"
#include "heuristics_covering.h"
#include "heuristics_delineation.h"

