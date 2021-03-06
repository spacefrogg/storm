#ifndef STORM_LOGIC_LONGRUNAVERAGEREWARDFORMULA_H_
#define STORM_LOGIC_LONGRUNAVERAGEREWARDFORMULA_H_

#include "storm/logic/PathFormula.h"

namespace storm {
    namespace logic {
        class LongRunAverageRewardFormula : public PathFormula {
        public:
            LongRunAverageRewardFormula();
            
            virtual ~LongRunAverageRewardFormula() {
                // Intentionally left empty.
            }
            
            virtual bool isLongRunAverageRewardFormula() const override;
            virtual bool isRewardPathFormula() const override;

            virtual boost::any accept(FormulaVisitor const& visitor, boost::any const& data) const override;
            
            virtual std::ostream& writeToStream(std::ostream& out) const override;
            
        };
    }
}

#endif /* STORM_LOGIC_LONGRUNAVERAGEREWARDFORMULA_H_ */
