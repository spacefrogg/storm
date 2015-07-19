#include "src/modelchecker/reachability/SparseDtmcRegionModelChecker.h"

#include <chrono>

#include "src/adapters/CarlAdapter.h"

//#include "src/storage/StronglyConnectedComponentDecomposition.h"

#include "src/modelchecker/results/ExplicitQualitativeCheckResult.h"
#include "src/modelchecker/results/ExplicitQuantitativeCheckResult.h"

#include "src/utility/graph.h"
#include "src/utility/vector.h"
#include "src/utility/macros.h"

#include "modelchecker/prctl/SparseDtmcPrctlModelChecker.h"
#include "modelchecker/prctl/SparseMdpPrctlModelChecker.h"
//#include "modelchecker/reachability/SparseDtmcEliminationModelChecker.h"

#include "src/exceptions/InvalidPropertyException.h"
#include "src/exceptions/InvalidStateException.h"
#include "src/exceptions/UnexpectedException.h"


namespace storm {
    namespace modelchecker {
        
        
        template<typename ParametricType, typename ConstantType>
        SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::ParameterRegion(std::map<VariableType, CoefficientType> lowerBounds, std::map<VariableType, CoefficientType> upperBounds) : lowerBounds(lowerBounds), upperBounds(upperBounds), checkResult(RegionCheckResult::UNKNOWN) {
            //check whether both mappings map the same variables and precompute the set of variables
            for(auto const& variableWithBound : lowerBounds) {
                STORM_LOG_THROW((upperBounds.find(variableWithBound.first)!=upperBounds.end()), storm::exceptions::InvalidArgumentException, "Couldn't create region. No upper bound specified for Variable " << variableWithBound.first);
                this->variables.insert(variableWithBound.first);
            }
            for(auto const& variableWithBound : upperBounds){
                STORM_LOG_THROW((this->variables.find(variableWithBound.first)!=this->variables.end()), storm::exceptions::InvalidArgumentException, "Couldn't create region. No lower bound specified for Variable " << variableWithBound.first);
            }
            
        }

        template<typename ParametricType, typename ConstantType>        
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::setViolatedPoint(std::map<VariableType, CoefficientType> const& violatedPoint) {
            this->violatedPoint = violatedPoint;
        }
        
        template<typename ParametricType, typename ConstantType>
        std::map<typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::VariableType, typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getViolatedPoint() const {
            return violatedPoint;
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::setSatPoint(std::map<VariableType, CoefficientType> const& satPoint) {
            this->satPoint = satPoint;
        }

        template<typename ParametricType, typename ConstantType>
        std::map<typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::VariableType, typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getSatPoint() const {
            return satPoint;
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::setCheckResult(RegionCheckResult checkResult) {
            //a few sanity checks
            STORM_LOG_THROW((this->checkResult==RegionCheckResult::UNKNOWN || checkResult!=RegionCheckResult::UNKNOWN),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from something known to UNKNOWN ");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::EXISTSSAT || checkResult!=RegionCheckResult::EXISTSVIOLATED),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from EXISTSSAT to EXISTSVIOLATED");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::EXISTSSAT || checkResult!=RegionCheckResult::ALLVIOLATED),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from EXISTSSAT to ALLVIOLATED");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::EXISTSVIOLATED || checkResult!=RegionCheckResult::EXISTSSAT),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from EXISTSVIOLATED to EXISTSSAT");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::EXISTSVIOLATED || checkResult!=RegionCheckResult::ALLSAT),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from EXISTSVIOLATED to ALLSAT");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::EXISTSBOTH || checkResult!=RegionCheckResult::ALLSAT),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from EXISTSBOTH to ALLSAT");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::EXISTSBOTH || checkResult!=RegionCheckResult::ALLVIOLATED),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from EXISTSBOTH to ALLVIOLATED");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::ALLSAT || checkResult==RegionCheckResult::ALLSAT),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from ALLSAT to something else");
            STORM_LOG_THROW((this->checkResult!=RegionCheckResult::ALLVIOLATED || checkResult==RegionCheckResult::ALLVIOLATED),storm::exceptions::InvalidArgumentException, "Tried to change the check result of a region from ALLVIOLATED to something else");
            this->checkResult = checkResult;
        }

        template<typename ParametricType, typename ConstantType>        
        typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::RegionCheckResult SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getCheckResult() const {
            return checkResult;
        }

      
                
        template<typename ParametricType, typename ConstantType>
        std::set<typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::VariableType> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getVariables() const{
            return this->variables;
        }
        
        template<typename ParametricType, typename ConstantType>
        typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType const& SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getLowerBound(VariableType const& variable) const{
            auto const& result = lowerBounds.find(variable);
            STORM_LOG_THROW(result!=lowerBounds.end(), storm::exceptions::IllegalArgumentException, "tried to find a lower bound of a variable that is not specified by this region");
            return (*result).second;
        }
        
        template<typename ParametricType, typename ConstantType>
        typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType const& SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getUpperBound(VariableType const& variable) const{
            auto const& result = upperBounds.find(variable);
            STORM_LOG_THROW(result!=upperBounds.end(), storm::exceptions::IllegalArgumentException, "tried to find an upper bound of a variable that is not specified by this region");
            return (*result).second;
        }

        template<typename ParametricType, typename ConstantType>
        const std::map<typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::VariableType, typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getUpperBounds() const {
            return upperBounds;
        }
        
        template<typename ParametricType, typename ConstantType>
        const std::map<typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::VariableType, typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getLowerBounds() const {
            return lowerBounds;
        }
        
        template<typename ParametricType, typename ConstantType>
        std::vector<std::map<typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::VariableType, typename SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType>> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::getVerticesOfRegion(std::set<VariableType> const& consideredVariables) const{
            std::size_t const numOfVariables=consideredVariables.size();
            std::size_t const numOfVertices=std::pow(2,numOfVariables);
            std::vector<std::map<VariableType, CoefficientType>> resultingVector(numOfVertices,std::map<VariableType, CoefficientType>());
            if(numOfVertices==1){
                //no variables are given, the returned vector should still contain an empty map
                return resultingVector;
            }
            
            for(uint_fast64_t vertexId=0; vertexId<numOfVertices; ++vertexId){
                //interprete vertexId as a bit sequence
                //the consideredVariables.size() least significant bits of vertex will always represent the next vertex
                //(00...0 = lower bounds for all variables, 11...1 = upper bounds for all variables)
                std::size_t variableIndex=0;
                for(auto const& variable : consideredVariables){
                    if( (vertexId>>variableIndex)%2==0  ){
                        resultingVector[vertexId].insert(std::pair<VariableType, CoefficientType>(variable, getLowerBound(variable)));
                    }
                    else{
                        resultingVector[vertexId].insert(std::pair<VariableType, CoefficientType>(variable, getUpperBound(variable)));
                    }
                    ++variableIndex;
                }
            }
            return resultingVector;
        }

        template<typename ParametricType, typename ConstantType>
        std::string SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::checkResultToString() const {
            switch (this->checkResult) {
                case RegionCheckResult::UNKNOWN:
                    return "unknown";
                case RegionCheckResult::EXISTSSAT:
                    return "ExistsSat";
                case RegionCheckResult::EXISTSVIOLATED:
                    return "ExistsViolated";
                case RegionCheckResult::EXISTSBOTH:
                    return "ExistsBoth";
                case RegionCheckResult::ALLSAT:
                    return "allSat";
                case RegionCheckResult::ALLVIOLATED:
                    return "allViolated";
            }
            STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "Could not identify check result")
            return "ERROR";
        }
            
        template<typename ParametricType, typename ConstantType>
        std::string SparseDtmcRegionModelChecker<ParametricType, ConstantType>::ParameterRegion::toString() const {
            std::stringstream regionstringstream;
            for(auto var : this->getVariables()){
                regionstringstream << storm::utility::regions::convertNumber<SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType,double>(this->getLowerBound(var));
                regionstringstream << "<=";
                regionstringstream << storm::utility::regions::getVariableName(var);
                regionstringstream << "<=";
                regionstringstream << storm::utility::regions::convertNumber<SparseDtmcRegionModelChecker<ParametricType, ConstantType>::CoefficientType,double>(this->getUpperBound(var));
                regionstringstream << ",";
            }
            std::string regionstring = regionstringstream.str();
            //the last comma should actually be a semicolon
            regionstring = regionstring.substr(0,regionstring.length()-1) + ";";
            return regionstring;
        }
                
        
        
        template<typename ParametricType, typename ConstantType>
        SparseDtmcRegionModelChecker<ParametricType, ConstantType>::SparseDtmcRegionModelChecker(storm::models::sparse::Dtmc<ParametricType> const& model) : model(model), eliminationModelChecker(model), smtSolver(nullptr), probabilityOperatorFormula(nullptr), sampleDtmc(nullptr), approxMdp(nullptr) {
            // Intentionally left empty.
        }
        
        template<typename ParametricType, typename ConstantType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::canHandle(storm::logic::Formula const& formula) const {
             //for simplicity we only support state formulas with eventually (e.g. P<0.5 [ F "target" ])
            if(!formula.isStateFormula()){
                STORM_LOG_DEBUG("Region Model Checker could not handle formula " << formula << " Reason: expected a stateFormula");
                return false;
            }
            if(!formula.asStateFormula().isProbabilityOperatorFormula()){
                STORM_LOG_DEBUG("Region Model Checker could not handle formula " << formula << " Reason: expected a probabilityOperatorFormula");
                return false;
            }
            storm::logic::ProbabilityOperatorFormula const& probOpForm=formula.asStateFormula().asProbabilityOperatorFormula();
            if(!probOpForm.hasBound()){
                STORM_LOG_DEBUG("Region Model Checker could not handle formula " << formula << " Reason: The formula has no bound");
                return false;
            }
            if(!probOpForm.getSubformula().asPathFormula().isEventuallyFormula()){
                STORM_LOG_DEBUG("Region Model Checker could not handle formula " << formula << " Reason: expected an eventually subformula");
                return false;
            }
            if(model.getInitialStates().getNumberOfSetBits() != 1){
                STORM_LOG_DEBUG("Input model is required to have exactly one initial state.");
                return false;
            }
            return true;
        }
            
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::specifyFormula(storm::logic::Formula const& formula) {
            std::chrono::high_resolution_clock::time_point timePreprocessingStart = std::chrono::high_resolution_clock::now();
            STORM_LOG_THROW(this->canHandle(formula), storm::exceptions::IllegalArgumentException, "Tried to specify a formula that can not be handled.");
            
            //Get subformula, initial state, target states
            //Note: canHandle already ensures that the formula has the right shape and that the model has a single initial state.
            this->probabilityOperatorFormula= std::unique_ptr<storm::logic::ProbabilityOperatorFormula>(new storm::logic::ProbabilityOperatorFormula(formula.asStateFormula().asProbabilityOperatorFormula()));
            storm::logic::EventuallyFormula const& eventuallyFormula = this->probabilityOperatorFormula->getSubformula().asPathFormula().asEventuallyFormula();
            std::unique_ptr<CheckResult> targetStatesResultPtr = this->eliminationModelChecker.check(eventuallyFormula.getSubformula());
            storm::storage::BitVector const& targetStates = targetStatesResultPtr->asExplicitQualitativeCheckResult().getTruthValuesVector();

            // Then, compute the subset of states that has a probability of 0 or 1, respectively.
            std::pair<storm::storage::BitVector, storm::storage::BitVector> statesWithProbability01 = storm::utility::graph::performProb01(model, storm::storage::BitVector(model.getNumberOfStates(),true), targetStates);
            storm::storage::BitVector statesWithProbability0 = statesWithProbability01.first;
            storm::storage::BitVector statesWithProbability1 = statesWithProbability01.second;
            storm::storage::BitVector maybeStates = ~(statesWithProbability0 | statesWithProbability1);
            
            // If the initial state is known to have either probability 0 or 1, we can directly set the reachProbFunction.
            if (model.getInitialStates().isDisjointFrom(maybeStates)) {
                STORM_LOG_DEBUG("The probability of the initial state is constant (0 or 1)");
                this->reachProbFunction = statesWithProbability0.get(*model.getInitialStates().begin()) ? storm::utility::zero<ParametricType>() : storm::utility::one<ParametricType>();
            }
            
            // Determine the set of states that is reachable from the initial state without jumping over a target state.
            storm::storage::BitVector reachableStates = storm::utility::graph::getReachableStates(model.getTransitionMatrix(), model.getInitialStates(), maybeStates, statesWithProbability1);
            // Subtract from the maybe states the set of states that is not reachable (on a path from the initial to a target state).
            maybeStates &= reachableStates;
            // Create a vector for the probabilities to go to a state with probability 1 in one step.
            this->oneStepProbabilities = model.getTransitionMatrix().getConstrainedRowSumVector(maybeStates, statesWithProbability1);
            // Determine the initial state of the sub-model.
            //storm::storage::BitVector newInitialStates = model.getInitialStates() % maybeStates;
            this->initialState = *(model.getInitialStates() % maybeStates).begin();
            // We then build the submatrix that only has the transitions of the maybe states.
            storm::storage::SparseMatrix<ParametricType> submatrix = model.getTransitionMatrix().getSubmatrix(false, maybeStates, maybeStates);
            storm::storage::SparseMatrix<ParametricType> submatrixTransposed = submatrix.transpose();
            
            // Then, we convert the reduced matrix to a more flexible format to be able to perform state elimination more easily.
            this->flexibleTransitions = this->eliminationModelChecker.getFlexibleSparseMatrix(submatrix);
            this->flexibleBackwardTransitions = this->eliminationModelChecker.getFlexibleSparseMatrix(submatrixTransposed, true);
            
            // Create a bit vector that represents the current subsystem, i.e., states that we have not eliminated.
            this->subsystem = storm::storage::BitVector(submatrix.getRowCount(), true);
            
            std::chrono::high_resolution_clock::time_point timeInitialStateEliminationStart = std::chrono::high_resolution_clock::now();
            // eliminate all states with only constant outgoing transitions
            //TODO: maybe also states with constant incoming tranistions. THEN the ordering of the eliminated states does matter.
            eliminateStatesConstSucc(this->subsystem, this->flexibleTransitions, this->flexibleBackwardTransitions, this->oneStepProbabilities, this->hasOnlyLinearFunctions, this->initialState);
            STORM_LOG_DEBUG("Eliminated " << subsystem.size() - subsystem.getNumberOfSetBits() << " of " << subsystem.size() << " states that had constant outgoing transitions." << std::endl);
            std::cout << "Eliminated " << subsystem.size() - subsystem.getNumberOfSetBits() << " of " << subsystem.size() << " states that had constant outgoing transitions." << std::endl;
            
            
            //eliminate the remaining states to get the reachability probability function
            this->sparseTransitions = flexibleTransitions.getSparseMatrix();
            this->sparseBackwardTransitions = this->sparseTransitions.transpose();
            std::chrono::high_resolution_clock::time_point timeComputeReachProbFunctionStart = std::chrono::high_resolution_clock::now();
            this->reachProbFunction = computeReachProbFunction(this->subsystem, this->flexibleTransitions, this->flexibleBackwardTransitions, this->sparseTransitions, this->sparseBackwardTransitions, this->oneStepProbabilities, this->initialState);
            std::chrono::high_resolution_clock::time_point timeComputeReachProbFunctionEnd = std::chrono::high_resolution_clock::now();
            
            //std::string funcStr = " (/ " +
              //                  this->reachProbFunction.nominator().toString(false, true) + " " +
               //                 this->reachProbFunction.denominator().toString(false, true) +
                //            " )";
           // std::cout << std::endl <<"the resulting reach prob function is " << std::endl << funcStr << std::endl << std::endl;

            std::chrono::high_resolution_clock::time_point timeInitialStateEliminationEnd = std::chrono::high_resolution_clock::now();
            
            initializeSampleDtmcAndApproxMdp(this->sampleDtmc,this->sampleDtmcMapping,this->approxMdp,this->approxMdpMapping,this->approxMdpSubstitutions,this->subsystem, this->sparseTransitions,this->oneStepProbabilities, this->initialState);
            initializeSMTSolver(this->smtSolver, this->reachProbFunction,*this->probabilityOperatorFormula);
            
            //some information for statistics...
            std::chrono::high_resolution_clock::time_point timePreprocessingEnd = std::chrono::high_resolution_clock::now();
            this->timePreprocessing= timePreprocessingEnd - timePreprocessingStart;
            this->timeInitialStateElimination = timeInitialStateEliminationEnd-timeInitialStateEliminationStart;
            this->timeComputeReachProbFunction = timeComputeReachProbFunctionEnd-timeComputeReachProbFunctionStart;
            this->numOfCheckedRegions=0;
            this->numOfRegionsSolvedThroughSampling=0;
            this->numOfRegionsSolvedThroughApproximation=0;
            this->numOfRegionsSolvedThroughSubsystemSmt=0;
            this->numOfRegionsSolvedThroughFullSmt=0;
            this->numOfRegionsExistsBoth=0;
            this->numOfRegionsAllSat=0;
            this->numOfRegionsAllViolated=0;
            this->timeCheckRegion=std::chrono::high_resolution_clock::duration::zero();
            this->timeSampling=std::chrono::high_resolution_clock::duration::zero();
            this->timeApproximation=std::chrono::high_resolution_clock::duration::zero();
            this->timeMDPBuild=std::chrono::high_resolution_clock::duration::zero();
            this->timeSubsystemSmt=std::chrono::high_resolution_clock::duration::zero();
            this->timeFullSmt=std::chrono::high_resolution_clock::duration::zero();
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::eliminateStatesConstSucc(
                storm::storage::BitVector& subsys, 
                FlexibleMatrix& flexTransitions,
                FlexibleMatrix& flexBackwardTransitions,
                std::vector<ParametricType>& oneStepProbs,
                bool& allFunctionsAreLinear,
                storm::storage::sparse::state_type const& initialState) {
            
            //temporarily unselect the initial state to skip it.
            subsys.set(initialState, false);
            
            allFunctionsAreLinear=true;
            
            boost::optional<std::vector<ParametricType>> missingStateRewards;
            for (auto const& state : subsys) {
                bool onlyConstantOutgoingTransitions=true;
                for(auto& entry : flexTransitions.getRow(state)){
                    if(!this->parametricTypeComparator.isConstant(entry.getValue())){
                        onlyConstantOutgoingTransitions=false;
                        allFunctionsAreLinear &= storm::utility::regions::functionIsLinear<ParametricType>(entry.getValue());
                    }
                }
                if(!this->parametricTypeComparator.isConstant(oneStepProbs[state])){
                    onlyConstantOutgoingTransitions=false;
                    allFunctionsAreLinear &= storm::utility::regions::functionIsLinear<ParametricType>(oneStepProbs[state]);
                }
                if(onlyConstantOutgoingTransitions){
                    this->eliminationModelChecker.eliminateState(flexTransitions, oneStepProbs, state, flexBackwardTransitions, missingStateRewards);
                    subsys.set(state,false);
                }
            }
            subsys.set(initialState, true);
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::initializeSampleDtmcAndApproxMdp(
                    std::shared_ptr<storm::models::sparse::Dtmc<ConstantType>>& sampleDtmc,
                    std::vector<std::pair<ParametricType, storm::storage::MatrixEntry<storm::storage::sparse::state_type,ConstantType>&>> & sampleDtmcMapping,
                    std::shared_ptr<storm::models::sparse::Mdp<ConstantType>> & approxMdp,
                    std::vector<std::tuple<ParametricType, storm::storage::MatrixEntry<storm::storage::sparse::state_type,ConstantType>&, std::size_t>> & approxMdpMapping,
                    std::vector<std::map<VariableType, TypeOfBound>> & approxMdpSubstitutions,
                    storm::storage::BitVector const& subsys,
                    storm::storage::SparseMatrix<ParametricType> const& transitions,
                    std::vector<ParametricType> const& oneStepProbs,
                    storm::storage::sparse::state_type const& initState) {
            
            //the matrix "transitions" might have empty rows where states have been eliminated.
            //The DTMC/ MDP matrices should not have such rows. We therefore leave them out, but we have to change the indices of the states accordingly.
            std::vector<storm::storage::sparse::state_type> newStateIndexMap(transitions.getRowCount(), transitions.getRowCount()); //initialize with some illegal index to easily check if a transition leads to an unselected state
            storm::storage::sparse::state_type newStateIndex=0;
            for(auto const& state : subsys){
                newStateIndexMap[state]=newStateIndex;
                ++newStateIndex;
            }
            //We need to add a target state to which the oneStepProbabilities will lead as well as a sink state to which the "missing" probability will lead
            storm::storage::sparse::state_type numStates=newStateIndex+2;
            storm::storage::sparse::state_type targetState=numStates-2;
            storm::storage::sparse::state_type sinkState= numStates-1;
            
            //We can now fill in the dummy data of the transition matrices for dtmc and mdp
            std::vector<ParametricType> oneStepToSink(oneStepProbs.size(), storm::utility::zero<ParametricType>()); // -2 because not needed for target and sink state
            storm::storage::SparseMatrixBuilder<ConstantType> dtmcMatrixBuilder(numStates, numStates);
            storm::storage::SparseMatrixBuilder<ConstantType> mdpMatrixBuilder(0, numStates, 0, true, true, numStates);
            std::vector<std::map<VariableType, TypeOfBound> > mdpRowSubstitutions;
            storm::storage::sparse::state_type currentMdpRow=0;
            //go through rows:
            for(storm::storage::sparse::state_type oldStateIndex : subsys){ 
                ParametricType valueToSinkState=storm::utility::regions::getNewFunction<ParametricType, CoefficientType>(storm::utility::one<CoefficientType>());
                // the dtmc and the mdp rows need to sum up to one, therefore the first entry that we add has value one.
                ConstantType dummyEntry=storm::utility::one<ConstantType>(); 
                // store the columns and values that we have added because we need the same information for the next rows in the mdp
                std::vector<std::pair<storm::storage::sparse::state_type, ConstantType>> rowInformation;
                mdpMatrixBuilder.newRowGroup(currentMdpRow);
                std::set<VariableType> occurringParameters;
                //go through columns:
                for(auto const& entry: transitions.getRow(oldStateIndex)){ 
                    valueToSinkState-=entry.getValue();
                    storm::utility::regions::gatherOccurringVariables(entry.getValue(), occurringParameters);
                    dtmcMatrixBuilder.addNextValue(newStateIndexMap[oldStateIndex],newStateIndexMap[entry.getColumn()],dummyEntry);
                    mdpMatrixBuilder.addNextValue(currentMdpRow, newStateIndexMap[entry.getColumn()], dummyEntry);
                    rowInformation.emplace_back(std::make_pair(newStateIndexMap[entry.getColumn()], dummyEntry));
                    dummyEntry=storm::utility::zero<ConstantType>(); 
                    STORM_LOG_THROW(newStateIndexMap[entry.getColumn()]!=transitions.getRowCount(), storm::exceptions::UnexpectedException, "There is a transition to a state that should have been eliminated.");
                }
                //transition to target state
                if(!this->parametricTypeComparator.isZero(oneStepProbs[oldStateIndex])){ 
                    valueToSinkState-=oneStepProbs[oldStateIndex];
                    storm::utility::regions::gatherOccurringVariables(oneStepProbs[oldStateIndex], occurringParameters);
                    
                    dtmcMatrixBuilder.addNextValue(newStateIndexMap[oldStateIndex], targetState, dummyEntry);
                    mdpMatrixBuilder.addNextValue(currentMdpRow, targetState, dummyEntry);
                    rowInformation.emplace_back(std::make_pair(targetState, dummyEntry));
                    dummyEntry=storm::utility::zero<ConstantType>(); 
                }
                //transition to sink state
                if(!this->parametricTypeComparator.isZero(valueToSinkState)){ 
                    dtmcMatrixBuilder.addNextValue(newStateIndexMap[oldStateIndex], sinkState, dummyEntry);
                    mdpMatrixBuilder.addNextValue(currentMdpRow, sinkState, dummyEntry);
                    rowInformation.emplace_back(std::make_pair(sinkState, dummyEntry));
                    oneStepToSink[oldStateIndex]=valueToSinkState;
                }
                // Find the different combinations of occuring variables and lower/upper bounds (mathematically speaking we want to have the set 'occuringVariables times {u,l}' )
                std::size_t numOfSubstitutions=std::pow(2,occurringParameters.size()); //1 substitution = 1 combination of lower and upper bounds
                for(uint_fast64_t substitutionId=0; substitutionId<numOfSubstitutions; ++substitutionId){
                    //interprete substitutionId as a bit sequence
                    //the occurringParameters.size() least significant bits of substitutionId will always represent the next substitution that we have to add
                    //(00...0 = lower bounds for all parameters, 11...1 = upper bounds for all parameters)
                    mdpRowSubstitutions.emplace_back();
                    std::size_t parameterIndex=0;
                    for(auto const& parameter : occurringParameters){
                        if( (substitutionId>>parameterIndex)%2==0  ){
                            mdpRowSubstitutions.back().insert(std::make_pair(parameter, TypeOfBound::LOWER));
                        }
                        else{
                            mdpRowSubstitutions.back().insert(std::make_pair(parameter, TypeOfBound::UPPER));
                        }
                        ++parameterIndex;
                    }
                }
                //We already added one row in the MDP. Now add the remaining 2^(occuringParameters.size())-1 rows
                for(++currentMdpRow; currentMdpRow<mdpRowSubstitutions.size(); ++currentMdpRow){
                    for(auto const& entryOfThisRow : rowInformation){
                        mdpMatrixBuilder.addNextValue(currentMdpRow, entryOfThisRow.first, entryOfThisRow.second);
                    }
                }
            }
            //add self loops on the additional states (i.e., target and sink)
            dtmcMatrixBuilder.addNextValue(targetState, targetState, storm::utility::one<ConstantType>());
            dtmcMatrixBuilder.addNextValue(sinkState, sinkState, storm::utility::one<ConstantType>());
            mdpMatrixBuilder.newRowGroup(currentMdpRow);
            mdpMatrixBuilder.addNextValue(currentMdpRow, targetState, storm::utility::one<ConstantType>());
            ++currentMdpRow;
            mdpMatrixBuilder.newRowGroup(currentMdpRow);
            mdpMatrixBuilder.addNextValue(currentMdpRow, sinkState, storm::utility::one<ConstantType>());
            ++currentMdpRow;
            //The DTMC labeling
            storm::models::sparse::StateLabeling dtmcStateLabeling(numStates);
            storm::storage::BitVector initLabel(numStates, false);
            initLabel.set(newStateIndexMap[this->initialState], true);
            dtmcStateLabeling.addLabel("init", std::move(initLabel));
            storm::storage::BitVector targetLabel(numStates, false);
            targetLabel.set(numStates-2, true);
            dtmcStateLabeling.addLabel("target", std::move(targetLabel));
            storm::storage::BitVector sinkLabel(numStates, false);
            sinkLabel.set(numStates-1, true);
            dtmcStateLabeling.addLabel("sink", std::move(sinkLabel));
            // The MDP labeling (is the same)
            storm::models::sparse::StateLabeling mdpStateLabeling(dtmcStateLabeling);
            // other ingredients
            boost::optional<std::vector<ConstantType>> dtmcNoStateRewards, mdpNoStateRewards;
            boost::optional<storm::storage::SparseMatrix<ConstantType>> dtmcNoTransitionRewards, mdpNoTransitionRewards;  
            boost::optional<std::vector<boost::container::flat_set<uint_fast64_t>>> dtmcNoChoiceLabeling, mdpNoChoiceLabeling;            
            // the final dtmc and mdp
            sampleDtmc=std::make_shared<storm::models::sparse::Dtmc<ConstantType>>(dtmcMatrixBuilder.build(), std::move(dtmcStateLabeling), std::move(dtmcNoStateRewards), std::move(dtmcNoTransitionRewards), std::move(dtmcNoChoiceLabeling));
            approxMdp=std::make_shared<storm::models::sparse::Mdp<ConstantType>>(mdpMatrixBuilder.build(), std::move(mdpStateLabeling), std::move(mdpNoStateRewards), std::move(mdpNoTransitionRewards), std::move(mdpNoChoiceLabeling));
            
            //build mapping vectors and the substitution vector. 
            sampleDtmcMapping=std::vector<std::pair<ParametricType, storm::storage::MatrixEntry<storm::storage::sparse::state_type,ConstantType>&>>();
            sampleDtmcMapping.reserve(sampleDtmc->getTransitionMatrix().getEntryCount());
            approxMdpMapping=std::vector<std::tuple<ParametricType, storm::storage::MatrixEntry<storm::storage::sparse::state_type,ConstantType>&, std::size_t>>();
            approxMdpMapping.reserve(approxMdp->getTransitionMatrix().getEntryCount());
            approxMdpSubstitutions = std::vector<std::map<VariableType, TypeOfBound> >();
            
            currentMdpRow=0;
            //go through rows:
            for(storm::storage::sparse::state_type oldStateIndex : subsys){
                //Get the index of the substitution for the current mdp row. (add it to approxMdpSubstitutions if we see this substitution the first time)
                std::size_t substitutionIndex = std::find(approxMdpSubstitutions.begin(),approxMdpSubstitutions.end(), mdpRowSubstitutions[currentMdpRow]) - approxMdpSubstitutions.begin();
                if(substitutionIndex==approxMdpSubstitutions.size()){
                    approxMdpSubstitutions.push_back(mdpRowSubstitutions[currentMdpRow]);
                }
                typename storm::storage::SparseMatrix<ConstantType>::iterator dtmcEntryIt=sampleDtmc->getTransitionMatrix().getRow(newStateIndexMap[oldStateIndex]).begin();
                typename storm::storage::SparseMatrix<ConstantType>::iterator mdpEntryIt=approxMdp->getTransitionMatrix().getRow(currentMdpRow).begin();
                //go through columns to fill the dtmc and the first mdp row (of this group):
                for(auto const& entry: transitions.getRow(oldStateIndex)){
                    STORM_LOG_THROW(newStateIndexMap[entry.getColumn()]==dtmcEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entries of the DTMC matrix and the pDtmc Transitionmatrix do not match");
                    STORM_LOG_THROW(newStateIndexMap[entry.getColumn()]==mdpEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entries of the MDP matrix and the pDtmc Transitionmatrix do not match (" << newStateIndexMap[entry.getColumn()] << " vs " << mdpEntryIt->getColumn() << ")");
                    sampleDtmcMapping.emplace_back(entry.getValue(),*dtmcEntryIt);
                    approxMdpMapping.emplace_back(entry.getValue(),*mdpEntryIt, substitutionIndex);
                    ++dtmcEntryIt;
                    ++mdpEntryIt;
                }
                //transition to target state
                if(!this->parametricTypeComparator.isZero(oneStepProbs[oldStateIndex])){ 
                    STORM_LOG_THROW(targetState==dtmcEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entry of the DTMC matrix should match the target state but it does not.");
                    STORM_LOG_THROW(targetState==mdpEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entry of the MDP matrix should match the target state but it does not.");
                    sampleDtmcMapping.emplace_back(oneStepProbs[oldStateIndex],*dtmcEntryIt);
                    approxMdpMapping.emplace_back(oneStepProbs[oldStateIndex], *mdpEntryIt, substitutionIndex);
                    ++dtmcEntryIt;
                    ++mdpEntryIt;
                }
                //transition to sink state
                if(!this->parametricTypeComparator.isZero(oneStepToSink[oldStateIndex])){ 
                    STORM_LOG_THROW(sinkState==dtmcEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entry of the DTMC matrix should match the sink state but it does not.");
                    STORM_LOG_THROW(sinkState==mdpEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entry of the MDP matrix should match the sink state but it does not.");
                    sampleDtmcMapping.emplace_back(oneStepToSink[oldStateIndex],*dtmcEntryIt);
                    approxMdpMapping.emplace_back(oneStepToSink[oldStateIndex], *mdpEntryIt, substitutionIndex);
                    ++dtmcEntryIt;
                    ++mdpEntryIt;
                }
                //fill in the remaining rows of the mdp
                storm::storage::sparse::state_type startOfNextRowGroup = currentMdpRow + approxMdp->getTransitionMatrix().getRowGroupSize(newStateIndexMap[oldStateIndex]);
                for(++currentMdpRow;currentMdpRow<startOfNextRowGroup; ++currentMdpRow){
                    //Get the new substitution index
                    substitutionIndex = std::find(approxMdpSubstitutions.begin(),approxMdpSubstitutions.end(), mdpRowSubstitutions[currentMdpRow]) - approxMdpSubstitutions.begin();
                    if(substitutionIndex==approxMdpSubstitutions.size()){
                        approxMdpSubstitutions.push_back(mdpRowSubstitutions[currentMdpRow]);
                    }
                    //go through columns
                    mdpEntryIt=approxMdp->getTransitionMatrix().getRow(currentMdpRow).begin();
                    for(auto const& entry: transitions.getRow(oldStateIndex)){
                        STORM_LOG_THROW(newStateIndexMap[entry.getColumn()]==mdpEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entries of the MDP matrix and the pDtmc Transitionmatrix do not match");
                        approxMdpMapping.emplace_back(entry.getValue(),*mdpEntryIt, substitutionIndex);
                        ++mdpEntryIt;
                    }
                    //transition to target state
                    if(!this->parametricTypeComparator.isZero(oneStepProbs[oldStateIndex])){ 
                        STORM_LOG_THROW(targetState==mdpEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entry of the MDP matrix should match the target state but it does not.");
                        approxMdpMapping.emplace_back(oneStepProbs[oldStateIndex], *mdpEntryIt, substitutionIndex);
                        ++mdpEntryIt;
                    }
                    //transition to sink state
                    if(!this->parametricTypeComparator.isZero(oneStepToSink[oldStateIndex])){ 
                        STORM_LOG_THROW(sinkState==mdpEntryIt->getColumn(), storm::exceptions::UnexpectedException, "The entry of the MDP matrix should match the sink state but it does not.");
                        approxMdpMapping.emplace_back(oneStepToSink[oldStateIndex], *mdpEntryIt, substitutionIndex);
                        ++mdpEntryIt;
                    }
                }
            }
        }

        
        template<typename ParametricType, typename ConstantType>
        ParametricType SparseDtmcRegionModelChecker<ParametricType, ConstantType>::computeReachProbFunction(
                storm::storage::BitVector const& subsys,
                FlexibleMatrix const& flexTransitions,
                FlexibleMatrix const& flexBackwardTransitions,
                storm::storage::SparseMatrix<ParametricType> const& spTransitions,
                storm::storage::SparseMatrix<ParametricType> const& spBackwardTransitions,
                std::vector<ParametricType> const& oneStepProbs,
                storm::storage::sparse::state_type const& initState){
            
            //Note: this function is very similar to eliminationModelChecker.computeReachabilityValue
            
            //get working copies of the flexible transitions and oneStepProbabilities with which we will actually work (eliminate states etc).
            FlexibleMatrix workingCopyFlexTrans(flexTransitions);
            FlexibleMatrix workingCopyFlexBackwTrans(flexBackwardTransitions);
            std::vector<ParametricType> workingCopyOneStepProbs(oneStepProbs);
            
            storm::storage::BitVector initialStates(subsys.size(),false);
            initialStates.set(initState, true);
            std::vector<std::size_t> statePriorities = this->eliminationModelChecker.getStatePriorities(spTransitions, spBackwardTransitions, initialStates, workingCopyOneStepProbs);
            boost::optional<std::vector<ParametricType>> missingStateRewards;
            
            // Remove the initial state from the states which we need to eliminate.
            storm::storage::BitVector statesToEliminate(subsys);
            statesToEliminate.set(initState, false);
            
            //pure state elimination or hybrid method?
            if (storm::settings::sparseDtmcEliminationModelCheckerSettings().getEliminationMethod() == storm::settings::modules::SparseDtmcEliminationModelCheckerSettings::EliminationMethod::State) {
                std::vector<storm::storage::sparse::state_type> states(statesToEliminate.begin(), statesToEliminate.end());
                std::sort(states.begin(), states.end(), [&statePriorities] (storm::storage::sparse::state_type const& a, storm::storage::sparse::state_type const& b) { return statePriorities[a] < statePriorities[b]; });
                
                STORM_LOG_DEBUG("Eliminating " << states.size() << " states using the state elimination technique." << std::endl);
                for (auto const& state : states) {
                    this->eliminationModelChecker.eliminateState(workingCopyFlexTrans, workingCopyOneStepProbs, state, workingCopyFlexBackwTrans, missingStateRewards);
                }
            }
            else if (storm::settings::sparseDtmcEliminationModelCheckerSettings().getEliminationMethod() == storm::settings::modules::SparseDtmcEliminationModelCheckerSettings::EliminationMethod::Hybrid) {
                uint_fast64_t maximalDepth = 0;
                std::vector<storm::storage::sparse::state_type> entryStateQueue;
                STORM_LOG_DEBUG("Eliminating " << statesToEliminate.size() << " states using the hybrid elimination technique." << std::endl);
                maximalDepth = eliminationModelChecker.treatScc(workingCopyFlexTrans, workingCopyOneStepProbs, initialStates, statesToEliminate, spTransitions, workingCopyFlexBackwTrans, false, 0, storm::settings::sparseDtmcEliminationModelCheckerSettings().getMaximalSccSize(), entryStateQueue, missingStateRewards, statePriorities);
                
                // If the entry states were to be eliminated last, we need to do so now.
                STORM_LOG_DEBUG("Eliminating " << entryStateQueue.size() << " entry states as a last step.");
                if (storm::settings::sparseDtmcEliminationModelCheckerSettings().isEliminateEntryStatesLastSet()) {
                    for (auto const& state : entryStateQueue) {
                        eliminationModelChecker.eliminateState(workingCopyFlexTrans, workingCopyOneStepProbs, state, workingCopyFlexBackwTrans, missingStateRewards);
                    }
                }
            }
            else {
                STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "The selected state elimination Method has not been implemented.");
            }
            //finally, eliminate the initial state
            this->eliminationModelChecker.eliminateState(workingCopyFlexTrans, workingCopyOneStepProbs, initState, workingCopyFlexBackwTrans, missingStateRewards);
            
            return workingCopyOneStepProbs[initState];
        }

        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::initializeSMTSolver(std::shared_ptr<storm::solver::Smt2SmtSolver>& solver, const ParametricType& reachProbFunction, const storm::logic::ProbabilityOperatorFormula& formula) {
                    
            storm::expressions::ExpressionManager manager; //this manager will do nothing as we will use carl expressions..
            solver = std::shared_ptr<storm::solver::Smt2SmtSolver>(new storm::solver::Smt2SmtSolver(manager, true));
            
            ParametricType bound= storm::utility::regions::convertNumber<double, ParametricType>(this->probabilityOperatorFormula->getBound());
            
            // To prove that the property is satisfied in the initial state for all parameters,
            // we ask the solver whether the negation of the property is satisfiable and invert the answer.
            // In this case, assert that this variable is true:
            VariableType proveAllSatVar=storm::utility::regions::getNewVariable<VariableType>("proveAllSat", storm::utility::regions::VariableSort::VS_BOOL);
            
            //Example:
            //Property:    P<=p [ F 'target' ] holds iff...
            // f(x)         <= p
            // Hence: If  f(x) > p is unsat, the property is satisfied for all parameters.
            
            storm::logic::ComparisonType proveAllSatRel; //the relation from the property needs to be inverted
            switch (this->probabilityOperatorFormula->getComparisonType()) {
                case storm::logic::ComparisonType::Greater:
                    proveAllSatRel=storm::logic::ComparisonType::LessEqual;
                    break;
                case storm::logic::ComparisonType::GreaterEqual:
                    proveAllSatRel=storm::logic::ComparisonType::Less;
                    break;
                case storm::logic::ComparisonType::Less:
                    proveAllSatRel=storm::logic::ComparisonType::GreaterEqual;
                    break;
                case storm::logic::ComparisonType::LessEqual:
                    proveAllSatRel=storm::logic::ComparisonType::Greater;
                    break;
                default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "the comparison relation of the formula is not supported");
            }
            storm::utility::regions::addGuardedConstraintToSmtSolver(solver, proveAllSatVar, this->reachProbFunction, proveAllSatRel, bound);
            
            // To prove that the property is violated in the initial state for all parameters,
            // we ask the solver whether the the property is satisfiable and invert the answer.
            // In this case, assert that this variable is true:
            VariableType proveAllViolatedVar=storm::utility::regions::getNewVariable<VariableType>("proveAllViolated", storm::utility::regions::VariableSort::VS_BOOL);        
            
            //Example:
            //Property:    P<=p [ F 'target' ] holds iff...
            // f(x)         <= p
            // Hence: If f(x)  <= p is unsat, the property is violated for all parameters. 
            storm::logic::ComparisonType proveAllViolatedRel = this->probabilityOperatorFormula->getComparisonType();
            storm::utility::regions::addGuardedConstraintToSmtSolver(solver, proveAllViolatedVar, this->reachProbFunction, proveAllViolatedRel, bound);          
        }



        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkRegion(ParameterRegion& region) {
            std::chrono::high_resolution_clock::time_point timeCheckRegionStart = std::chrono::high_resolution_clock::now();
            ++this->numOfCheckedRegions;
            
            STORM_LOG_THROW(this->probabilityOperatorFormula!=nullptr, storm::exceptions::InvalidStateException, "Tried to analyze a region although no property has been specified" );
            STORM_LOG_DEBUG("Analyzing the region " << region.toString());
           // std::cout << "Analyzing the region " << region.toString() << std::endl;
            
            //switches for the different steps.
            bool doApproximation=this->hasOnlyLinearFunctions; // this approach is only correct if the model has only linear functions
            bool doSampling=true;
            bool doSubsystemSmt=false; //this->hasOnlyLinearFunctions;  // this approach is only correct if the model has only linear functions
            bool doFullSmt=true;
            
            std::chrono::high_resolution_clock::time_point timeApproximationStart = std::chrono::high_resolution_clock::now();
            std::vector<ConstantType> lowerBounds;
            std::vector<ConstantType> upperBounds;
            if(doApproximation){
                STORM_LOG_DEBUG("Checking approximative probabilities...");
          //      std::cout << "  Checking approximative probabilities..." << std::endl;
                if(checkApproximativeProbabilities(region, lowerBounds, upperBounds)){
                    ++this->numOfRegionsSolvedThroughApproximation;
                    STORM_LOG_DEBUG("Result '" << region.checkResultToString() <<"' obtained through approximation.");
                    doSampling=false;
                    doSubsystemSmt=false;
                    doFullSmt=false;
                }
            }
            std::chrono::high_resolution_clock::time_point timeApproximationEnd = std::chrono::high_resolution_clock::now();
            
            std::chrono::high_resolution_clock::time_point timeSamplingStart = std::chrono::high_resolution_clock::now();
            if(doSampling){
                STORM_LOG_DEBUG("Checking sample points...");
        //        std::cout << "  Checking sample points..." << std::endl;
                if(checkSamplePoints(region)){
                    ++this->numOfRegionsSolvedThroughSampling;
                    STORM_LOG_DEBUG("Result '" << region.checkResultToString() <<"' obtained through sampling.");
                    doApproximation=false;
                    doSubsystemSmt=false;
                    doFullSmt=false;
                }
            }
            std::chrono::high_resolution_clock::time_point timeSamplingEnd = std::chrono::high_resolution_clock::now();
            
            std::chrono::high_resolution_clock::time_point timeSubsystemSmtStart = std::chrono::high_resolution_clock::now();
            if(doSubsystemSmt){
                STORM_LOG_WARN("SubsystemSmt approach not yet implemented");
            }
            std::chrono::high_resolution_clock::time_point timeSubsystemSmtEnd = std::chrono::high_resolution_clock::now();
            
            std::chrono::high_resolution_clock::time_point timeFullSmtStart = std::chrono::high_resolution_clock::now();
            if(doFullSmt){
                STORM_LOG_DEBUG("Checking with Smt Solving...");
     //           std::cout << "  Checking with Smt Solving..." << std::endl;
                if(checkFullSmt(region)){
                    ++this->numOfRegionsSolvedThroughFullSmt;
                    STORM_LOG_DEBUG("Result '" << region.checkResultToString() <<"' obtained through Smt Solving.");
                    doApproximation=false;
                    doSampling=false;
                    doSubsystemSmt=false;
                }
            }
            std::chrono::high_resolution_clock::time_point timeFullSmtEnd = std::chrono::high_resolution_clock::now();
            
            
            //some information for statistics...
            std::chrono::high_resolution_clock::time_point timeCheckRegionEnd = std::chrono::high_resolution_clock::now();
            this->timeCheckRegion += timeCheckRegionEnd-timeCheckRegionStart;
            this->timeSampling += timeSamplingEnd - timeSamplingStart;
            this->timeApproximation += timeApproximationEnd - timeApproximationStart;
            this->timeSubsystemSmt += timeSubsystemSmtEnd - timeSubsystemSmtStart;
            this->timeFullSmt += timeFullSmtEnd - timeFullSmtStart;
            switch(region.getCheckResult()){
                case RegionCheckResult::EXISTSBOTH:
                    ++this->numOfRegionsExistsBoth;
                    break;
                case RegionCheckResult::ALLSAT:
                    ++this->numOfRegionsAllSat;
                    break;
                case RegionCheckResult::ALLVIOLATED:
                    ++this->numOfRegionsAllViolated;
                    break;
                default:
                    break;
            }
            
        }

        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkRegions(std::vector<ParameterRegion>& regions) {
            STORM_LOG_DEBUG("Checking " << regions.size() << "regions.");
            std::cout << "Checking " << regions.size() << "regions. Progress: ";
            std::cout.flush();
                    
            uint_fast64_t progress=0;
            uint_fast64_t checkedRegions=0;
            for(auto& region : regions){
                this->checkRegion(region);
                if((checkedRegions++)*10/regions.size()==progress){
                    std::cout << progress++;
                    std::cout.flush();
                }
            }
            std::cout << std::endl;
        }
        
        template<typename ParametricType, typename ConstantType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkSamplePoints(ParameterRegion& region) {
            
            auto samplingPoints = region.getVerticesOfRegion(region.getVariables()); //test the 4 corner points
            for (auto const& point : samplingPoints){
                if(checkPoint(region, point, false)){
                    return true;
                }            
            }
            return false;
        }
        
        template<typename ParametricType, typename ConstantType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkPoint(ParameterRegion& region, std::map<VariableType, CoefficientType>const& point, bool viaReachProbFunction) {
            // check whether the property is satisfied or not at the given point
            bool valueInBoundOfFormula;
            if(viaReachProbFunction){
                valueInBoundOfFormula = this->valueIsInBoundOfFormula(storm::utility::regions::evaluateFunction<ParametricType, ConstantType>(this->reachProbFunction, point));
            }
            else{
                //put the values into the dtmc matrix
                for( std::pair<ParametricType, typename storm::storage::MatrixEntry<storm::storage::sparse::state_type, ConstantType>&>& mappingPair : this->sampleDtmcMapping){
                    mappingPair.second.setValue(storm::utility::regions::convertNumber<CoefficientType,ConstantType>(
                            storm::utility::regions::evaluateFunction<ParametricType,ConstantType>(mappingPair.first, point)
                            )
                        );
                }
                std::shared_ptr<storm::logic::Formula> targetFormulaPtr(new storm::logic::AtomicLabelFormula("target"));
                storm::logic::EventuallyFormula eventuallyFormula(targetFormulaPtr);
                storm::modelchecker::SparseDtmcPrctlModelChecker<ConstantType> modelChecker(*this->sampleDtmc);
                std::unique_ptr<storm::modelchecker::CheckResult> resultPtr = modelChecker.computeEventuallyProbabilities(eventuallyFormula,false);
                valueInBoundOfFormula=this->valueIsInBoundOfFormula(resultPtr->asExplicitQuantitativeCheckResult<ConstantType>().getValueVector()[*this->sampleDtmc->getInitialStates().begin()]);

        //Delete from here
           //     ConstantType result=resultPtr->asExplicitQuantitativeCheckResult<ConstantType>().getValueVector()[*this->sampleDtmc->getInitialStates().begin()];
            //    ConstantType otherresult=storm::utility::regions::convertNumber<CoefficientType, ConstantType>(storm::utility::regions::evaluateFunction<ParametricType, ConstantType>(this->reachProbFunction, point));
                
            //    STORM_LOG_THROW((std::abs(result - otherresult) <= 0.01),storm::exceptions::UnexpectedException, "The results of new DTMC algorithm does not match: " << result << " vs. " << otherresult);
        //To here
                
            }
                
            if(valueInBoundOfFormula){
                if (region.getCheckResult()!=RegionCheckResult::EXISTSSAT){
                    region.setSatPoint(point);
                    if(region.getCheckResult()==RegionCheckResult::EXISTSVIOLATED){
                        region.setCheckResult(RegionCheckResult::EXISTSBOTH);
                        return true;
                    }
                    region.setCheckResult(RegionCheckResult::EXISTSSAT);
                }
            }
            else{
                if (region.getCheckResult()!=RegionCheckResult::EXISTSVIOLATED){
                    region.setViolatedPoint(point);
                    if(region.getCheckResult()==RegionCheckResult::EXISTSSAT){
                        region.setCheckResult(RegionCheckResult::EXISTSBOTH);
                        return true;
                    }
                    region.setCheckResult(RegionCheckResult::EXISTSVIOLATED);
                }
            }
            return false;
        }
        
        template<typename ParametricType, typename ConstantType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkApproximativeProbabilities(ParameterRegion& region, std::vector<ConstantType>& lowerBounds, std::vector<ConstantType>& upperBounds) {
            STORM_LOG_THROW(this->hasOnlyLinearFunctions, storm::exceptions::UnexpectedException, "Tried to generate bounds on the probability (only applicable if all functions are linear), but there are nonlinear functions.");
            //build the mdp and a reachability formula and create a modelchecker
            std::chrono::high_resolution_clock::time_point timeMDPBuildStart = std::chrono::high_resolution_clock::now();
            buildMdpForApproximation(region);
            std::chrono::high_resolution_clock::time_point timeMDPBuildEnd = std::chrono::high_resolution_clock::now();
            this->timeMDPBuild += timeMDPBuildEnd-timeMDPBuildStart;
            std::shared_ptr<storm::logic::Formula> targetFormulaPtr(new storm::logic::AtomicLabelFormula("target"));
            storm::logic::EventuallyFormula eventuallyFormula(targetFormulaPtr);
            storm::modelchecker::SparseMdpPrctlModelChecker<ConstantType> modelChecker(*this->approxMdp);
            
            
            
            //Decide whether we should compute lower or upper bounds first.
            //This does not matter if the current result is unknown. However, let us assume that it is more likely that the final result will be ALLSAT. So we test this first.
            storm::logic::OptimalityType firstOpType;
            switch (this->probabilityOperatorFormula->getComparisonType()) {
                case storm::logic::ComparisonType::Greater:
                case storm::logic::ComparisonType::GreaterEqual:
                    switch (region.getCheckResult()){
                        case RegionCheckResult::EXISTSSAT:
                        case RegionCheckResult::UNKNOWN: 
                            firstOpType=storm::logic::OptimalityType::Minimize;
                            break;
                        case RegionCheckResult::EXISTSVIOLATED:
                            firstOpType=storm::logic::OptimalityType::Maximize;
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "The checkresult of the current region should not be conclusive, i.e. it should be either EXISTSSAT or EXISTSVIOLATED or UNKNOWN");
                    }
                    break;
                case storm::logic::ComparisonType::Less:
                case storm::logic::ComparisonType::LessEqual:
                    switch (region.getCheckResult()){
                        case RegionCheckResult::EXISTSSAT:
                        case RegionCheckResult::UNKNOWN: 
                            firstOpType=storm::logic::OptimalityType::Maximize;
                            break;
                        case RegionCheckResult::EXISTSVIOLATED:
                            firstOpType=storm::logic::OptimalityType::Minimize;
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "The checkresult of the current region should not be conclusive, i.e. it should be either EXISTSSAT or EXISTSVIOLATED or UNKNOWN");
                    }
                    break;
                default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "the comparison relation of the formula is not supported");
            }
            
            //one iteration for each opType \in {Maximize, Minimize}
            storm::logic::OptimalityType opType=firstOpType;
            for(int i=1; i<=2; ++i){   
                //perform model checking on the mdp
                std::unique_ptr<storm::modelchecker::CheckResult> resultPtr = modelChecker.computeEventuallyProbabilities(eventuallyFormula,false,opType);
                
        //DELETE FROM HERE
          //  storm::models::sparse::Mdp<ConstantType> othermdp = buildMdpForApproximation2(region);
         //   storm::modelchecker::SparseMdpPrctlModelChecker<ConstantType> othermodelChecker(othermdp);
          //  std::unique_ptr<storm::modelchecker::CheckResult> otherresultPtr = othermodelChecker.computeEventuallyProbabilities(eventuallyFormula,false,opType);
         //   ConstantType origResult=resultPtr->asExplicitQuantitativeCheckResult<ConstantType>().getValueVector()[*this->approxMdp->getInitialStates().begin()];
        //    ConstantType otherResult=otherresultPtr->asExplicitQuantitativeCheckResult<ConstantType>().getValueVector()[*othermdp.getInitialStates().begin()];
        //    STORM_LOG_THROW(this->constantTypeComparator.isEqual(origResult,otherResult),storm::exceptions::UnexpectedException, "The results of new mdp algorithm does not match: " << origResult << " vs. " << otherResult);
      //TO HERE
            
                //check if the approximation yielded a conclusive result
                if(opType==storm::logic::OptimalityType::Maximize){
                    upperBounds = resultPtr->asExplicitQuantitativeCheckResult<ConstantType>().getValueVector();
                    if(valueIsInBoundOfFormula(upperBounds[*this->approxMdp->getInitialStates().begin()])){
                        if((this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::Less) || 
                                (this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::LessEqual)){
                            region.setCheckResult(RegionCheckResult::ALLSAT);
                            return true;
                        }
                    }
                    else{
                        if((this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::Greater) || 
                                (this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::GreaterEqual)){
                            region.setCheckResult(RegionCheckResult::ALLVIOLATED);
                            return true;
                        }
                    }
                    //flip the optype for the next iteration
                    opType=storm::logic::OptimalityType::Minimize;
           //         if(i==1) std::cout << "    Requiring a second model checker run (with Minimize)" << std::endl;
                }
                else if(opType==storm::logic::OptimalityType::Minimize){
                    lowerBounds = resultPtr->asExplicitQuantitativeCheckResult<ConstantType>().getValueVector();
                    if(valueIsInBoundOfFormula(lowerBounds[*this->approxMdp->getInitialStates().begin()])){
                        if((this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::Greater) || 
                                (this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::GreaterEqual)){
                            region.setCheckResult(RegionCheckResult::ALLSAT);
                            return true;
                        }
                    }
                    else{
                        if((this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::Less) || 
                                (this->probabilityOperatorFormula->getComparisonType()== storm::logic::ComparisonType::LessEqual)){
                            region.setCheckResult(RegionCheckResult::ALLVIOLATED);
                            return true;
                        }
                    }                
                    //flip the optype for the next iteration
                    opType=storm::logic::OptimalityType::Maximize;
        //            if(i==1) std::cout << "    Requiring a second model checker run (with Maximize)" << std::endl;
                }
            }
            
            //if we reach this point than the result is still inconclusive.
            return false;            
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::buildMdpForApproximation(const ParameterRegion& region) {
            //instantiate the substitutions for the given region
            std::vector<std::map<VariableType, CoefficientType>> substitutions(this->approxMdpSubstitutions.size());
            for(uint_fast64_t substitutionIndex=0; substitutionIndex<this->approxMdpSubstitutions.size(); ++substitutionIndex){
                for(std::pair<VariableType, TypeOfBound> const& sub : this->approxMdpSubstitutions[substitutionIndex]){
                    switch(sub.second){
                        case TypeOfBound::LOWER:
                            substitutions[substitutionIndex].insert(std::make_pair(sub.first, region.getLowerBound(sub.first)));
                            break;
                        case TypeOfBound::UPPER:
                            substitutions[substitutionIndex].insert(std::make_pair(sub.first, region.getUpperBound(sub.first)));
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "Unexpected Type of Bound");
                    }
                }
            }
            
            //now put the values into the mdp matrix
            for( std::tuple<ParametricType, typename storm::storage::MatrixEntry<storm::storage::sparse::state_type, ConstantType>&, size_t>& mappingTriple : this->approxMdpMapping){
                std::get<1>(mappingTriple).setValue(storm::utility::regions::convertNumber<CoefficientType,ConstantType>(
                                                storm::utility::regions::evaluateFunction<ParametricType,ConstantType>(std::get<0>(mappingTriple),substitutions[std::get<2>(mappingTriple)])
                                                )
                                            );
            }
            
        }
        
        //DELETEME
        template<typename ParametricType, typename ConstantType>
        storm::models::sparse::Mdp<ConstantType> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::buildMdpForApproximation2(const ParameterRegion& region) {
            //We are going to build a (non parametric) MDP which has an action for the lower bound and an action for the upper bound of every parameter 
            
            //The matrix (and the Choice labeling)
            
            //the matrix this->sparseTransitions might have empty rows where states have been eliminated.
            //The MDP matrix should not have such rows. We therefore leave them out, but we have to change the indices of the states accordingly.
            //These changes are computed in advance
            std::vector<storm::storage::sparse::state_type> newStateIndexMap(this->sparseTransitions.getRowCount(), this->sparseTransitions.getRowCount()); //initialize with some illegal index to easily check if a transition leads to an unselected state
            storm::storage::sparse::state_type newStateIndex=0;
            for(auto const& state : this->subsystem){
                newStateIndexMap[state]=newStateIndex;
                ++newStateIndex;
            }
            //We need to add a target state to which the oneStepProbabilities will lead as well as a sink state to which the "missing" probability will lead
            storm::storage::sparse::state_type numStates=newStateIndex+2;
            storm::storage::sparse::state_type targetState=numStates-2;
            storm::storage::sparse::state_type sinkState= numStates-1;
            storm::storage::SparseMatrixBuilder<ConstantType> matrixBuilder(0, numStates, 0, true, true, numStates);
            //std::vector<boost::container::flat_set<uint_fast64_t>> choiceLabeling;
            
            //fill in the data row by row            
            storm::storage::sparse::state_type currentRow=0;
            for(storm::storage::sparse::state_type oldStateIndex : this->subsystem){
                //we first go through the row to find out a) which parameter occur in that row and b) at which point do we have to insert the selfloop
                storm::storage::sparse::state_type selfloopIndex=0;
                std::set<VariableType> occurringParameters;
                for(auto const& entry: this->sparseTransitions.getRow(oldStateIndex)){
                    storm::utility::regions::gatherOccurringVariables(entry.getValue(), occurringParameters);
                    if(entry.getColumn()<=oldStateIndex){
                        if(entry.getColumn()==oldStateIndex){
                            //there already is a selfloop so we do not have to add one.
                            selfloopIndex=numStates; // --> selfloop will never be inserted
                        }
                        else {
                            ++selfloopIndex;
                        }
                    }
                    STORM_LOG_THROW(newStateIndexMap[entry.getColumn()]!=this->sparseTransitions.getRowCount(), storm::exceptions::UnexpectedException, "There is a transition to a state that should have been eliminated.");
                }
                storm::utility::regions::gatherOccurringVariables(this->oneStepProbabilities[oldStateIndex], occurringParameters);
                
                //we now add a row for every combination of lower and upper bound of the variables
                auto const& substitutions = region.getVerticesOfRegion(occurringParameters);
                STORM_LOG_ASSERT(!substitutions.empty(), "there are no substitutions, i.e., no vertices of the current region. this should not be possible");
                matrixBuilder.newRowGroup(currentRow);
                for(size_t substitutionIndex=0; substitutionIndex< substitutions.size(); ++substitutionIndex){
                    ConstantType missingProbability = storm::utility::one<ConstantType>();
                    if(selfloopIndex==0){ //add the selfloop first.
                        matrixBuilder.addNextValue(currentRow, newStateIndexMap[oldStateIndex], storm::utility::zero<ConstantType>());
                        selfloopIndex=numStates; // --> selfloop will never be inserted again
                    }
                    for(auto const& entry : this->sparseTransitions.getRow(oldStateIndex)){
                        ConstantType value = storm::utility::regions::convertNumber<CoefficientType,ConstantType>(
                                             storm::utility::regions::evaluateFunction<ParametricType,ConstantType>(entry.getValue(),substitutions[substitutionIndex])
                                             );
                        missingProbability-=value;
                        storm::storage::sparse::state_type column = newStateIndexMap[entry.getColumn()];
                        matrixBuilder.addNextValue(currentRow, column, value);
                        --selfloopIndex;
                        if(selfloopIndex==0){ //add the selfloop now
                            matrixBuilder.addNextValue(currentRow, newStateIndexMap[oldStateIndex], storm::utility::zero<ConstantType>());
                            selfloopIndex=numStates; // --> selfloop will never be inserted again
                        }
                    }
                    
                    if(!this->parametricTypeComparator.isZero(this->oneStepProbabilities[oldStateIndex])){ //transition to target state
                        ConstantType value = storm::utility::regions::convertNumber<CoefficientType,ConstantType>(
                                             storm::utility::regions::evaluateFunction<ParametricType,ConstantType>(this->oneStepProbabilities[oldStateIndex],substitutions[substitutionIndex])
                                             );
                        missingProbability-=value;
                        matrixBuilder.addNextValue(currentRow, targetState, value);
                    }
                    if(!this->constantTypeComparator.isZero(missingProbability)){ //transition to sink state
                        STORM_LOG_THROW((missingProbability> -storm::utility::regions::convertNumber<double, ConstantType>(storm::settings::generalSettings().getPrecision())), storm::exceptions::UnexpectedException, "The missing probability is negative.");
                        matrixBuilder.addNextValue(currentRow, sinkState, missingProbability);
                    }
                    //boost::container::flat_set<uint_fast64_t> label;
                    //label.insert(substitutionIndex);
                    //choiceLabeling.emplace_back(label);
                    ++currentRow;
                }
            }
            //add self loops on the additional states (i.e., target and sink)
            //boost::container::flat_set<uint_fast64_t> labelAll;
            //labelAll.insert(substitutions.size());
            matrixBuilder.newRowGroup(currentRow);
            matrixBuilder.addNextValue(currentRow, targetState, storm::utility::one<ConstantType>());
            //    choiceLabeling.emplace_back(labelAll);
            ++currentRow;
            matrixBuilder.newRowGroup(currentRow);
            matrixBuilder.addNextValue(currentRow, sinkState, storm::utility::one<ConstantType>());
            //    choiceLabeling.emplace_back(labelAll);
            ++currentRow;
            
            //The labeling
            
            storm::models::sparse::StateLabeling stateLabeling(numStates);
            storm::storage::BitVector initLabel(numStates, false);
            initLabel.set(newStateIndexMap[this->initialState], true);
            stateLabeling.addLabel("init", std::move(initLabel));
            storm::storage::BitVector targetLabel(numStates, false);
            targetLabel.set(numStates-2, true);
            stateLabeling.addLabel("target", std::move(targetLabel));
            storm::storage::BitVector sinkLabel(numStates, false);
            sinkLabel.set(numStates-1, true);
            stateLabeling.addLabel("sink", std::move(sinkLabel));

            // The MDP
            boost::optional<std::vector<ConstantType>> noStateRewards;
            boost::optional<storm::storage::SparseMatrix<ConstantType>> noTransitionRewards;  
            boost::optional<std::vector<boost::container::flat_set<uint_fast64_t>>> noChoiceLabeling;            
            
            return storm::models::sparse::Mdp<ConstantType>(matrixBuilder.build(), std::move(stateLabeling), std::move(noStateRewards), std::move(noTransitionRewards), std::move(noChoiceLabeling));
        }

        
        
        template<typename ParametricType, typename ConstantType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkFullSmt(ParameterRegion& region) {
            if (region.getCheckResult()==RegionCheckResult::UNKNOWN){
                //Sampling needs to be done (on a single point)
                checkPoint(region,region.getLowerBounds(), true);
            }
            
            this->smtSolver->push();
            
            //add constraints for the region
            for(auto const& variable : region.getVariables()) {
                storm::utility::regions::addParameterBoundsToSmtSolver(this->smtSolver, variable, storm::logic::ComparisonType::GreaterEqual, region.getLowerBound(variable));
                storm::utility::regions::addParameterBoundsToSmtSolver(this->smtSolver, variable, storm::logic::ComparisonType::LessEqual, region.getUpperBound(variable));
            }
            
            //add constraint that states what we want to prove            
            VariableType proveAllSatVar=storm::utility::regions::getVariableFromString<VariableType>("proveAllSat");     
            VariableType proveAllViolatedVar=storm::utility::regions::getVariableFromString<VariableType>("proveAllViolated");     
            switch(region.getCheckResult()){
                case RegionCheckResult::EXISTSBOTH:
                    STORM_LOG_WARN_COND((region.getCheckResult()!=RegionCheckResult::EXISTSBOTH), "checkFullSmt invoked although the result is already clear (EXISTSBOTH). Will validate this now...");
                case RegionCheckResult::ALLSAT:
                    STORM_LOG_WARN_COND((region.getCheckResult()!=RegionCheckResult::ALLSAT), "checkFullSmt invoked although the result is already clear (ALLSAT). Will validate this now...");
                case RegionCheckResult::EXISTSSAT:
                    storm::utility::regions::addBoolVariableToSmtSolver(this->smtSolver, proveAllSatVar, true);
                    storm::utility::regions::addBoolVariableToSmtSolver(this->smtSolver, proveAllViolatedVar, false);
                    break;
                case RegionCheckResult::ALLVIOLATED:
                    STORM_LOG_WARN_COND((region.getCheckResult()!=RegionCheckResult::ALLVIOLATED), "checkFullSmt invoked although the result is already clear (ALLVIOLATED). Will validate this now...");
                case RegionCheckResult::EXISTSVIOLATED:
                    storm::utility::regions::addBoolVariableToSmtSolver(this->smtSolver, proveAllSatVar, false);
                    storm::utility::regions::addBoolVariableToSmtSolver(this->smtSolver, proveAllViolatedVar, true);
                    break;
                default:
                STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "Could not handle the current region CheckResult: " << region.checkResultToString());
            }
            
            storm::solver::SmtSolver::CheckResult solverResult= this->smtSolver->check();
            this->smtSolver->pop();
            
            switch(solverResult){
                case storm::solver::SmtSolver::CheckResult::Sat:
                    switch(region.getCheckResult()){
                        case RegionCheckResult::EXISTSSAT:
                            region.setCheckResult(RegionCheckResult::EXISTSBOTH);
                            //There is also a violated point
                            STORM_LOG_WARN("Extracting a violated point from the smt solver is not yet implemented!");
                            break;
                        case RegionCheckResult::EXISTSVIOLATED:
                            region.setCheckResult(RegionCheckResult::EXISTSBOTH);
                            //There is also a sat point
                            STORM_LOG_WARN("Extracting a sat point from the smt solver is not yet implemented!");
                            break;
                        case RegionCheckResult::EXISTSBOTH:
                            //That was expected
                            STORM_LOG_WARN("result EXISTSBOTH Validated!");
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "The solver gave an unexpected result (sat)");
                    }
                    return true;
                case storm::solver::SmtSolver::CheckResult::Unsat:
                    switch(region.getCheckResult()){
                        case RegionCheckResult::EXISTSSAT:
                            region.setCheckResult(RegionCheckResult::ALLSAT);
                            break;
                        case RegionCheckResult::EXISTSVIOLATED:
                            region.setCheckResult(RegionCheckResult::ALLVIOLATED);
                            break;
                        case RegionCheckResult::ALLSAT:
                            //That was expected...
                            STORM_LOG_WARN("result ALLSAT Validated!");
                            break;
                        case RegionCheckResult::ALLVIOLATED:
                            //That was expected...
                            STORM_LOG_WARN("result ALLVIOLATED Validated!");
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "The solver gave an unexpected result (unsat)");
                    }
                    return true;
                case storm::solver::SmtSolver::CheckResult::Unknown:
                default:
                    STORM_LOG_WARN("The SMT solver was not able to compute a result for this region. (Timeout? Memout?)");
                    if(this->smtSolver->isNeedsRestart()){
                        initializeSMTSolver(this->smtSolver,this->reachProbFunction, *this->probabilityOperatorFormula);
                    }
                    return false;
            }
        }

             template<typename ParametricType, typename ConstantType>
        template<typename ValueType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::valueIsInBoundOfFormula(ValueType value){
            STORM_LOG_THROW(this->probabilityOperatorFormula!=nullptr, storm::exceptions::InvalidStateException, "Tried to compare a value to the bound of a formula, but no formula specified.");
            double valueAsDouble = storm::utility::regions::convertNumber<ValueType, double>(value);
        //    std::cout << "checked whether value: " << value << " (= " << valueAsDouble << " double ) is in bound of formula: " << *this->probabilityOperatorFormula << std::endl;
        //    std::cout << "     (valueAsDouble >= this->probabilityOperatorFormula->getBound()) is " << (valueAsDouble >= this->probabilityOperatorFormula->getBound()) << std::endl;
            switch (this->probabilityOperatorFormula->getComparisonType()) {
                case storm::logic::ComparisonType::Greater:
                    return (valueAsDouble > this->probabilityOperatorFormula->getBound());
                case storm::logic::ComparisonType::GreaterEqual:
                    return (valueAsDouble >= this->probabilityOperatorFormula->getBound());
                case storm::logic::ComparisonType::Less:
                    return (valueAsDouble < this->probabilityOperatorFormula->getBound());
                case storm::logic::ComparisonType::LessEqual:
                    return (valueAsDouble <= this->probabilityOperatorFormula->getBound());
                default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "the comparison relation of the formula is not supported");
            }
        }

        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::printStatisticsToStream(std::ostream& outstream) {
            
            if(this->probabilityOperatorFormula==nullptr){
                outstream << "Statistic Region Model Checker Statistics Error: No formula specified." << std::endl; 
                return;
            }
            
            std::chrono::milliseconds timePreprocessingInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timePreprocessing);
            std::chrono::milliseconds timeInitialStateEliminationInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeInitialStateElimination);
            std::chrono::milliseconds timeComputeReachProbFunctionInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeComputeReachProbFunction);
            std::chrono::milliseconds timeCheckRegionInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeCheckRegion);
            std::chrono::milliseconds timeSammplingInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeSampling);
            std::chrono::milliseconds timeApproximationInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeApproximation);
            std::chrono::milliseconds timeMDPBuildInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeMDPBuild);
            std::chrono::milliseconds timeFullSmtInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(this->timeFullSmt);
            
            std::chrono::high_resolution_clock::duration timeOverall = timePreprocessing + timeCheckRegion; // + ...
            std::chrono::milliseconds timeOverallInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeOverall);
            
            std::size_t subsystemTransitions = this->sparseTransitions.getNonzeroEntryCount();
            for(auto const& transition : this->oneStepProbabilities){
                if(!this->parametricTypeComparator.isZero(transition)){
                    ++subsystemTransitions;
                }
            }
            
            uint_fast64_t numOfSolvedRegions= this->numOfRegionsExistsBoth + this->numOfRegionsAllSat + this->numOfRegionsAllViolated;
            
            outstream << std::endl << "Statistics Region Model Checker Statistics:" << std::endl;
            outstream << "-----------------------------------------------" << std::endl;
            outstream << "Model: " << this->model.getNumberOfStates() << " states, " << this->model.getNumberOfTransitions() << " transitions." << std::endl;
            outstream << "Reduced model: " << this->subsystem.getNumberOfSetBits() << " states, " << subsystemTransitions << "transitions" << std::endl;
            outstream << "Formula: " << *this->probabilityOperatorFormula << std::endl;
            outstream << (this->hasOnlyLinearFunctions ? "A" : "Not a") << "ll occuring functions in the model are linear" << std::endl;
            outstream << "Number of checked regions: " << this->numOfCheckedRegions << std::endl;
            outstream << "  Number of solved regions:  " <<  numOfSolvedRegions << "(" << numOfSolvedRegions*100/this->numOfCheckedRegions << "%)" <<  std::endl;
            outstream << "    AllSat:      " <<  this->numOfRegionsAllSat << "(" << this->numOfRegionsAllSat*100/this->numOfCheckedRegions << "%)" <<  std::endl;
            outstream << "    AllViolated: " <<  this->numOfRegionsAllViolated << "(" << this->numOfRegionsAllViolated*100/this->numOfCheckedRegions << "%)" <<  std::endl;
            outstream << "    ExistsBoth:  " <<  this->numOfRegionsExistsBoth << "(" << this->numOfRegionsExistsBoth*100/this->numOfCheckedRegions << "%)" <<  std::endl;
            outstream << "    Unsolved:    " <<  this->numOfCheckedRegions - numOfSolvedRegions << "(" << (this->numOfCheckedRegions - numOfSolvedRegions)*100/this->numOfCheckedRegions << "%)" <<  std::endl;
            outstream << "  --  " <<  std::endl;
            outstream << "  " << this->numOfRegionsSolvedThroughSampling << " regions solved through Sampling" << std::endl;
            outstream << "  " << this->numOfRegionsSolvedThroughApproximation << " regions solved through Approximation" << std::endl;
            outstream << "  " << this->numOfRegionsSolvedThroughSubsystemSmt << " regions solved through SubsystemSmt" << std::endl;
            outstream << "  " << this->numOfRegionsSolvedThroughFullSmt << " regions solved through FullSmt" << std::endl;
            outstream << std::endl;
            outstream << "Running times:" << std::endl;
            outstream << "  " << timeOverallInMilliseconds.count() << "ms overall" << std::endl;
            outstream << "  " << timePreprocessingInMilliseconds.count() << "ms Preprocessing including... " << std::endl;
            outstream << "    " << timeInitialStateEliminationInMilliseconds.count() << "ms Initial state elimination including..." << std::endl;
            outstream << "      " << timeComputeReachProbFunctionInMilliseconds.count() << "ms to compute the reachability probability function" << std::endl;
            outstream << "  " << timeCheckRegionInMilliseconds.count() << "ms Region Check including... " << std::endl;
            outstream << "    " << timeSammplingInMilliseconds.count() << "ms Sampling " << std::endl;
            outstream << "    " << timeApproximationInMilliseconds.count() << "ms Approximation including... " << std::endl;
            outstream << "      " << timeMDPBuildInMilliseconds.count() << "ms to build the MDP" << std::endl;
            outstream << "    " << timeFullSmtInMilliseconds.count() << "ms Full Smt solving" << std::endl;
            outstream << "-----------------------------------------------" << std::endl;
            
        }
   



#ifdef STORM_HAVE_CARL
        
        //DELETEME
        template<>
        std::pair<storm::storage::SparseMatrix<double>,std::vector<boost::container::flat_set<uint_fast64_t>>> SparseDtmcRegionModelChecker<storm::RationalFunction, double>::instantiateFlexibleMatrix(FlexibleMatrix const& matrix, std::vector<std::map<storm::Variable, storm::RationalFunction::CoeffType>> const& substitutions, storm::storage::BitVector const& filter, bool addSinkState, std::vector<storm::RationalFunction> const& oneStepProbabilities, bool addSelfLoops) const {
            
            //Check if the arguments are as expected
            STORM_LOG_THROW((std::is_same<storm::RationalFunction::CoeffType, cln::cl_RA>::value), storm::exceptions::IllegalArgumentException, "Unexpected Type of Coefficients");
            STORM_LOG_THROW(filter.size()==matrix.getNumberOfRows(), storm::exceptions::IllegalArgumentException, "Unexpected size of the filter");
            STORM_LOG_THROW(oneStepProbabilities.empty() || oneStepProbabilities.size()==matrix.getNumberOfRows(), storm::exceptions::IllegalArgumentException, "Unexpected size of the oneStepProbabilities");
            
            //get a mapping from old state indices to the new ones
            std::vector<storm::storage::sparse::state_type> newStateIndexMap(matrix.getNumberOfRows(), matrix.getNumberOfRows()); //initialize with some illegal index to easily check if a transition leads to an unselected state
            storm::storage::sparse::state_type newStateIndex=0;
            for(auto const& state : filter){
                newStateIndexMap[state]=newStateIndex;
                ++newStateIndex;
            }
            storm::storage::sparse::state_type numStates=filter.getNumberOfSetBits();
            STORM_LOG_ASSERT(newStateIndex==numStates, "unexpected number of new states");
            storm::storage::sparse::state_type targetState =0;
            storm::storage::sparse::state_type sinkState=0;
            if(!oneStepProbabilities.empty()){
                targetState=numStates;
                ++numStates;
            }
            if(addSinkState){
                sinkState=numStates;
                ++numStates;
            }
            //todo rows (i.e. the first parameter) should be numStates*substitutions.size ?
            storm::storage::SparseMatrixBuilder<double> matrixBuilder(0, numStates, 0, true, true, numStates);
            std::vector<boost::container::flat_set<uint_fast64_t>> choiceLabeling;
            //fill in the data row by row            
            storm::storage::sparse::state_type currentRow=0;
            for(auto const& oldStateIndex : filter){
                matrixBuilder.newRowGroup(currentRow);
                for(size_t substitutionIndex=0; substitutionIndex< substitutions.size(); ++substitutionIndex){
                    double missingProbability = 1.0;
                    if(matrix.getRow(oldStateIndex).empty()){ //just add the selfloop if there is no transition
                        if(addSelfLoops){
                            matrixBuilder.addNextValue(currentRow, newStateIndexMap[oldStateIndex], storm::utility::zero<double>());
                        }
                    }
                    else{
                        FlexibleMatrix::const_iterator entry = matrix.getRow(oldStateIndex).begin();
                        for(; entry<matrix.getRow(oldStateIndex).end() && entry->getColumn()<oldStateIndex; ++entry){ //insert until we come to the diagonal entry
                            double value = cln::double_approx(entry->getValue().evaluate(substitutions[substitutionIndex]));
                            missingProbability-=value;
                            storm::storage::sparse::state_type column = newStateIndexMap[entry->getColumn()];
                            STORM_LOG_THROW(column<numStates, storm::exceptions::IllegalArgumentException, "Illegal filter: Selected a state that has a transition to an unselected state.");
                            matrixBuilder.addNextValue(currentRow, column, value);
                        }
                        if(addSelfLoops && entry->getColumn()!=oldStateIndex){ //maybe add a zero valued diagonal entry
                            matrixBuilder.addNextValue(currentRow, newStateIndexMap[oldStateIndex], storm::utility::zero<double>());
                        }
                        for(; entry < matrix.getRow(oldStateIndex).end(); ++entry){ //insert the rest
                            double value = cln::double_approx(entry->getValue().evaluate(substitutions[substitutionIndex]));
                            missingProbability-=value;
                            storm::storage::sparse::state_type column = newStateIndexMap[entry->getColumn()];
                            STORM_LOG_THROW(column<numStates, storm::exceptions::IllegalArgumentException, "Illegal filter: Selected a state that has a transition to an unselected state.");
                            matrixBuilder.addNextValue(currentRow, column, value);
                        }
                    }
                    if(!oneStepProbabilities.empty() && !oneStepProbabilities[oldStateIndex].isZero()){ //transition to target state
                        double value = cln::double_approx(oneStepProbabilities[oldStateIndex].evaluate(substitutions[substitutionIndex]));
                        missingProbability-=value;
                        matrixBuilder.addNextValue(currentRow, targetState, value);
                    }
                    storm::utility::ConstantsComparator<double> doubleComperator;
                    if(addSinkState && !doubleComperator.isZero(missingProbability)){ //transition to sink state
                        STORM_LOG_ASSERT(missingProbability> -storm::settings::generalSettings().getPrecision(), "The missing probability is negative.");
                        matrixBuilder.addNextValue(currentRow, sinkState, missingProbability);
                    }
                    boost::container::flat_set<uint_fast64_t> label;
                    label.insert(substitutionIndex);
                    choiceLabeling.emplace_back(label);
                    ++currentRow;
                }
            }
            //finally, add self loops on the additional states (i.e., target and sink)
            boost::container::flat_set<uint_fast64_t> labelAll;
            labelAll.insert(substitutions.size());
            if (!oneStepProbabilities.empty()){
                matrixBuilder.newRowGroup(currentRow);
                matrixBuilder.addNextValue(currentRow, targetState, storm::utility::one<double>());
                choiceLabeling.emplace_back(labelAll);
                ++currentRow;
            }
            
            if (addSinkState){
                matrixBuilder.newRowGroup(currentRow);
                matrixBuilder.addNextValue(currentRow, sinkState, storm::utility::one<double>());
                choiceLabeling.emplace_back(labelAll);
                ++currentRow;
            }
       
            return std::pair<storm::storage::SparseMatrix<double>, std::vector<boost::container::flat_set<uint_fast64_t>>>(matrixBuilder.build(), std::move(choiceLabeling));
        }
        
        //DELETEME
        template<typename ParametricType, typename ConstantType>
        std::pair<storm::storage::SparseMatrix<double>,std::vector<boost::container::flat_set<uint_fast64_t>>> SparseDtmcRegionModelChecker<ParametricType, ConstantType>::instantiateFlexibleMatrix(FlexibleMatrix const& matrix, std::vector<std::map<storm::Variable, storm::RationalFunction::CoeffType>> const& substitutions, storm::storage::BitVector const& filter, bool addSinkState, std::vector<ParametricType> const& oneStepProbabilities, bool addSelfLoops) const{
            STORM_LOG_THROW(false, storm::exceptions::IllegalArgumentException, "Instantiation of flexible matrix is not supported for this type");
        }


      //DELETEME
        template<>
        void SparseDtmcRegionModelChecker<storm::RationalFunction, double>::eliminateStates(storm::storage::BitVector& subsystem, FlexibleMatrix& flexibleMatrix, std::vector<storm::RationalFunction>& oneStepProbabilities, FlexibleMatrix& flexibleBackwardTransitions, storm::storage::BitVector const& initialstates, storm::storage::SparseMatrix<storm::RationalFunction> const& forwardTransitions, boost::optional<std::vector<std::size_t>> const& statePriorities){
    
            if(true){ // eliminate all states with constant outgoing transitions
                storm::storage::BitVector statesToEliminate = ~initialstates;
                //todo: ordering of states important?
                boost::optional<std::vector<storm::RationalFunction>> missingStateRewards;
                for (auto const& state : statesToEliminate) {
                    bool onlyConstantOutgoingTransitions=true;
                    for(auto const& entry : flexibleMatrix.getRow(state)){
                        if(!entry.getValue().isConstant()){
                            onlyConstantOutgoingTransitions=false;
                            break;
                        }
                    }
                    if(onlyConstantOutgoingTransitions){
                        eliminationModelChecker.eliminateState(flexibleMatrix, oneStepProbabilities, state, flexibleBackwardTransitions, missingStateRewards);
                        subsystem.set(state,false);
                    }
                }

                //Note: we could also "eliminate" the initial state to get rid of its selfloop
            }
            else if(false){ //eliminate all states with standard state elimination
                boost::optional<std::vector<storm::RationalFunction>> missingStateRewards;
                
                storm::storage::BitVector statesToEliminate = ~initialstates;
                std::vector<storm::storage::sparse::state_type> states(statesToEliminate.begin(), statesToEliminate.end());
                
                if (statePriorities) {
                    std::sort(states.begin(), states.end(), [&statePriorities] (storm::storage::sparse::state_type const& a, storm::storage::sparse::state_type const& b) { return statePriorities.get()[a] < statePriorities.get()[b]; });
                }
                
                STORM_LOG_DEBUG("Eliminating " << states.size() << " states using the state elimination technique." << std::endl);
                for (auto const& state : states) {
                    eliminationModelChecker.eliminateState(flexibleMatrix, oneStepProbabilities, state, flexibleBackwardTransitions, missingStateRewards);
                }
                subsystem=~statesToEliminate;
                
            }
            else if(false){ //hybrid method
                boost::optional<std::vector<storm::RationalFunction>> missingStateRewards;
                storm::storage::BitVector statesToEliminate = ~initialstates;
                uint_fast64_t maximalDepth =0;
                std::vector<storm::storage::sparse::state_type> entryStateQueue;
                STORM_LOG_DEBUG("Eliminating " << statesToEliminate.size() << " states using the hybrid elimination technique." << std::endl);
                maximalDepth = eliminationModelChecker.treatScc(flexibleMatrix, oneStepProbabilities, initialstates, statesToEliminate, forwardTransitions, flexibleBackwardTransitions, false, 0, storm::settings::sparseDtmcEliminationModelCheckerSettings().getMaximalSccSize(), entryStateQueue, missingStateRewards, statePriorities);
                
                // If the entry states were to be eliminated last, we need to do so now.
                STORM_LOG_DEBUG("Eliminating " << entryStateQueue.size() << " entry states as a last step.");
                if (storm::settings::sparseDtmcEliminationModelCheckerSettings().isEliminateEntryStatesLastSet()) {
                    for (auto const& state : entryStateQueue) {
                        eliminationModelChecker.eliminateState(flexibleMatrix, oneStepProbabilities, state, flexibleBackwardTransitions, missingStateRewards);
                    }
                }
                subsystem=~statesToEliminate;     
            }
            std::cout << "Eliminated " << subsystem.size() - subsystem.getNumberOfSetBits() << " of " << subsystem.size() << "states." << std::endl;
            STORM_LOG_DEBUG("Eliminated " << subsystem.size() - subsystem.getNumberOfSetBits() << " states." << std::endl);
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::eliminateStates(storm::storage::BitVector& subsystem, FlexibleMatrix& flexibleMatrix, std::vector<ParametricType>& oneStepProbabilities, FlexibleMatrix& flexibleBackwardTransitions, storm::storage::BitVector const& initialstates, storm::storage::SparseMatrix<ParametricType> const& forwardTransitions, boost::optional<std::vector<std::size_t>> const& statePriorities){
            STORM_LOG_THROW(false, storm::exceptions::IllegalArgumentException, "elimination of states not suported for this type");
        }

        //OBSOLETE ... 
        template<>
        void SparseDtmcRegionModelChecker<storm::RationalFunction, double>::formulateModelWithSMT(storm::solver::Smt2SmtSolver& solver, std::vector<storm::RationalFunction::PolyType>& stateProbVars, storm::storage::BitVector const& subsystem, FlexibleMatrix const& flexibleMatrix, std::vector<storm::RationalFunction> const& oneStepProbabilities){
            carl::VariablePool& varPool = carl::VariablePool::getInstance();
            
            //first add a state variable for every state in the subsystem, providing that such a variable does not already exist.
            for (storm::storage::sparse::state_type state : subsystem){
                if(stateProbVars[state].isZero()){ //variable does not exist yet
                    storm::Variable stateVar = varPool.getFreshVariable("p_" + std::to_string(state));
                    std::shared_ptr<carl::Cache<carl::PolynomialFactorizationPair<storm::RawPolynomial>>> cache(new carl::Cache<carl::PolynomialFactorizationPair<storm::RawPolynomial>>());
                    storm::RationalFunction::PolyType stateVarAsPoly(storm::RationalFunction::PolyType::PolyType(stateVar), cache);

                    //each variable is in the interval [0,1]
                    solver.add(storm::RationalFunction(stateVarAsPoly), storm::CompareRelation::GEQ, storm::RationalFunction(0));
                    solver.add(storm::RationalFunction(stateVarAsPoly), storm::CompareRelation::LEQ, storm::RationalFunction(1));
                    stateProbVars[state] = stateVarAsPoly;
                }
            }
            
            //now lets add the actual transitions
            for (storm::storage::sparse::state_type state : subsystem){
                storm::RationalFunction reachProbability(oneStepProbabilities[state]);
                for(auto const& transition : flexibleMatrix.getRow(state)){
                    reachProbability += transition.getValue() * stateProbVars[transition.getColumn()];
                }
                //Todo: depending on the objective (i.e. the formlua) it suffices to use LEQ or GEQ here... maybe this is faster?
                solver.add(storm::RationalFunction(stateProbVars[state]), storm::CompareRelation::EQ, reachProbability);
            }
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::formulateModelWithSMT(storm::solver::Smt2SmtSolver& solver, std::vector<storm::RationalFunction::PolyType>& stateProbVars, storm::storage::BitVector const& subsystem, FlexibleMatrix const& flexibleMatrix, std::vector<storm::RationalFunction> const& oneStepProbabilities){
            STORM_LOG_THROW(false, storm::exceptions::IllegalArgumentException, "SMT formulation is not supported for this type");
        }
        
        //DELETEME
        template<>
        void SparseDtmcRegionModelChecker<storm::RationalFunction, double>::restrictProbabilityVariables(storm::solver::Smt2SmtSolver& solver, std::vector<storm::RationalFunction::PolyType> const& stateProbVars, storm::storage::BitVector const& subsystem, FlexibleMatrix const& flexibleMatrix, std::vector<storm::RationalFunction> const& oneStepProbabilities, ParameterRegion const& region, storm::logic::ComparisonType const& compType){
            //We are going to build a new (non parametric) MDP which has an action for the lower bound and an action for the upper bound of every parameter 
                        
            //todo invent something better to obtain the substitutions.
            //this only works as long as there is only one parameter per state,
            // also: check whether the terms are linear/monotone(?)
            
            STORM_LOG_WARN("the probability restriction only works on linear terms which is not checked");
            storm::storage::sparse::state_type const numOfStates=subsystem.getNumberOfSetBits() + 2; //subsystem + target state + sink state
            storm::models::sparse::StateLabeling stateLabeling(numOfStates);
            stateLabeling.addLabel("init", storm::storage::BitVector(numOfStates, true));
            storm::storage::BitVector targetLabel(numOfStates, false);
            targetLabel.set(numOfStates-2, true);
            stateLabeling.addLabel("target", std::move(targetLabel));
            storm::storage::BitVector sinkLabel(numOfStates, false);
            sinkLabel.set(numOfStates-1, true);
            stateLabeling.addLabel("sink", std::move(sinkLabel));

            std::pair<storm::storage::SparseMatrix<double>,std::vector<boost::container::flat_set<uint_fast64_t>>> instantiation = instantiateFlexibleMatrix(flexibleMatrix, region.getVerticesOfRegion(region.getVariables()), subsystem, true, oneStepProbabilities, true);
            boost::optional<std::vector<double>> noStateRewards;
            boost::optional<storm::storage::SparseMatrix<double>> noTransitionRewards;            
            storm::models::sparse::Mdp<double> mdp(instantiation.first, std::move(stateLabeling),noStateRewards,noTransitionRewards,instantiation.second);
                        
            //we need the correct optimalityType for model checking as well as the correct relation for smt solving
            storm::logic::OptimalityType opType;
            storm::CompareRelation boundRelation;
            switch (compType){
                case storm::logic::ComparisonType::Greater:
                    opType=storm::logic::OptimalityType::Minimize;
                    boundRelation=storm::CompareRelation::GEQ;
                    break;
                case storm::logic::ComparisonType::GreaterEqual:
                    opType=storm::logic::OptimalityType::Minimize;
                    boundRelation=storm::CompareRelation::GEQ;
                    break;
                case storm::logic::ComparisonType::Less:
                    opType=storm::logic::OptimalityType::Maximize;
                    boundRelation=storm::CompareRelation::LEQ;
                    break;
                case storm::logic::ComparisonType::LessEqual:
                    opType=storm::logic::OptimalityType::Maximize;
                    boundRelation=storm::CompareRelation::LEQ;
                    break;
                default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "the comparison relation of the formula is not supported");
            }
            
            //perform model checking on the mdp
            std::shared_ptr<storm::logic::Formula> targetFormulaPtr(new storm::logic::AtomicLabelFormula("target"));
            storm::logic::EventuallyFormula eventuallyFormula(targetFormulaPtr);
            storm::modelchecker::SparseMdpPrctlModelChecker<double> modelChecker(mdp);
            std::unique_ptr<CheckResult> resultPtr = modelChecker.computeEventuallyProbabilities(eventuallyFormula,false,opType);
            std::vector<double> resultVector = resultPtr->asExplicitQuantitativeCheckResult<double>().getValueVector();            
            
            //formulate constraints for the solver
            uint_fast64_t boundDenominator = 1.0/storm::settings::generalSettings().getPrecision(); //we need to approx. the obtained bounds as rational numbers
            storm::storage::sparse::state_type subsystemState=0; //the subsystem uses other state indices
            for(storm::storage::sparse::state_type state : subsystem){
                uint_fast64_t boundNumerator = resultVector[subsystemState]*boundDenominator;
                storm::RationalFunction bound(boundNumerator);
                bound = bound/boundDenominator;
                //Todo: non-exact values might be problematic here...
                solver.add(storm::RationalFunction(stateProbVars[state]), boundRelation, bound);
                ++subsystemState;
            }
        }
        
        template<typename ParametricType, typename ConstantType>
        void SparseDtmcRegionModelChecker<ParametricType, ConstantType>::restrictProbabilityVariables(storm::solver::Smt2SmtSolver& solver, std::vector<storm::RationalFunction::PolyType> const& stateProbVars, storm::storage::BitVector const& subsystem, FlexibleMatrix const& flexibleMatrix, std::vector<storm::RationalFunction> const& oneStepProbabilities, ParameterRegion const& region, storm::logic::ComparisonType const& compType){
            STORM_LOG_THROW(false, storm::exceptions::IllegalArgumentException, "restricting Probability Variables is not supported for this type");
        }
        
        //DELETEME
        template<>
        bool SparseDtmcRegionModelChecker<storm::RationalFunction, double>::checkRegionOld(storm::logic::Formula const& formula, std::vector<ParameterRegion> parameterRegions){
            //Note: this is an 'experimental' implementation
            
            std::chrono::high_resolution_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
                
            //Start with some preprocessing (inspired by computeUntilProbabilities...)
            //for simplicity we only support state formulas with eventually (e.g. P<0.5 [ F "target" ])
            //get the (sub)formulae and the vector of target states
            STORM_LOG_THROW(formula.isStateFormula(), storm::exceptions::IllegalArgumentException, "expected a stateFormula");
            STORM_LOG_THROW(formula.asStateFormula().isProbabilityOperatorFormula(), storm::exceptions::IllegalArgumentException, "expected a probabilityOperatorFormula");
            storm::logic::ProbabilityOperatorFormula const& probOpForm=formula.asStateFormula().asProbabilityOperatorFormula();
            STORM_LOG_THROW(probOpForm.hasBound(), storm::exceptions::IllegalArgumentException, "The formula has no bound");
            STORM_LOG_THROW(probOpForm.getSubformula().asPathFormula().isEventuallyFormula(), storm::exceptions::IllegalArgumentException, "expected an eventually subformula");
            storm::logic::EventuallyFormula const& eventuallyFormula = probOpForm.getSubformula().asPathFormula().asEventuallyFormula();
            std::unique_ptr<CheckResult> targetStatesResultPtr = eliminationModelChecker.check(eventuallyFormula.getSubformula());
            storm::storage::BitVector const& targetStates = targetStatesResultPtr->asExplicitQualitativeCheckResult().getTruthValuesVector();
            // Do some sanity checks to establish some required properties.
            STORM_LOG_THROW(model.getInitialStates().getNumberOfSetBits() == 1, storm::exceptions::IllegalArgumentException, "Input model is required to have exactly one initial state.");
            storm::storage::sparse::state_type initialState = *model.getInitialStates().begin();
            // Then, compute the subset of states that has a probability of 0 or 1, respectively.
            std::pair<storm::storage::BitVector, storm::storage::BitVector> statesWithProbability01 = storm::utility::graph::performProb01(model, storm::storage::BitVector(model.getNumberOfStates(),true), targetStates);
            storm::storage::BitVector statesWithProbability0 = statesWithProbability01.first;
            storm::storage::BitVector statesWithProbability1 = statesWithProbability01.second;
            storm::storage::BitVector maybeStates = ~(statesWithProbability0 | statesWithProbability1);
            // If the initial state is known to have either probability 0 or 1, we can directly return the result.
            if (model.getInitialStates().isDisjointFrom(maybeStates)) {
                STORM_LOG_DEBUG("The probability of all initial states was found in a preprocessing step.");
                double res= statesWithProbability0.get(*model.getInitialStates().begin()) ? 0.0 : 1.0;
                switch (probOpForm.getComparisonType()){
                    case storm::logic::ComparisonType::Greater:
                        return (res > probOpForm.getBound());
                    case storm::logic::ComparisonType::GreaterEqual:
                        return (res >= probOpForm.getBound());
                    case storm::logic::ComparisonType::Less:
                        return (res < probOpForm.getBound());
                    case storm::logic::ComparisonType::LessEqual:
                        return (res <= probOpForm.getBound());
                    default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "the comparison relation of the formula is not supported");
                }
            }
            // Determine the set of states that is reachable from the initial state without jumping over a target state.
            storm::storage::BitVector reachableStates = storm::utility::graph::getReachableStates(model.getTransitionMatrix(), model.getInitialStates(), maybeStates, statesWithProbability1);
            // Subtract from the maybe states the set of states that is not reachable (on a path from the initial to a target state).
            maybeStates &= reachableStates;
            // Create a vector for the probabilities to go to a state with probability 1 in one step.
            std::vector<storm::RationalFunction> oneStepProbabilities = model.getTransitionMatrix().getConstrainedRowSumVector(maybeStates, statesWithProbability1);
            // Determine the set of initial states of the sub-model.
            storm::storage::BitVector newInitialStates = model.getInitialStates() % maybeStates;
            // We then build the submatrix that only has the transitions of the maybe states.
            storm::storage::SparseMatrix<storm::RationalFunction> submatrix = model.getTransitionMatrix().getSubmatrix(false, maybeStates, maybeStates);
            storm::storage::SparseMatrix<storm::RationalFunction> submatrixTransposed = submatrix.transpose();
            
            std::vector<std::size_t> statePriorities = eliminationModelChecker.getStatePriorities(submatrix, submatrixTransposed, newInitialStates, oneStepProbabilities);
            
            // Then, we convert the reduced matrix to a more flexible format to be able to perform state elimination more easily.
            FlexibleMatrix flexibleMatrix = eliminationModelChecker.getFlexibleSparseMatrix(submatrix);
            FlexibleMatrix flexibleBackwardTransitions = eliminationModelChecker.getFlexibleSparseMatrix(submatrixTransposed, true);
            
            std::chrono::high_resolution_clock::time_point timePreprocessingEnd = std::chrono::high_resolution_clock::now();
            
           // Create a bit vector that represents the current subsystem, i.e., states that we have not eliminated.
            storm::storage::BitVector subsystem = storm::storage::BitVector(submatrix.getRowCount(), true);
            eliminateStates(subsystem, flexibleMatrix, oneStepProbabilities, flexibleBackwardTransitions, newInitialStates, submatrix, statePriorities);
            
            std::chrono::high_resolution_clock::time_point timeStateElemEnd = std::chrono::high_resolution_clock::now();
            
            // SMT formulation of resulting pdtmc
            storm::expressions::ExpressionManager manager; //this manager will do nothing as we will use carl expressions
            storm::solver::Smt2SmtSolver solver(manager, true);
            // we will introduce a variable for every state which encodes the probability to reach a target state from this state.
            // we will store them as polynomials to easily use operations with rational functions
            std::vector<storm::RationalFunction::PolyType> stateProbVars(subsystem.size(), storm::RationalFunction::PolyType(0));
            // todo maybe introduce the parameters already at this point?
            formulateModelWithSMT(solver, stateProbVars, subsystem, flexibleMatrix, oneStepProbabilities);
            
            //the property should be satisfied in the initial state for all parameters.
            //this is equivalent to:
            //the negation of the property should not be satisfied for some parameter valuation.
            //Hence, we flip the comparison relation and later check whether all the constraints are unsat.
            storm::CompareRelation propertyCompRel;
            switch (probOpForm.getComparisonType()){
                case storm::logic::ComparisonType::Greater:
                    propertyCompRel=storm::CompareRelation::LEQ;
                    break;
                case storm::logic::ComparisonType::GreaterEqual:
                    propertyCompRel=storm::CompareRelation::LT;
                    break;
                case storm::logic::ComparisonType::Less:
                    propertyCompRel=storm::CompareRelation::GEQ;
                    break;
                case storm::logic::ComparisonType::LessEqual:
                    propertyCompRel=storm::CompareRelation::GT;
                    break;
                default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "the comparison relation of the formula is not supported");
            }
            uint_fast64_t thresholdDenominator = 1.0/storm::settings::generalSettings().getPrecision();
            uint_fast64_t thresholdNumerator = probOpForm.getBound()*thresholdDenominator;
            storm::RationalFunction threshold(thresholdNumerator);
            threshold = threshold / thresholdDenominator;
            solver.add(storm::RationalFunction(stateProbVars[*newInitialStates.begin()]), propertyCompRel, threshold);
            
            //the bounds for the parameters
            solver.push();
            //STORM_LOG_THROW(parameterRegions.size()==1, storm::exceptions::NotImplementedException, "multiple regions not yet implemented");
            ParameterRegion region=parameterRegions[0];
            for(auto variable : region.getVariables()){
                storm::RawPolynomial lB(variable);
                lB -= region.getLowerBound(variable);
                solver.add(carl::Constraint<storm::RawPolynomial>(lB,storm::CompareRelation::GEQ));
                storm::RawPolynomial uB(variable);
                uB -= region.getUpperBound(variable);
                solver.add(carl::Constraint<storm::RawPolynomial>(uB,storm::CompareRelation::LEQ));
            }
            
            std::chrono::high_resolution_clock::time_point timeSmtFormulationEnd = std::chrono::high_resolution_clock::now();
            
            // find further restriction on probabilities
            restrictProbabilityVariables(solver,stateProbVars,subsystem,flexibleMatrix,oneStepProbabilities, parameterRegions[0], storm::logic::ComparisonType::Less); //probOpForm.getComparisonType());
            restrictProbabilityVariables(solver,stateProbVars,subsystem,flexibleMatrix,oneStepProbabilities, parameterRegions[0], storm::logic::ComparisonType::Greater);
            
            std::chrono::high_resolution_clock::time_point timeRestrictingEnd = std::chrono::high_resolution_clock::now();
            
            std::cout << "start solving ..." << std::endl;
            bool result;
                switch (solver.check()){
                case storm::solver::SmtSolver::CheckResult::Sat:
                    std::cout << "sat!" << std::endl;
                    result=false;
                    break;
                case storm::solver::SmtSolver::CheckResult::Unsat:
                    std::cout << "unsat!" << std::endl;
                    result=true;
                    break;
                case storm::solver::SmtSolver::CheckResult::Unknown:
                    std::cout << "unknown!" << std::endl;
                    STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "Could not solve the SMT-Problem (Check-result: Unknown)")
                    result=false;
                    break;
                default:
                    STORM_LOG_THROW(false, storm::exceptions::UnexpectedException, "Could not solve the SMT-Problem (unexpected check-result)")
                    result=false;
            }
            
            std::chrono::high_resolution_clock::time_point timeSolvingEnd = std::chrono::high_resolution_clock::now();    
                
            std::chrono::high_resolution_clock::duration timePreprocessing = timePreprocessingEnd - timeStart;
            std::chrono::high_resolution_clock::duration timeStateElem = timeStateElemEnd - timePreprocessingEnd;
            std::chrono::high_resolution_clock::duration timeSmtFormulation = timeSmtFormulationEnd - timeStateElemEnd;
            std::chrono::high_resolution_clock::duration timeRestricting = timeRestrictingEnd - timeSmtFormulationEnd;
            std::chrono::high_resolution_clock::duration timeSolving = timeSolvingEnd- timeRestrictingEnd;
            std::chrono::high_resolution_clock::duration timeOverall = timeSolvingEnd - timeStart;
            std::chrono::milliseconds timePreprocessingInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timePreprocessing);
            std::chrono::milliseconds timeStateElemInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeStateElem);
            std::chrono::milliseconds timeSmtFormulationInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeSmtFormulation);
            std::chrono::milliseconds timeRestrictingInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeRestricting);
            std::chrono::milliseconds timeSolvingInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeSolving);
            std::chrono::milliseconds timeOverallInMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeOverall);
            STORM_PRINT_AND_LOG(std::endl << "required time: " << timeOverallInMilliseconds.count() << "ms. Time Breakdown:" << std::endl);
            STORM_PRINT_AND_LOG("    * " << timePreprocessingInMilliseconds.count() << "ms for Preprocessing" << std::endl);
            STORM_PRINT_AND_LOG("    * " << timeStateElemInMilliseconds.count() << "ms for StateElemination" << std::endl);
            STORM_PRINT_AND_LOG("    * " << timeSmtFormulationInMilliseconds.count() << "ms for SmtFormulation" << std::endl);
            STORM_PRINT_AND_LOG("    * " << timeRestrictingInMilliseconds.count() << "ms for Restricting" << std::endl);
            STORM_PRINT_AND_LOG("    * " << timeSolvingInMilliseconds.count() << "ms for Solving" << std::endl);

            return result;
        }

        template<typename ParametricType, typename ConstantType>
        bool SparseDtmcRegionModelChecker<ParametricType, ConstantType>::checkRegionOld(storm::logic::Formula const& formula, std::vector<ParameterRegion> parameterRegions){
            STORM_LOG_THROW(false, storm::exceptions::IllegalArgumentException, "Region check is not supported for this type");
        }
        
        #endif


        
        
#ifdef STORM_HAVE_CARL
        template class SparseDtmcRegionModelChecker<storm::RationalFunction, double>;
#endif
        //note: for other template instantiations, add a rule for the typedefs of VariableType and CoefficientType
        
    } // namespace modelchecker
} // namespace storm
