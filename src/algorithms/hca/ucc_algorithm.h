//
// Created by eduard on 08.03.23.
//

#include "primitive.h"
#include "vertical.h"
#include "column_layout_relation_data.h"

namespace algos {

using UCC = Vertical;

class UCCAlgorithm : public Primitive {

private:
    void ResetState() final {
        ucc_collection_.clear();
    }
    void RegisterOptions();

protected:
    size_t number_of_columns_;
    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::list<UCC> ucc_collection_;
    bool is_null_equal_null_;

    ColumnLayoutRelationData const& GetRelation() const noexcept {
        // GetRelation should be called after the dataset has been parsed, i.e. after algorithm
        // execution
        assert(relation_ != nullptr);
        return *relation_;
    }

    void FitInternal(model::IDatasetStream &data_stream) final;
    virtual void FitUCC(model::IDatasetStream &data_stream);

public:
    UCCAlgorithm(std::vector<std::string_view> phase_names);

    virtual ~UCCAlgorithm() = default;

    virtual void RegisterUCC(Vertical const& column_combination) {
        ucc_collection_.push_back(column_combination);
    }

    std::list<UCC> const& UccList() const noexcept {
        return ucc_collection_;
    }
    std::list<UCC>& UccList() noexcept {
        return ucc_collection_;
    }

    std::string GetJsonUCCs() const {
        return UCCsToJson(ucc_collection_);
    }

    template <typename Container>
    static std::string UCCsToJson(Container const& uccs) {
        std::string result = "{\"uccs\": [";
        std::vector<std::string> discovered_ucc_strings;
        for (UCC const& ucc : uccs) {
            discovered_ucc_strings.push_back("{ " + ucc.ToIndicesString() + " }");
        }
        std::sort(discovered_ucc_strings.begin(), discovered_ucc_strings.end());
        for (std::string const& ucc : discovered_ucc_strings) {
            result += ucc + ",";
        }
        if (result.back() == ',') {
            result.erase(result.size() - 1);
        }
        result += "]}";
        return result;
    }

    using Primitive::Fit;
    void Fit(std::shared_ptr<ColumnLayoutRelationData> data);
};

}  // namespace algos

