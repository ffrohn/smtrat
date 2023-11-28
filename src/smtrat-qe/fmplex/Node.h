#pragma once

#include <smtrat-common/smtrat-common.h>
#include "../util/Matrix.h"
#include <carl-common/util/streamingOperators.h>

namespace smtrat::qe::fmplex {

struct Node {
    using Matrix   = qe::util::Matrix;
    using RowIndex = Matrix::RowIndex;
    using ColIndex = Matrix::ColIndex;
    
    enum class Type { CONFLICT, LEAF, LBS, UBS, NBS, FM };

    Matrix   matrix;
    Type     type;
    ColIndex chosen_col;
    std::vector<ColIndex> cols_to_elim;
    std::vector<RowIndex> eliminators;
    std::set<RowIndex> ignored;

    static constexpr ColIndex DUMMY_COL = 0; // TODO: this should be a dummy-row?

    void choose_elimination() {
        ignored.clear();

        if (matrix.n_rows() == 0 || cols_to_elim.empty()) {
            type = Type::LEAF;
            return;
        }

        if (cols_to_elim.size() == 1) {
            type = Type::FM;
            chosen_col = cols_to_elim.front();
            eliminators = {DUMMY_COL};
            return;
        }

        // find best column
        std::size_t min_branches = matrix.n_rows();
        for (const auto j : cols_to_elim) {
            std::size_t lbs = 0, ubs = 0;
            for (const auto& entry : matrix.col_entries(j)) {
                if (entry.value < 0) ++lbs;
                else ++ubs;
            }
            std::size_t min_j = std::min(lbs, ubs);
            if (min_j == 0) {
                chosen_col = j;
                type = Type::NBS;
                break;
            } else if (min_j < min_branches) {
                min_branches = min_j;
                chosen_col = j;
                type = (lbs == min_j) ? Type::LBS : Type::UBS;
            }
        }

        // gather eliminators
        auto col_it  =  matrix.col_begin(chosen_col);
        auto col_end =  matrix.col_end(chosen_col);

        auto ign_it = ignored.begin();
        switch (type) {
        case Type::LBS:
            for (; col_it != col_end; ++col_it) {
                if (ign_it != ignored.end() && *ign_it == col_it.row()) ++ign_it;
                else if (col_it->value < 0) eliminators.push_back(col_it.row());
            }
            break;
        case Type::UBS:
            for (; col_it != col_end; ++col_it){
                if (ign_it != ignored.end() && *ign_it == col_it.row()) ++ign_it;
                else if (col_it->value > 0) eliminators.push_back(col_it.row());
            }
            break;
        case Type::NBS:
            for (; col_it != col_end; ++col_it){
                eliminators.push_back(col_it.row());
            }
            break;
        default:
            break;
        }
    }

    Node(bool is_sat) {
        type = is_sat ? (Type::LEAF) : (Type::CONFLICT);
        eliminators.clear();
    } 

    Node(const Matrix& m, const std::vector<ColIndex>& cols)
    : matrix(m)
    , cols_to_elim(cols)
    { choose_elimination(); }

    Node(const Matrix& m, const std::vector<ColIndex>& cols, const std::set<RowIndex>& ign)
    : matrix(m)
    , cols_to_elim(cols)
    , ignored(ign)
    { choose_elimination(); }

    Node(Matrix&& m, const std::vector<ColIndex>& cols)
    : matrix(std::move(m))
    , cols_to_elim(cols)
    { choose_elimination(); }


    inline bool is_conflict() const { return type == Node::Type::CONFLICT; }
    inline bool is_finished() const { return eliminators.empty(); }

    static Node conflict() { return Node(false); }
    static Node leaf()     { return Node(true); }
};


template<typename T>
std::ostream& print_vec(std::ostream& os, const std::vector<T>& vec) {
    os << "[" << vec.size() << ": ";
    for (const auto& t : vec) os << t << ", ";
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const smtrat::qe::fmplex::Node& n) {
    os << "\n========== NODE ============\n";
    switch (n.type) {
    using Type = smtrat::qe::fmplex::Node::Type;
    case Type::CONFLICT: os << "CONFLICT\n"; return os;
    case Type::LEAF: os << "Leaf\n"; return os;
    case Type::LBS: os << "LBS"; break;
    case Type::UBS: os << "UBS"; break;
    case Type::NBS: os << "NBS"; break;
    case Type::FM:  os << "FM";  break;
    }
    os << "| Chose col " << n.chosen_col << " out of ";
    print_vec(os, n.cols_to_elim);
    os << "\n";
    os << "Total n. rows:" << n.matrix.n_rows() << ", Eliminators: ";
    print_vec(os, n.eliminators);
    os << "\n\n";
    for (std::size_t i = 0; i < n.matrix.n_rows(); ++i) {
        std::vector<qe::util::Matrix::RowEntry> es;
        for (const auto& e: n.matrix.row_entries(i)) es.push_back(e);
        print_vec(os, es);
        os << "\n";
    }
    os << "============================\n";
	return os;
}

}