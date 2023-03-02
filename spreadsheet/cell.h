#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class CellImpl;

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet, Position position);
    ~Cell();

    void Set(const std::string& text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    using PositionsSet = std::unordered_set<Position, std::hash<Position>>;

    SheetInterface& sheet_;
    std::unique_ptr<CellImpl> impl_;
    Position position_;
    PositionsSet cells_included_me_;
    PositionsSet cells_included_by_me_;

    bool HasCircularDependencies(Position start_position, PositionsSet& visited, const PositionsSet& new_dependents) const;
    bool HasCircularDependencies(const PositionsSet& new_dependents) const;
    void RemoveDependencies();
    void AddDependencies();
    void UpdateDependencies(PositionsSet&& cells_included_by_me_tmp);
    void InvalidateCache();
};