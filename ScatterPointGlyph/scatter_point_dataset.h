#ifndef SCATTER_POINT_DATASET_H_
#define SCATTER_POINT_DATASET_H_

#include <vector>

class ScatterPointDataset
{
public:
	ScatterPointDataset();
	~ScatterPointDataset();

	void Sample(int point_num, float left, float right, float bottom, float top);
	void DirectConstruct();

	bool is_structured_data;
	int w, h;

	std::vector< std::vector< float > > point_pos;
	std::vector< std::vector< float > > point_values;
	std::vector< float > weights;

	std::vector< std::vector< float > > original_point_pos;
	std::vector< std::vector< float > > original_pos_ranges;
	std::vector< std::vector< float > > original_point_values;
	std::vector< std::vector< float > > original_value_ranges;

private:
	void NormalizePosition(std::vector< std::vector< float > >& vec, std::vector< std::vector< float > >& ranges);
	void NormalizeValues(std::vector< std::vector< float > >& vec, std::vector< std::vector< float > >& ranges);
};

#endif