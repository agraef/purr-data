#include "Pattern.hpp"

#include <iostream>

#include "Common.hpp"

using std::cout;
using std::cerr;
using std::endl;

Pattern::Pattern(int numRows, int numCols, string patternName)
: name(patternName)
{
	Cell empty_cell;
	Row empty_row;

	SETSYMBOL(&empty_cell, gensym("empty"));

	while(numCols-- > 0)
	{
		empty_row.push_back(empty_cell);
	}

	columns = empty_row.size();

	while(numRows-- > 0)
	{
		Row row(empty_row);
		rows.push_back(row);
	}
}

void Pattern::print()
{
	cerr << "---- Pattern: " << name << " ----" << endl;

	char buf[MAXPDSTRING];

	for(unsigned int i = 0; i < rows.size(); i++)
	{
		cerr << "  Row[" << i << "]: ";
		for(unsigned int j = 0; j < rows[i].size(); j++)
		{
			if(j > 0) cerr << ", ";
			atom_string(&rows[i][j], buf, MAXPDSTRING);
			cerr << buf << endl;
		}
	}

	cerr << "---- End pattern (" << name << ") ----" << endl;
}

void Pattern::resize(int numRows, int numCols)
{
	Cell empty_cell;
	Row empty_row;

	SETSYMBOL(&empty_cell, gensym("empty"));

	while(numCols-- > 0)
	{
		empty_row.push_back(empty_cell);
	}

	rows.resize(numRows, empty_row);

	for(unsigned int i = 0; i < rows.size(); i++)
	{
		rows[i].resize(empty_row.size(), empty_cell);
	}

	columns = empty_row.size();
}

void Pattern::setCell(int row, int col, Cell cell)
{
    row = WRAP(row, rows.size());
    col = WRAP(col, columns);
	rows[row][col] = cell;
}

Cell Pattern::getCell(int row, int col)
{
    row = WRAP(row, rows.size());
    col = WRAP(col, columns);
	return rows[row][col];
}
