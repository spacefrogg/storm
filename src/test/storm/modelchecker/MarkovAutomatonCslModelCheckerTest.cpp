#include "gtest/gtest.h"
#include "storm-config.h"

#include "test/storm_gtest.h"

#include "storm/api/builder.h"
#include "storm-parsers/api/model_descriptions.h"
#include "storm/api/properties.h"
#include "storm-parsers/api/properties.h"

#include "storm/models/sparse/MarkovAutomaton.h"
#include "storm/models/symbolic/MarkovAutomaton.h"
#include "storm/models/sparse/StandardRewardModel.h"
#include "storm/modelchecker/csl/SparseMarkovAutomatonCslModelChecker.h"
#include "storm/modelchecker/results/QuantitativeCheckResult.h"
#include "storm/modelchecker/results/QualitativeCheckResult.h"
#include "storm/modelchecker/results/ExplicitQualitativeCheckResult.h"
#include "storm/environment/solver/MinMaxSolverEnvironment.h"
#include "storm/environment/solver/TopologicalSolverEnvironment.h"
#include "storm/settings/modules/CoreSettings.h"
#include "storm/logic/Formulas.h"
#include "storm/storage/jani/Property.h"
#include "storm/exceptions/UncheckedRequirementException.h"

namespace {
    class SparseDoubleValueIterationEnvironment {
    public:
        static const storm::dd::DdType ddType = storm::dd::DdType::Sylvan; // Unused for sparse models
        static const storm::settings::modules::CoreSettings::Engine engine = storm::settings::modules::CoreSettings::Engine::Sparse;
        static const bool isExact = false;
        typedef double ValueType;
        typedef storm::models::sparse::MarkovAutomaton<ValueType> ModelType;
        static storm::Environment createEnvironment() {
            storm::Environment env;
            env.solver().minMax().setMethod(storm::solver::MinMaxMethod::ValueIteration);
            env.solver().minMax().setPrecision(storm::utility::convertNumber<storm::RationalNumber>(1e-10));
            return env;
        }
    };
    class SparseDoubleIntervalIterationEnvironment {
    public:
        static const storm::dd::DdType ddType = storm::dd::DdType::Sylvan; // Unused for sparse models
        static const storm::settings::modules::CoreSettings::Engine engine = storm::settings::modules::CoreSettings::Engine::Sparse;
        static const bool isExact = false;
        typedef double ValueType;
        typedef storm::models::sparse::MarkovAutomaton<ValueType> ModelType;
        static storm::Environment createEnvironment() {
            storm::Environment env;
            env.solver().setForceSoundness(true);
            env.solver().minMax().setMethod(storm::solver::MinMaxMethod::IntervalIteration);
            env.solver().minMax().setPrecision(storm::utility::convertNumber<storm::RationalNumber>(1e-6));
            env.solver().minMax().setRelativeTerminationCriterion(false);
            return env;
        }
    };
    class SparseRationalPolicyIterationEnvironment {
    public:
        static const storm::dd::DdType ddType = storm::dd::DdType::Sylvan; // Unused for sparse models
        static const storm::settings::modules::CoreSettings::Engine engine = storm::settings::modules::CoreSettings::Engine::Sparse;
        static const bool isExact = true;
        typedef storm::RationalNumber ValueType;
        typedef storm::models::sparse::MarkovAutomaton<ValueType> ModelType;
        static storm::Environment createEnvironment() {
            storm::Environment env;
            env.solver().minMax().setMethod(storm::solver::MinMaxMethod::PolicyIteration);
            return env;
        }
    };
    class SparseRationalRationalSearchEnvironment {
    public:
        static const storm::dd::DdType ddType = storm::dd::DdType::Sylvan; // Unused for sparse models
        static const storm::settings::modules::CoreSettings::Engine engine = storm::settings::modules::CoreSettings::Engine::Sparse;
        static const bool isExact = true;
        typedef storm::RationalNumber ValueType;
        typedef storm::models::sparse::MarkovAutomaton<ValueType> ModelType;
        static storm::Environment createEnvironment() {
            storm::Environment env;
            env.solver().minMax().setMethod(storm::solver::MinMaxMethod::RationalSearch);
            return env;
        }
    };

    template<typename TestType>
    class MarkovAutomatonCslModelCheckerTest : public ::testing::Test {
    public:
        typedef typename TestType::ValueType ValueType;
        typedef typename storm::models::sparse::MarkovAutomaton<ValueType> SparseModelType;
        typedef typename storm::models::symbolic::MarkovAutomaton<TestType::ddType, ValueType> SymbolicModelType;
        
        MarkovAutomatonCslModelCheckerTest() : _environment(TestType::createEnvironment()) {}
        storm::Environment const& env() const { return _environment; }
        ValueType parseNumber(std::string const& input) const { return storm::utility::convertNumber<ValueType>(input);}
        ValueType precision() const { return TestType::isExact ? parseNumber("0") : parseNumber("1e-6");}
        bool isSparseModel() const { return std::is_same<typename TestType::ModelType, SparseModelType>::value; }
        bool isSymbolicModel() const { return std::is_same<typename TestType::ModelType, SymbolicModelType>::value; }
        
        template <typename MT = typename TestType::ModelType>
        typename std::enable_if<std::is_same<MT, SparseModelType>::value, std::pair<std::shared_ptr<MT>, std::vector<std::shared_ptr<storm::logic::Formula const>>>>::type
        buildModelFormulas(std::string const& pathToPrismFile, std::string const& formulasAsString, std::string const& constantDefinitionString = "") const {
            std::pair<std::shared_ptr<MT>, std::vector<std::shared_ptr<storm::logic::Formula const>>> result;
            storm::prism::Program program = storm::api::parseProgram(pathToPrismFile);
            program = storm::utility::prism::preprocess(program, constantDefinitionString);
            result.second = storm::api::extractFormulasFromProperties(storm::api::parsePropertiesForPrismProgram(formulasAsString, program));
            result.first = storm::api::buildSparseModel<ValueType>(program, result.second)->template as<MT>();
            return result;
        }
        
        template <typename MT = typename TestType::ModelType>
        typename std::enable_if<std::is_same<MT, SymbolicModelType>::value, std::pair<std::shared_ptr<MT>, std::vector<std::shared_ptr<storm::logic::Formula const>>>>::type
        buildModelFormulas(std::string const& pathToPrismFile, std::string const& formulasAsString, std::string const& constantDefinitionString = "") const {
            std::pair<std::shared_ptr<MT>, std::vector<std::shared_ptr<storm::logic::Formula const>>> result;
            storm::prism::Program program = storm::api::parseProgram(pathToPrismFile);
            program = storm::utility::prism::preprocess(program, constantDefinitionString);
            result.second = storm::api::extractFormulasFromProperties(storm::api::parsePropertiesForPrismProgram(formulasAsString, program));
            result.first = storm::api::buildSymbolicModel<TestType::ddType, ValueType>(program, result.second)->template as<MT>();
            return result;
        }
        
        std::vector<storm::modelchecker::CheckTask<storm::logic::Formula, ValueType>> getTasks(std::vector<std::shared_ptr<storm::logic::Formula const>> const& formulas) const {
            std::vector<storm::modelchecker::CheckTask<storm::logic::Formula, ValueType>> result;
            for (auto const& f : formulas) {
                result.emplace_back(*f);
            }
            return result;
        }
        
        template <typename MT = typename TestType::ModelType>
        typename std::enable_if<std::is_same<MT, SparseModelType>::value, std::shared_ptr<storm::modelchecker::AbstractModelChecker<MT>>>::type
        createModelChecker(std::shared_ptr<MT> const& model) const {
            if (TestType::engine == storm::settings::modules::CoreSettings::Engine::Sparse) {
                return std::make_shared<storm::modelchecker::SparseMarkovAutomatonCslModelChecker<SparseModelType>>(*model);
            }
        }
        
        template <typename MT = typename TestType::ModelType>
        typename std::enable_if<std::is_same<MT, SymbolicModelType>::value, std::shared_ptr<storm::modelchecker::AbstractModelChecker<MT>>>::type
        createModelChecker(std::shared_ptr<MT> const& model) const {
//            if (TestType::engine == storm::settings::modules::CoreSettings::Engine::Hybrid) {
//              return std::make_shared<storm::modelchecker::HybridMarkovAutomatonCslModelChecker<SymbolicModelType>>(*model);
//            } else if (TestType::engine == storm::settings::modules::CoreSettings::Engine::Dd) {
//                return std::make_shared<storm::modelchecker::SymbolicMarkovAutomatonCslModelChecker<SymbolicModelType>>(*model);
//            }
            return nullptr;
        }
        
        bool getQualitativeResultAtInitialState(std::shared_ptr<storm::models::Model<ValueType>> const& model, std::unique_ptr<storm::modelchecker::CheckResult>& result) {
            auto filter = getInitialStateFilter(model);
            result->filter(*filter);
            return result->asQualitativeCheckResult().forallTrue();
        }
        
        ValueType getQuantitativeResultAtInitialState(std::shared_ptr<storm::models::Model<ValueType>> const& model, std::unique_ptr<storm::modelchecker::CheckResult>& result) {
            auto filter = getInitialStateFilter(model);
            result->filter(*filter);
            return result->asQuantitativeCheckResult<ValueType>().getMin();
        }
    
    private:
        storm::Environment _environment;
        
        std::unique_ptr<storm::modelchecker::QualitativeCheckResult> getInitialStateFilter(std::shared_ptr<storm::models::Model<ValueType>> const& model) const {
            if (isSparseModel()) {
                return std::make_unique<storm::modelchecker::ExplicitQualitativeCheckResult>(model->template as<SparseModelType>()->getInitialStates());
            } else {
            //    return std::make_unique<storm::modelchecker::SymbolicQualitativeCheckResult<TestType::ddType>>(model->template as<SymbolicModelType>()->getReachableStates(), model->template as<SymbolicModelType>()->getInitialStates());
                return nullptr;
            }
        }
    };
  
    typedef ::testing::Types<
            SparseDoubleValueIterationEnvironment,
            SparseDoubleIntervalIterationEnvironment,
            SparseRationalPolicyIterationEnvironment,
            SparseRationalRationalSearchEnvironment
        > TestingTypes;
    
    TYPED_TEST_CASE(MarkovAutomatonCslModelCheckerTest, TestingTypes);
    

    TYPED_TEST(MarkovAutomatonCslModelCheckerTest, server) {
        std::string formulasString = "Tmax=? [F \"error\"]";
                 formulasString += "; Pmax=? [F \"processB\"]";
                 formulasString += "; Pmax=? [F<1 \"error\"]";
        
        auto modelFormulas = this->buildModelFormulas(STORM_TEST_RESOURCES_DIR "/ma/server.ma", formulasString);
        auto model = std::move(modelFormulas.first);
        auto tasks = this->getTasks(modelFormulas.second);
        EXPECT_EQ(6ul, model->getNumberOfStates());
        EXPECT_EQ(10ul, model->getNumberOfTransitions());
        ASSERT_EQ(model->getType(), storm::models::ModelType::MarkovAutomaton);
        auto checker = this->createModelChecker(model);
        std::unique_ptr<storm::modelchecker::CheckResult> result;
        
        result = checker->check(this->env(), tasks[0]);
        EXPECT_NEAR(this->parseNumber("11/6"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
        
        result = checker->check(this->env(), tasks[1]);
        EXPECT_NEAR(this->parseNumber("2/3"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
        
        if (!storm::utility::isZero(this->precision())) {
            result = checker->check(this->env(), tasks[2]);
            EXPECT_NEAR(this->parseNumber("0.455504"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
        }
        
    }
    
    TYPED_TEST(MarkovAutomatonCslModelCheckerTest, simple) {
        std::string formulasString = "Pmin=? [F<1 s>2]";
                 formulasString += "; Pmax=? [F<1.3 s=3]";

        auto modelFormulas = this->buildModelFormulas(STORM_TEST_RESOURCES_DIR "/ma/simple.ma", formulasString);
        auto model = std::move(modelFormulas.first);
        auto tasks = this->getTasks(modelFormulas.second);
        EXPECT_EQ(5ul, model->getNumberOfStates());
        EXPECT_EQ(8ul, model->getNumberOfTransitions());
        ASSERT_EQ(model->getType(), storm::models::ModelType::MarkovAutomaton);
        auto checker = this->createModelChecker(model);
        std::unique_ptr<storm::modelchecker::CheckResult> result;
        
        if (!storm::utility::isZero(this->precision())) {
            result = checker->check(this->env(), tasks[0]);
            EXPECT_NEAR(this->parseNumber("0.6321205588"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
            
            result = checker->check(this->env(), tasks[1]);
            EXPECT_NEAR(this->parseNumber("0.727468207"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
        }
    }
    
    TYPED_TEST(MarkovAutomatonCslModelCheckerTest, simple2) {
        std::string formulasString = "R{\"rew0\"}max=? [C]";
                    formulasString += "; R{\"rew0\"}min=? [C]";
                    formulasString += "; R{\"rew1\"}max=? [C]";
                    formulasString += "; R{\"rew1\"}min=? [C]";
                    formulasString += "; R{\"rew2\"}max=? [C]";
                    formulasString += "; R{\"rew2\"}min=? [C]";
                    formulasString += "; R{\"rew3\"}min=? [C]";
        
        auto modelFormulas = this->buildModelFormulas(STORM_TEST_RESOURCES_DIR "/ma/simple2.ma", formulasString);
        auto model = std::move(modelFormulas.first);
        auto tasks = this->getTasks(modelFormulas.second);
        EXPECT_EQ(6ul, model->getNumberOfStates());
        EXPECT_EQ(11ul, model->getNumberOfTransitions());
        ASSERT_EQ(model->getType(), storm::models::ModelType::MarkovAutomaton);
        auto checker = this->createModelChecker(model);
        std::unique_ptr<storm::modelchecker::CheckResult> result;

        result = checker->check(this->env(), tasks[0]);
        EXPECT_NEAR(this->parseNumber("2"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
 
        result = checker->check(this->env(), tasks[1]);
        EXPECT_NEAR(this->parseNumber("0"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
 
        result = checker->check(this->env(), tasks[2]);
        EXPECT_TRUE(storm::utility::isInfinity(this->getQuantitativeResultAtInitialState(model, result)));
 
        result = checker->check(this->env(), tasks[3]);
        EXPECT_NEAR(this->parseNumber("7/8"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
 
        result = checker->check(this->env(), tasks[4]);
        EXPECT_TRUE(storm::utility::isInfinity(this->getQuantitativeResultAtInitialState(model, result)));
 
        result = checker->check(this->env(), tasks[5]);
        EXPECT_NEAR(this->parseNumber("7/8"), this->getQuantitativeResultAtInitialState(model, result), this->precision());
 
        result = checker->check(this->env(), tasks[6]);
        EXPECT_TRUE(storm::utility::isInfinity(this->getQuantitativeResultAtInitialState(model, result)));
 
    }
}