#ifndef HIER_CLUSTER_GLYPH_LAYER_H_
#define HIER_CLUSTER_GLYPH_LAYER_H_

#include "basic_glyph_layer.h"

class ScatterPointDataset;

class ClusterGlyphLayer : public BasicGlyphLayer
{
public:
	ClusterGlyphLayer();
	~ClusterGlyphLayer();

	void SetData(ScatterPointDataset* data);
	void SetRadiusRange(float maxR, float minR);

	void SetClusterIndex(int cluster_count, std::vector< int >& point_index);

	void ClearGlyph();

private:
	ScatterPointDataset* dataset_;
	float radius_range_[2];

	void InitGlyphActor();
};

#endif