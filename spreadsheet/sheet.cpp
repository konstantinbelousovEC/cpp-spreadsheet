#include "sheet.h"
#include "cell.h"
#include "common.h"
#include <algorithm>
#include <functional>
#include <iostream>

using namespace std::literals;

enum class Sheet::PrintType {
    TEXT,
    VALUE
};

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position value range"s);
    Cell* cell_existing = static_cast<Cell*>(GetCell(pos));
    if (cell_existing == nullptr) {
        std::unique_ptr<Cell> cell = std::make_unique<Cell>(*this, pos);
        cell->Set(text);
        cells_[pos] = std::move(cell);
    } else {
        if (cell_existing->GetText() == text) return;
        cell_existing->Set(text);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position value range"s);
    if (cells_.count(pos) > 0) {
        return cells_.at(pos).get();
    } else {
        return nullptr;
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position value range"s);
    if (cells_.count(pos) > 0) {
        return cells_[pos].get();
    } else {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("invalid position value range"s);
    if (cells_.count(pos) > 0) {
        cells_[pos]->Clear();
        cells_.erase(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    if (cells_.empty()) return {0,0};
    int max_row = 0;
    int max_col = 0;
    for (auto& it : cells_) {
        max_row = std::max(max_row, it.first.row);
        max_col = std::max(max_col, it.first.col);
    }
    return {max_row + 1, max_col + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    Sheet::Print(output, PrintType::VALUE);
}

void Sheet::PrintTexts(std::ostream& output) const {
    Sheet::Print(output, PrintType::TEXT);
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& val){
        output << val;
    }, value);
    return output;
}

void Sheet::Print(std::ostream& output, PrintType type) const {
    if (cells_.empty()) {
        output << ""s;
        return;
    }
    Size printable_area = GetPrintableSize();
    for (int row = 0; row < printable_area.rows; ++row) {
        for (int col = 0; col < printable_area.cols; ++col) {
            const auto& cell = GetCell({row, col});
            if (cell == nullptr) {
                output << ""s;
            } else if (type == PrintType::TEXT) {
                output << cell->GetText();
            } else {
                output << cell->GetValue();
            }
            if (col < printable_area.cols - 1) output << "\t";
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}