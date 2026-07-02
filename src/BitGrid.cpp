#include "BitGrid.h"

namespace cube3d {

bool FaceGrid::get(int row, int col) const {
    return bits_.test(static_cast<size_t>(flatIndex(row, col)));
}

void FaceGrid::set(int row, int col, bool value) {
    bits_.set(static_cast<size_t>(flatIndex(row, col)), value);
}

bool FaceGrid::pegAt(int tileRow, int tileCol, int pegRow, int pegCol) const {
    const int row = tileRow * kPegsPerTileSide + pegRow;
    const int col = tileCol * kPegsPerTileSide + pegCol;
    return get(row, col);
}

void FaceGrid::setPeg(int tileRow, int tileCol, int pegRow, int pegCol, bool value) {
    const int row = tileRow * kPegsPerTileSide + pegRow;
    const int col = tileCol * kPegsPerTileSide + pegCol;
    set(row, col, value);
}

} // namespace cube3d
