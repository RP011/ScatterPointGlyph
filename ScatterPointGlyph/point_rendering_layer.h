#ifndef POINT_RENDERING_LAYER_H_
#define POINT_RENDERING_LAYER_H_

#include "basic_glyph_layer.h"
#include <vector>

class PointRenderingLayer : public BasicGlyphLayer
{
public:
	PointRenderingLayer();
	~PointRenderingLayer();

	void SetData(vtkPolyData* data);
	void SetClusterIndex(int cluster_count, std::vector< int >& point_index);
	void SetHighlightPointIndex(std::vector< int >& index);
	void SetPointValue(std::vector< std::vector< float > >& values);

private:
	std::vector< int > point_index_;

	void UpdatePointGlyph();
};

#endif