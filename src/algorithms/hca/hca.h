//
// Created by eduard on 06.03.23.
//

#pragma once

#include <vector>
#include <list>

#include "primitive.h"
#include "vertical.h"
#include "relational_schema.h"
#include "pyro.h"

namespace algos {

class HCA : public Primitive {

private:
    std::unique_ptr<Pyro> pyro_fds_;

    RelationalSchema const* schema_;

    std::vector<boost::dynamic_bitset<>>
        CandidateGeneration(std::vector<boost::dynamic_bitset<>> const& non_uniques, size_t k);
    bool IsMinimal(boost::dynamic_bitset<> const& candidate) const;
    bool IsUnique(boost::dynamic_bitset<> const& candidate) const;
    bool IsFutile(boost::dynamic_bitset<> const& candidate) const;

    unsigned long long Execute();

protected:
    std::list<Vertical> uniques_;

public:
    HCA();

    virtual void RegisterUnique(Vertical const& column_combination) {
        uniques_.push_back(column_combination);
    }

};

}

