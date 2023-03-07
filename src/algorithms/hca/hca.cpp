//
// Created by eduard on 06.03.23.
//

#include "hca.h"

// for fd mining
#include "dfd.h"

unsigned long long algos::HCA::ExecuteInternal() {
    // time
    auto start_time = std::chrono::system_clock::now();

    // find all fds

    fds_algorithm_.reset(new DFD());

    // fds_map
    std::map<boost::dynamic_bitset<>, boost::dynamic_bitset<>> fds;
    for (auto const& fd : fds_algorithm_->FdList()) {
        for (auto& [lhs, rhs] : fds) {
            fds[lhs | fd.GetLhs().GetColumnIndices()]
                .set(fd.GetRhs().GetIndex());
        }
        fds[fd.GetLhs().GetColumnIndices()].set(fd.GetRhs().GetIndex());
    }

    // HCA algorithm
    std::vector<boost::dynamic_bitset<>> non_uniques(0);
    for (size_t column_index = 0; column_index < schema_->GetNumColumns(); column_index++) {
        boost::dynamic_bitset<> candidate(schema_->GetNumColumns(), 0);
        candidate.set(column_index);
        if (IsUnique(candidate)) {
            RegisterUnique(Vertical(schema_, candidate));
        } else {
            non_uniques.push_back(candidate);
            StoreHistogram(candidate);
        }
    }
    size_t max_k = non_uniques.size();
    for (size_t k = 2; k <= max_k; k++) {
        //  generating candidates with k columns
        auto k_candidates = CandidateGeneration(non_uniques, k - 1);
        std::vector<bool> is_futile(k_candidates.size(), false);
        non_uniques.clear();
        for (size_t candidate_index = 0; candidate_index < k_candidates.size(); candidate_index++) {
            if (is_futile[candidate_index]) {
                continue;
            }
            auto const& candidate = k_candidates[candidate_index];
            if (IsPrunedByHistogram(candidate)) {
                non_uniques.push_back(candidate);
                continue;
            }
            if (IsUnique(candidate)) {
                RegisterUnique(Vertical(schema_, candidate));
                for (size_t i = 0; i < k_candidates.size(); i++) {
                    auto const& k_candidate = k_candidates[i];
                    if ((fds[k_candidate] & candidate) == candidate) {
                        RegisterUnique(Vertical(schema_, k_candidate));
                        is_futile[i] = true;
                    }
                }
            } else {
                non_uniques.push_back(candidate);
                for (size_t i = 0; i < k_candidates.size(); i++) {
                    auto const& k_candidate = k_candidates[i];
                    if ((fds[candidate] & k_candidate) == k_candidate) {
                        non_uniques.push_back(k_candidate);
                        is_futile[i] = true;
                    }
                }
                StoreHistogram(candidate);
            }

        }
    }

    // return time
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();

}

std::vector<boost::dynamic_bitset<>>
    algos::HCA::CandidateGeneration(std::vector<boost::dynamic_bitset<>> const& non_uniques,
                                    size_t k) {
    std::vector<boost::dynamic_bitset<>> candidates(0);
    for (size_t i = 0; i < non_uniques.size(); i++) {
        for (size_t j = i + 1; j < non_uniques.size(); j++) {
            auto const& nu1 = non_uniques[i];
            auto const& nu2 = non_uniques[j];
            // checking if first k-1 columns are same
            auto const& nu_xor = nu1 ^ nu2;
            auto const& nu_and = nu1 & nu2;
            if (nu_and.count() != k - 1)
                continue;
            int cnt = 0;
            for (int index = nu_xor.size(); index >= 0; index--) {
                if (nu_and[index])
                    break;
                if (nu_xor[index])
                    cnt++;
                if (cnt == 2)
                    break;
            }
            if ( cnt == 2 ) {
                boost::dynamic_bitset<> candidate(nu1 | nu2);

                if (!IsMinimal(candidate)) {
                    continue;
                }
                candidates.push_back(candidate);
            }
        }
    }
    return candidates;
}

bool algos::HCA::IsMinimal(boost::dynamic_bitset<> const& candidate) const {
    for (auto const& unique : uniques_) {
        if ((candidate & unique.GetColumnIndices()) == unique.GetColumnIndices()) {
            return false;
        }
    }
    return true;
}

// TODO
bool algos::HCA::IsUnique(boost::dynamic_bitset<> const& candidate) const {

}

// TODO
bool algos::HCA::IsPrunedByHistogram(boost::dynamic_bitset<> const& candidate) const {

}

void algos::HCA::StoreHistogram(boost::dynamic_bitset<> const& candidate) const {

}