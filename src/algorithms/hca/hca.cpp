//
// Created by eduard on 06.03.23.
//

#include <iostream>

#include "hca.h"

algos::HCA::HCA()
    : UCCAlgorithm({"k-candidate validation"}) {

}

unsigned long long algos::HCA::ExecuteInternal() {
    // time
    auto start_time = std::chrono::system_clock::now();
    // get data rows, need to sort
    rows_data_ = std::vector<std::vector<int>>(GetRelation().GetNumRows(), std::vector<int>(GetRelation().GetNumColumns()));
    for (auto const& column_data : GetRelation().GetColumnData()) {
        for (size_t i = 0; i < column_data.GetProbingTable().size(); i++) {
            rows_data_[i][column_data.GetColumn()->GetIndex()]
                = column_data.GetProbingTableValue(i);
        }
    }

    // fds - stores fds with 1 column lhs and 1 column rhs
    // stores pairs of column indexes
    std::vector<std::pair<size_t, size_t>> fds(0);

    // HCA algorithm
    std::vector<boost::dynamic_bitset<>> non_uniques(0);
    // Single column
    for (size_t column_index = 0; column_index < GetRelation().GetNumColumns(); column_index++) {
        boost::dynamic_bitset<> candidate(GetRelation().GetNumColumns(), 0);
        candidate.set(column_index);
        size_t distinct = 0, freq = 0;
        if (IsUnique(candidate, distinct, freq)) {
            RegisterUCC(Vertical(GetRelation().GetSchema(), candidate));
        } else {
            non_uniques.push_back(candidate);
            // StoreFrequency
            freq_[candidate] = freq;
            // StoreDistinct
            distinct_[candidate] = distinct;
        }
    }
    // Non-single column combinations
    size_t max_k = non_uniques.size();
    for (size_t k = 2; k <= max_k; k++) {
        //  generating candidates with k columns
        auto k_candidates = CandidateGeneration(non_uniques, k - 1);
        non_uniques.clear();
        for (auto const& [candidate, is_futile] : k_candidates) {
            // checking, if candidate is covered by FD pruning
            if (is_futile) {
                continue;
            }
            if (IsPrunedByHistogram(candidate)) {
                non_uniques.push_back(candidate);
                continue;
            }

            size_t distinct_count = 0;
            if (IsUnique(candidate, distinct_count)) {
                RegisterUCC(Vertical(GetRelation().GetSchema(), candidate));
                // fds pruning
                // if {AXB} is UCC and Y -> X, then {AYB} is UCC too
                for (auto const& fd : fds) {
                    auto pruning_candidate = candidate;
                    pruning_candidate.set(fd.second, false);
                    pruning_candidate.set(fd.first, true);
                    if (pruning_candidate != candidate
                        && (k_candidates.count(pruning_candidate) != 0
                            && !k_candidates[pruning_candidate])) {
                        RegisterUCC(Vertical(GetRelation().GetSchema(),
                                                                        pruning_candidate));
                                k_candidates[pruning_candidate] = true;
                    }
                }
            } else {
                non_uniques.push_back(candidate);
                // fds pruning
                // if {AXB} is not UCC and X -> y, then {AYB} is not UCC too
                for (auto const& fd : fds) {
                    auto pruning_candidate = candidate;
                    pruning_candidate.set(fd.first, false);
                    pruning_candidate.set(fd.second, true);
                    if (pruning_candidate != candidate
                        && (k_candidates.count(pruning_candidate) != 0
                            && !k_candidates[pruning_candidate])) {
                                non_uniques.push_back(pruning_candidate);
                                k_candidates[pruning_candidate] = true;
                    }
                }
                distinct_[candidate] = distinct_count;
            }

        }

        if (k == 2) {
            // RetrieveFDs
            // If distinct count of X if equal to distinct count of {A, X}
            // then X -> A
            for (auto const& candidate : non_uniques) {
                // separating column indexes
                size_t f_index = candidate.find_first();
                size_t s_index = candidate.find_next(f_index);
                if (distinct_[boost::dynamic_bitset<>(1, 1) << f_index] == distinct_[candidate]) {
                    fds.push_back(std::make_pair(f_index, s_index));
                }
                if (distinct_[boost::dynamic_bitset<>(1, 1) << s_index] == distinct_[candidate]) {
                    fds.push_back(std::make_pair(s_index, f_index));
                }
            }
        }

    }

    // debug output
    std::cerr << GetJsonUCCs() << "\n";

    // return time
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

std::unordered_map<boost::dynamic_bitset<>, bool>
    algos::HCA::CandidateGeneration(std::vector<boost::dynamic_bitset<>> const& non_uniques,
                                    size_t k) {
    std::unordered_map<boost::dynamic_bitset<>, bool> candidates;
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
                candidates[candidate] = false;
            }
        }
    }
    return candidates;
}

bool algos::HCA::IsMinimal(boost::dynamic_bitset<> const& candidate) const {
    for (auto const& ucc : UccList()) {
        if ((candidate & ucc.GetColumnIndices()) == ucc.GetColumnIndices()) {
            return false;
        }
    }
    return true;
}

bool algos::HCA::IsUnique(boost::dynamic_bitset<> const& candidate,
                          size_t& distinct_count, size_t& frequency) {
    // Sorting table
    std::sort(rows_data_.begin(), rows_data_.end(),
              [&candidate] (std::vector<int> const& comp1, std::vector<int> const& comp2) {
                  for (size_t i = candidate.find_first();
                       i < candidate.size();
                       i = candidate.find_next(i)) {
                          if (comp1[i] < comp2[i]) {
                              return true;
                          } else if (comp1[i] > comp2[i]) {
                              return false;
                          }
                  }
                  return false;
              });
    // Finding distinct
    distinct_count = 1;
    frequency = 1;
    size_t current_freq = 1;
    for (size_t i = 1; i < rows_data_.size(); i++) {
        bool are_equal = true;
        for (size_t column_index = candidate.find_first();
             column_index < candidate.size();
             column_index = candidate.find_next(column_index)) {
            if (rows_data_[i - 1][column_index] == 0 ||
                    rows_data_[i][column_index] != rows_data_[i - 1][column_index]) {
                distinct_count++;
                current_freq = 1;
                are_equal = false;
                break;
            }
        }
        if (are_equal) {
            current_freq++;
        }
        frequency = std::max(frequency, current_freq);
    }
    return distinct_count == rows_data_.size();
}

bool algos::HCA::IsUnique(boost::dynamic_bitset<> const& candidate,
                          size_t& distinct_count) {
    // Sorting table
    std::sort(rows_data_.begin(), rows_data_.end(),
              [&candidate] (std::vector<int> const& comp1, std::vector<int> const& comp2) {
                  for (size_t i = candidate.find_first();
                       i < candidate.size();
                       i = candidate.find_next(i)) {
                      if (comp1[i] < comp2[i]) {
                          return true;
                      } else if (comp1[i] > comp2[i]) {
                          return false;
                      }
                  }
                  return false;
              });
    // Finding distinct
    distinct_count = 1;
    for (size_t i = 1; i < rows_data_.size(); i++) {
        for (size_t column_index = candidate.find_first();
             column_index < candidate.size();
             column_index = candidate.find_next(column_index)) {
            if (rows_data_[i - 1][column_index] == 0 ||
                    rows_data_[i][column_index] != rows_data_[i - 1][column_index]) {
                distinct_count++;
                break;
            }
        }
    }
    return distinct_count == rows_data_.size();
}

bool algos::HCA::IsPrunedByHistogram(boost::dynamic_bitset<> const& candidate) const {
    size_t cnt = 0;
    for (int i = candidate.size() - 1; i >= 0; i--) {
        if (candidate[i]) {
            boost::dynamic_bitset<> comb = candidate;
            comb.set(i, false);
            boost::dynamic_bitset<> single_column = candidate ^ comb;
            if (distinct_.count(comb) != 0 &&
                    distinct_.at(comb) < freq_.at(single_column)) {
                return true;
            }
            cnt++;
        }
        if (cnt == 2) {
            break;
        }
    }
    return false;
}
