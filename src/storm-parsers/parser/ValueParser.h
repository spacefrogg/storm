#ifndef STORM_PARSER_VALUEPARSER_H_
#define STORM_PARSER_VALUEPARSER_H_

#include "storm/storage/expressions/ExpressionManager.h"
#include "storm-parsers/parser/ExpressionParser.h"
#include "storm/storage/expressions/ExpressionEvaluator.h"
#include "storm/exceptions/WrongFormatException.h"

namespace storm {
    namespace parser {
        /*!
         * Parser for values according to their ValueType.
         */
        template<typename ValueType>
        class ValueParser {
        public:

            /*!
             * Constructor.
             */
            ValueParser() : manager(new storm::expressions::ExpressionManager()), parser(*manager), evaluator(*manager) {
            }

            /*!
             * Parse ValueType from string.
             *
             * @param value String containing the value.
             *
             * @return ValueType
             */
            ValueType parseValue(std::string const& value) const;

            /*!
             * Add declaration of parameter.
             *
             * @param parameter New parameter.
             */
            void addParameter(std::string const& parameter);

        private:

            std::shared_ptr<storm::expressions::ExpressionManager> manager;
            storm::parser::ExpressionParser parser;
            storm::expressions::ExpressionEvaluator<ValueType> evaluator;
            std::unordered_map<std::string, storm::expressions::Expression> identifierMapping;
        };

        template<typename NumberType>
        class NumberParser {
        public:
            /*!
             * Parse number from string.
             *
             * @param value String containing the value.
             *
             * @return NumberType.
             */
            static NumberType parse(std::string const& value) {
                try {
                    return boost::lexical_cast<NumberType>(value);
                }
                catch(boost::bad_lexical_cast &) {
                    STORM_LOG_THROW(false, storm::exceptions::WrongFormatException, "Could not parse value '" << value << "' into " << typeid(NumberType).name() << ".");
                }
            }
        };

    } // namespace parser
} // namespace storm

#endif /* STORM_PARSER_VALUEPARSER_H_ */
