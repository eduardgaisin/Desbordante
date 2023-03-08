//
// Created by eduard on 08.03.23.
//

#include "ucc_algorithm.h"
#include "algorithms/options/equal_nulls_opt.h"

namespace algos {

UCCAlgorithm::UCCAlgorithm(std::vector<std::string_view> phase_names)
    : Primitive(std::move(phase_names)) {
    RegisterOptions();
}

void UCCAlgorithm::RegisterOptions() {
    RegisterOption(config::EqualNullsOpt.GetOption(&is_null_equal_null_));
}

void UCCAlgorithm::FitInternal(model::IDatasetStream& data_stream) {
    number_of_columns_ = data_stream.GetNumberOfColumns();
    FitUCC(data_stream);
}

void UCCAlgorithm::FitUCC(model::IDatasetStream& data_stream) {
    relation_ = ColumnLayoutRelationData::CreateFrom(data_stream, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }
}

void UCCAlgorithm::Fit(std::shared_ptr<ColumnLayoutRelationData> data) {
    if (data->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: FD mining is meaningless.");
    }  // TODO: this has to be repeated for every "alternative" Fit
    number_of_columns_ = data->GetNumColumns();
    relation_ = std::move(data);
    ExecutePrepare();  // TODO: this has to be repeated for every "alternative" Fit
}

}  // namespace algos