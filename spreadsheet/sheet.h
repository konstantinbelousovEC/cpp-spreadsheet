#pragma once

#include "cell.h"
#include "common.h"
#include <functional>
#include <vector>
#include <unordered_map>


class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;
    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    void ClearCell(Position pos) override;
    Size GetPrintableSize() const override;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    enum class PrintType;

    std::unordered_map<Position, std::unique_ptr<Cell>, std::hash<Position>> cells_;
    void Print(std::ostream& output, PrintType type) const;
};