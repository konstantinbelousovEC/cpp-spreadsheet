#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {

    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression);
        Value Evaluate(const SheetInterface &sheet) const override;
        std::string GetExpression() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        FormulaAST ast_;
        std::vector<Position> referenced_cells_;
    };

    Formula::Formula(std::string expression)
    try : ast_(ParseFormulaAST(expression)),
          referenced_cells_(ast_.GetCells().begin(), ast_.GetCells().end())
    {
        auto end_iterator = std::unique(referenced_cells_.begin(), referenced_cells_.end());
        referenced_cells_.resize(end_iterator - referenced_cells_.begin());
    } catch (std::exception& error) {
        throw FormulaException("Incorrect formula: "s.append(error.what()));
    }

    FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const {
        auto lambda = [&](Position pos) -> double {
            const CellInterface* cell = sheet.GetCell(pos);
            if (cell == nullptr) return 0.0;

            CellInterface::Value value = cell->GetValue();
            if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                std::string string_value = std::get<std::string>(value);
                if (string_value.empty()) return 0.0;
                try {
                    double double_value = stod(string_value);
                    return double_value;
                } catch (std::invalid_argument&) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            } else {
                throw FormulaError(std::get<FormulaError>(value));
            }
        };

        double result;
        try {
            result = ast_.Execute(lambda);
        } catch (FormulaError &err) {
            return err;
        }
        return result;
    }

    std::string Formula::GetExpression() const {
        std::stringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> Formula::GetReferencedCells() const {
        return referenced_cells_;
    }
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
        return std::make_unique<Formula>(std::move(expression));
}