#pragma once

#include "roots.h"
#include "delineation.h"


namespace smtrat::cadcells::datastrucutres {
    struct cell_representation {
        cell interval;
        indexed_root_ordering ordering;
        std::vector<poly_ref> equational;
        const delineation& delineation;
    };

    struct covering_representation {
        std::vector<cell_representation> cell_representations;
        covering get_covering() {
            covering cov;
            for (const auto& cell_repr : cell_representations) {
                cov.push_back(cell_repr.cell);
            }
            return cov;
        }
    };
}