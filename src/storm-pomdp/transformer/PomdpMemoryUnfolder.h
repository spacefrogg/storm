#pragma once

#include "storm/models/sparse/Pomdp.h"
#include "storm/models/sparse/StandardRewardModel.h"

namespace storm {
    namespace transformer {

        template<typename ValueType>
        class PomdpMemoryUnfolder {

        public:
            
            PomdpMemoryUnfolder(storm::models::sparse::Pomdp<ValueType> const& pomdp, uint64_t numMemoryStates);
            
            std::shared_ptr<storm::models::sparse::Pomdp<ValueType>> transform() const;

        private:
            storm::storage::SparseMatrix<ValueType> transformTransitions() const;
            storm::models::sparse::StateLabeling transformStateLabeling() const;
            std::vector<uint32_t> transformObservabilityClasses() const;
            storm::models::sparse::StandardRewardModel<ValueType> transformRewardModel(storm::models::sparse::StandardRewardModel<ValueType> const& rewardModel) const;
            
            uint64_t getUnfoldingState(uint64_t modelState, uint32_t memoryState) const;
            uint64_t getModelState(uint64_t unfoldingState) const;
            uint32_t getMemoryState(uint64_t unfoldingState) const;
            
            uint32_t getUnfoldingObersvation(uint32_t modelObservation, uint32_t memoryState) const;
            uint32_t getModelObersvation(uint32_t unfoldingObservation) const;
            uint32_t getMemoryStateFromObservation(uint32_t unfoldingObservation) const;
            
            
            storm::models::sparse::Pomdp<ValueType> const& pomdp;
            uint32_t numMemoryStates;
        };
    }
}