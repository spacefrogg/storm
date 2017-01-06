#include "storm.h"

// Headers related to parsing.
#include "storm/parser/JaniParser.h"

#include "storm/parser/PrismParser.h"
#include "storm/parser/FormulaParser.h"
#include "storm/utility/macros.h"
#include "storm/storage/jani/Property.h"

namespace storm{
    
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulasInProperties(std::vector<storm::jani::Property> const& properties) {
        
        std::vector<std::shared_ptr<storm::logic::Formula const>> formulas;
        for (auto const& prop : properties) {
            formulas.push_back(prop.getFilter().getFormula());
        }
        return formulas;
    }
   
     storm::prism::Program parseProgram(std::string const& path) {
        storm::prism::Program program = storm::parser::PrismParser::parse(path).simplify().simplify();
        program.checkValidity();
        return program;
    }

    std::pair<storm::jani::Model, std::map<std::string, storm::jani::Property>> parseJaniModel(std::string const& path) {
        std::pair<storm::jani::Model, std::map<std::string, storm::jani::Property>> modelAndFormulae = storm::parser::JaniParser::parse(path);
        modelAndFormulae.first.checkValid();
        return modelAndFormulae;
    }
    
    void exportJaniModel(storm::jani::Model const& model, std::vector<storm::jani::Property> const& properties, std::string const& filepath) {
        STORM_LOG_TRACE("Exporting JANI model.");
        if (storm::settings::getModule<storm::settings::modules::JaniExportSettings>().isExportAsStandardJaniSet()) {
            storm::jani::Model normalisedModel = model;
            normalisedModel.makeStandardJaniCompliant();
            storm::jani::JsonExporter::toFile(normalisedModel, properties, filepath);
        } else {
            storm::jani::JsonExporter::toFile(model, properties, filepath);
        }
    }

    /**
     * Helper
     * @param FormulaParser
     * @return The formulas.
     */
    std::vector<std::shared_ptr<storm::logic::Formula const>> parseFormulas(storm::parser::FormulaParser & formulaParser, std::string const& inputString) {
        // If the given property looks like a file (containing a dot and there exists a file with that name),
        // we try to parse it as a file, otherwise we assume it's a property.
        if (inputString.find(".") != std::string::npos && std::ifstream(inputString).good()) {
            return formulaParser.parseFromFile(inputString);
        } else {
            return formulaParser.parseFromString(inputString);
        }
    }

    std::vector<std::shared_ptr<storm::logic::Formula const>> parseFormulasForExplicit(std::string const& inputString) {
        auto exprManager = std::make_shared<storm::expressions::ExpressionManager>();
        storm::parser::FormulaParser formulaParser(exprManager);
        return parseFormulas(formulaParser, inputString);
    }

    std::vector<std::shared_ptr<storm::logic::Formula const>> substituteConstantsInFormulas(std::vector<std::shared_ptr<storm::logic::Formula const>> const& formulas, std::map<storm::expressions::Variable, storm::expressions::Expression> const& substitution) {
        std::vector<std::shared_ptr<storm::logic::Formula const>> preprocessedFormulas;
        for (auto const& formula : formulas) {
            preprocessedFormulas.emplace_back(formula->substitute(substitution));
        }
        return preprocessedFormulas;
    }
    
    std::vector<std::shared_ptr<storm::logic::Formula const>> parseFormulasForJaniModel(std::string const& inputString, storm::jani::Model const& model) {
        storm::parser::FormulaParser formulaParser(model.getManager().getSharedPointer());
        auto formulas = parseFormulas(formulaParser, inputString);
        return substituteConstantsInFormulas(formulas, model.getConstantsSubstitution());
    }
    
    std::vector<std::shared_ptr<storm::logic::Formula const>> parseFormulasForPrismProgram(std::string const& inputString, storm::prism::Program const& program) {
        storm::parser::FormulaParser formulaParser(program);
        auto formulas = parseFormulas(formulaParser, inputString);
        return substituteConstantsInFormulas(formulas, program.getConstantsSubstitution());
    } 
}