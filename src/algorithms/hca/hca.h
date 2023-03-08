//
// Created by eduard on 06.03.23.
//

#pragma once

#include <vector>
#include <unordered_map>
#include <list>
#include <memory>

#include "primitive.h"
#include "vertical.h"
#include "relational_schema.h"
#include "column_layout_relation_data.h"

namespace algos {

class HCA : public Primitive {

private:
    std::unordered_map<boost::dynamic_bitset<>, size_t> freq_;
    std::unordered_map<boost::dynamic_bitset<>, size_t> distinct_;

    RelationalSchema const* schema_;
    ColumnLayoutRelationData const* data_;
    std::vector<std::vector<int>> rows_data_;

    std::unordered_map<boost::dynamic_bitset<>, bool>
        CandidateGeneration(std::vector<boost::dynamic_bitset<>> const& non_uniques, size_t k);
    bool IsMinimal(boost::dynamic_bitset<> const& candidate) const;
    bool IsUnique(boost::dynamic_bitset<> const& candidate, size_t& distinct_count,
                  size_t& frequency);
    bool IsUnique(boost::dynamic_bitset<> const& candidate, size_t& distinct_count);
    bool IsPrunedByHistogram(boost::dynamic_bitset<> const& candidate) const;

    virtual unsigned long long ExecuteInternal() override;

protected:
    std::list<Vertical> uniques_;

public:
    HCA();

    virtual void RegisterUnique(Vertical const& column_combination) {
        uniques_.push_back(column_combination);
    }

};

}

