#ifndef COMPOSER_PATTERN_H_INCLUDED
#define COMPOSER_PATTERN_H_INCLUDED

#include <string>
#include <vector>

#include <m_pd.h>

using std::string;
using std::vector;

typedef t_atom Cell;

typedef vector<Cell> Row;

class Pattern
{
private:
	string name;
	vector<Row> rows;
	int columns;
public:
	Pattern(int numRows, int numCols, string patternName);
	void print();
	void resize(int numRows, int numCols);
	void setCell(int row, int col, Cell cell);
	Cell getCell(int row, int col);
	inline const string &getName() {return name;}
    inline void setName(const string &newName) {name = newName;}
	inline unsigned int getRows() {return rows.size();}
	inline unsigned int getColumns() {return columns;}
};

#endif // COMPOSER_PATTERN_H_INCLUDED
