#include "src/solver/LinearEquationSolver.h"

#include "src/solver/SolverSelectionOptions.h"

#include "src/solver/GmmxxLinearEquationSolver.h"
#include "src/solver/NativeLinearEquationSolver.h"
#include "src/solver/EigenLinearEquationSolver.h"
#include "src/solver/EliminationLinearEquationSolver.h"

#include "src/settings/SettingsManager.h"
#include "src/settings/modules/CoreSettings.h"

namespace storm {
    namespace solver {
        
        template<typename ValueType>
        LinearEquationSolver<ValueType>::LinearEquationSolver() {
            // Intentionally left empty.
        }
        
        template<typename ValueType>
        void LinearEquationSolver<ValueType>::repeatedMultiply(std::vector<ValueType>& x, std::vector<ValueType> const* b, uint_fast64_t n) const {
            
            if(!auxiliaryRowVector) {
                auxiliaryRowVector = std::make_unique<std::vector<ValueType>>(getMatrixRowCount());
            }
            
            // Set up some temporary variables so that we can just swap pointers instead of copying the result after
            // each iteration.
            std::vector<ValueType>* currentX = &x;
            std::vector<ValueType>* nextX = auxiliaryRowVector.get();
            
            // Now perform matrix-vector multiplication as long as we meet the bound.
            for (uint_fast64_t i = 0; i < n; ++i) {
                this->multiply(*currentX, b, *nextX);
                std::swap(nextX, currentX);
            }
            
            // If we performed an odd number of repetitions, we need to swap the contents of currentVector and x,
            // because the output is supposed to be stored in the input vector x.
            if (currentX == auxiliaryRowVector.get()) {
                std::swap(x, *currentX);
            }
        }
        
        template<typename ValueType>
        void LinearEquationSolver<ValueType>::resetAuxiliaryData() const {
            auxiliaryRowVector.reset();
        }
      
        template<typename ValueType>
        std::unique_ptr<LinearEquationSolver<ValueType>> LinearEquationSolverFactory<ValueType>::create(storm::storage::SparseMatrix<ValueType>&& matrix) const {
            return create(matrix);
        }
        
        template<typename ValueType>
        std::unique_ptr<LinearEquationSolver<ValueType>> GeneralLinearEquationSolverFactory<ValueType>::create(storm::storage::SparseMatrix<ValueType> const& matrix) const {
            return selectSolver(matrix);
        }
        
        template<typename ValueType>
        std::unique_ptr<LinearEquationSolver<ValueType>> GeneralLinearEquationSolverFactory<ValueType>::create(storm::storage::SparseMatrix<ValueType>&& matrix) const {
            return selectSolver(std::move(matrix));
        }
        
        template<typename ValueType>
        template<typename MatrixType>
        std::unique_ptr<LinearEquationSolver<ValueType>> GeneralLinearEquationSolverFactory<ValueType>::selectSolver(MatrixType&& matrix) const {
            EquationSolverType equationSolver = storm::settings::getModule<storm::settings::modules::CoreSettings>().getEquationSolver();
            switch (equationSolver) {
                case EquationSolverType::Gmmxx: return std::make_unique<GmmxxLinearEquationSolver<ValueType>>(std::forward<MatrixType>(matrix));
                case EquationSolverType::Native: return std::make_unique<NativeLinearEquationSolver<ValueType>>(std::forward<MatrixType>(matrix));
                case EquationSolverType::Eigen: return std::make_unique<EigenLinearEquationSolver<ValueType>>(std::forward<MatrixType>(matrix));
                case EquationSolverType::Elimination: return std::make_unique<EliminationLinearEquationSolver<ValueType>>(std::forward<MatrixType>(matrix));
                default: return std::make_unique<GmmxxLinearEquationSolver<ValueType>>(std::forward<MatrixType>(matrix));
            }
        }
        
        template<typename ValueType>
        std::unique_ptr<LinearEquationSolverFactory<ValueType>> GeneralLinearEquationSolverFactory<ValueType>::clone() const {
            return std::make_unique<GeneralLinearEquationSolverFactory<ValueType>>(*this);
        }
        
#ifdef STORM_HAVE_CARL
        std::unique_ptr<LinearEquationSolver<storm::RationalNumber>> GeneralLinearEquationSolverFactory<storm::RationalNumber>::create(storm::storage::SparseMatrix<storm::RationalNumber> const& matrix) const {
            return selectSolver(matrix);
        }
        
        std::unique_ptr<LinearEquationSolver<storm::RationalNumber>> GeneralLinearEquationSolverFactory<storm::RationalNumber>::create(storm::storage::SparseMatrix<storm::RationalNumber>&& matrix) const {
            return selectSolver(std::move(matrix));
        }
        
        template<typename MatrixType>
        std::unique_ptr<LinearEquationSolver<storm::RationalNumber>> GeneralLinearEquationSolverFactory<storm::RationalNumber>::selectSolver(MatrixType&& matrix) const {
            EquationSolverType equationSolver = storm::settings::getModule<storm::settings::modules::CoreSettings>().getEquationSolver();
            switch (equationSolver) {
                case EquationSolverType::Elimination: return std::make_unique<EliminationLinearEquationSolver<storm::RationalNumber>>(std::forward<MatrixType>(matrix));
                default: return std::make_unique<EigenLinearEquationSolver<storm::RationalNumber>>(std::forward<MatrixType>(matrix));
            }
        }
        
        std::unique_ptr<LinearEquationSolverFactory<storm::RationalNumber>> GeneralLinearEquationSolverFactory<storm::RationalNumber>::clone() const {
            return std::make_unique<GeneralLinearEquationSolverFactory<storm::RationalNumber>>(*this);
        }
        
        std::unique_ptr<LinearEquationSolver<storm::RationalFunction>> GeneralLinearEquationSolverFactory<storm::RationalFunction>::create(storm::storage::SparseMatrix<storm::RationalFunction> const& matrix) const {
            return selectSolver(matrix);
        }
        
        std::unique_ptr<LinearEquationSolver<storm::RationalFunction>> GeneralLinearEquationSolverFactory<storm::RationalFunction>::create(storm::storage::SparseMatrix<storm::RationalFunction>&& matrix) const {
            return selectSolver(std::move(matrix));
        }
        
        template<typename MatrixType>
        std::unique_ptr<LinearEquationSolver<storm::RationalFunction>> GeneralLinearEquationSolverFactory<storm::RationalFunction>::selectSolver(MatrixType&& matrix) const {
            EquationSolverType equationSolver = storm::settings::getModule<storm::settings::modules::CoreSettings>().getEquationSolver();
            switch (equationSolver) {
                case EquationSolverType::Elimination: return std::make_unique<EliminationLinearEquationSolver<storm::RationalFunction>>(std::forward<MatrixType>(matrix));
                default: return std::make_unique<EigenLinearEquationSolver<storm::RationalFunction>>(std::forward<MatrixType>(matrix));
            }
        }
        
        std::unique_ptr<LinearEquationSolverFactory<storm::RationalFunction>> GeneralLinearEquationSolverFactory<storm::RationalFunction>::clone() const {
            return std::make_unique<GeneralLinearEquationSolverFactory<storm::RationalFunction>>(*this);
        }
#endif

        template class LinearEquationSolver<double>;
        template class LinearEquationSolverFactory<double>;
        template class GeneralLinearEquationSolverFactory<double>;

#ifdef STORM_HAVE_CARL
        template class LinearEquationSolver<storm::RationalNumber>;
        template class LinearEquationSolver<storm::RationalFunction>;

        template class LinearEquationSolverFactory<storm::RationalNumber>;
        template class LinearEquationSolverFactory<storm::RationalFunction>;
        
        template class GeneralLinearEquationSolverFactory<storm::RationalNumber>;
        template class GeneralLinearEquationSolverFactory<storm::RationalFunction>;
#endif

    }
}
