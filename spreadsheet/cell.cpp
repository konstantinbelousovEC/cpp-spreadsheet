#include "cell.h"
#include <string>
#include <optional>

using namespace std::literals;

class CellImpl {
public:
    using Value = std::variant<std::string, double, FormulaError>;
    virtual std::string GetText() const = 0;
    virtual Value GetValue(const SheetInterface& sheet) const = 0;
    virtual bool HasCache() const;
    virtual void InvalidateCache();
    virtual std::vector<Position> GetReferencedCells() const;
    virtual ~CellImpl() = default;
};

class EmptyImpl : public CellImpl {
public:
    std::string GetText() const override { return ""s; }
    Value GetValue(const SheetInterface&) const override { return ""s; }
};

class TextImpl : public CellImpl {
public:
    TextImpl() = delete;
    explicit TextImpl(std::string expression);
    std::string GetText() const override;
    Value GetValue(const SheetInterface&) const override;
private:
    std::string value_;
};

class FormulaImpl : public CellImpl {
public:
    explicit FormulaImpl(const std::string& expression);
    std::string GetText() const override;
    Value GetValue(const SheetInterface& sheet) const override;
    virtual bool HasCache() const override;
    void InvalidateCache() override;
    std::vector<Position> GetReferencedCells() const override;
private:
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<Value> cache_;
};

Cell::Cell(SheetInterface& sheet, Position position)
: sheet_(sheet), impl_(std::make_unique<EmptyImpl>()), position_(position) {}

Cell::Cell(Cell &&other)
: sheet_(other.sheet_),
  impl_(std::move(other.impl_)),
  position_(other.position_),
  cells_included_me_(std::move(other.cells_included_me_)),
  cells_included_by_me_(std::move(other.cells_included_by_me_)) {}

Cell::~Cell() = default;

void Cell::Set(const std::string& text) {
    if (text.front() == FORMULA_SIGN && text.size() != 1) {
        std::unique_ptr<FormulaImpl> impl_tmp = std::make_unique<FormulaImpl>(text.substr(1));
        const std::vector<Position>& positions = impl_tmp->GetReferencedCells();
        PositionsSet cells_included_by_me_tmp(positions.begin(), positions.end());
        if (HasCircularDependencies(cells_included_by_me_tmp)) throw CircularDependencyException("Circular dependency was found"s);
        UpdateDependencies(std::move(cells_included_by_me_tmp));
        impl_ = std::move(impl_tmp);
    } else {
        UpdateDependencies(PositionsSet{});
        if (text.empty()) {
            impl_ = std::make_unique<EmptyImpl>();
        } else {
            impl_ = std::make_unique<TextImpl>(text);
        }
    }
}

void Cell::Clear() {
    Set(""s);
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue(sheet_);
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

void Cell::UpdateDependencies(PositionsSet&& cells_included_by_me_tmp) {
    InvalidateCache();
    RemoveDependencies();
    cells_included_by_me_ = std::move(cells_included_by_me_tmp);
    for (const Position& pos : cells_included_by_me_) {
        if (sheet_.GetCell(pos) == nullptr) sheet_.SetCell(pos, ""s);
    }
    AddDependencies();
}

void Cell::RemoveDependencies() {
    for (Position position : cells_included_by_me_) {
        Cell* cell = static_cast<Cell*>(sheet_.GetCell(position));
        if (cell != nullptr) cell->cells_included_me_.erase(position_);
    }
}

void Cell::AddDependencies() {
    for (Position position : cells_included_by_me_) {
        Cell* cell = static_cast<Cell*>(sheet_.GetCell(position));
        if (cell != nullptr) cell->cells_included_me_.insert(position_);
    }
}

bool Cell::HasCircularDependencies(const PositionsSet& dependents) const {
    PositionsSet visited;
    return HasCircularDependencies(position_, visited, dependents);
}

bool Cell::HasCircularDependencies(Position start_position, PositionsSet& visited, const PositionsSet& dependents) const {
    for (Position pos : dependents) {
        if (visited.count(pos) > 0) continue;
        visited.insert(pos);

        Cell* cell = static_cast<Cell*>(sheet_.GetCell(pos));

        if (pos == start_position) return true;
        if (cell == nullptr) continue;

        if (HasCircularDependencies(
                start_position,
                visited,
                cell->cells_included_by_me_)) {
            return true;
        }
    }
    return false;
}

void Cell::InvalidateCache() {
    if (impl_->HasCache()) impl_->InvalidateCache();
    for (Position position : cells_included_me_) {
        Cell* cell = static_cast<Cell*>(sheet_.GetCell(position));
        if (cell != nullptr) cell->InvalidateCache();
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

// CellImpl definitions

bool CellImpl::HasCache() const { return false; }
void CellImpl::InvalidateCache() {}
std::vector<Position> CellImpl::GetReferencedCells() const { return {}; }

TextImpl::TextImpl(std::string expression)
: value_(std::move(expression)) {}

std::string TextImpl::GetText() const { return value_; }

CellImpl::Value TextImpl::GetValue(const SheetInterface&) const {
    if (value_.front() == ESCAPE_SIGN) {
        return value_.substr(1);
    } else {
        return value_;
    }
}

FormulaImpl::FormulaImpl(const std::string& expression)
: formula_(ParseFormula(expression)) {}

std::string FormulaImpl::GetText() const {
    std::string result = "="s;
    result.append(formula_->GetExpression());
    return result;
}

CellImpl::Value FormulaImpl::GetValue(const SheetInterface& sheet) const {
    if (HasCache()) return cache_.value();
    std::variant<double, FormulaError> res = formula_->Evaluate(sheet);
    std::visit([&](const auto& val){
        cache_ = val;
    }, res);
    return cache_.value();
}

bool FormulaImpl::HasCache() const {
    return cache_.has_value();
}

void FormulaImpl::InvalidateCache() {
    cache_.reset();
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}