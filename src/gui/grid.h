#ifndef ZC_GUI_GRID_H_
#define ZC_GUI_GRID_H_

#include "gui/widget.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <map>

namespace GUI
{

class Grid: public Widget
{
public:
	enum class type { ROWS, COLUMNS, ROWS_COLUMNS, COLUMNS_ROWS };

	Grid(type growthType, size_t size1, size_t size2 = 0);

	/* Create a new grid that fills left-to-right, then top-to-bottom. */
	static inline std::shared_ptr<Grid> rows(size_t itemsPerRow)
	{
		return std::make_shared<Grid>(type::ROWS, itemsPerRow);
	}

	/* Create a new grid that fills top-to-bottom, then left-to-right. */
	inline static std::shared_ptr<Grid> columns(size_t itemsPerCol)
	{
		return std::make_shared<Grid>(type::COLUMNS, itemsPerCol);
	}

	/* Create a new grid that fills left-to-right, then top-to-bottom, then left-to-right. */
	inline static std::shared_ptr<Grid> rows_columns(size_t itemsPerRow, size_t rowsPerCol)
	{
		return std::make_shared<Grid>(type::ROWS_COLUMNS, itemsPerRow, rowsPerCol);
	}
	
	/* Create a new grid that fills top-to-bottom, then left-to-right, then top-to-bottom. */
	inline static std::shared_ptr<Grid> columns_rows(size_t itemsPerCol, size_t colsPerRow)
	{
		return std::make_shared<Grid>(type::COLUMNS_ROWS, itemsPerCol, colsPerRow);
	}

	/* Set the space between rows. */
	inline void setRowSpacing(Size size) noexcept
	{
		rowSpacing = size.resolve();
	}

	/* Set the space between columns. */
	inline void setColumnSpacing(Size size) noexcept
	{
		colSpacing = size.resolve();
	}

	/* Set the space between rows and columns to the same value. */
	inline void setSpacing(Size size) noexcept
	{
		rowSpacing = size.resolve();
		colSpacing = size.resolve();
	}

	/* Add a widget at the next position in the grid. */
	void add(std::shared_ptr<Widget> child);

	void applyVisibility(bool visible) override;
	void applyDisabled(bool dis) override;
	void calculateSize() override;
	void arrange(int32_t contX, int32_t contY, int32_t contW, int32_t contH) override;
	void realize(DialogRunner& runner) override;
private:
	//std::vector<std::shared_ptr<Widget>> children;
	std::vector<int32_t> rowWidths, colWidths, rowHeights, colHeights;
	size_t rowSpacing, colSpacing;
	
	std::map<size_t, std::shared_ptr<Widget>> children;
	std::map<size_t, bool> usedIndexes;
	
	type growthType;
	size_t size1, size2;
	
	size_t maxChildIndex() const
	{
		if (children.empty()) return 0;
		return children.rbegin()->first;
	}
	size_t get_index(size_t row, size_t col) const;
	size_t get_row(size_t index) const;
	size_t get_col(size_t index) const;
	std::pair<size_t,size_t> get_counts() const;
};

}

#endif
