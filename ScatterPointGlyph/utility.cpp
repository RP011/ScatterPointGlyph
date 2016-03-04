#include "utility.h"
#include <vtkDelaunay2D.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkLine.h>
#include <vtkCellLocator.h>
#include <vtkSmartPointer.h>
#include <vtkDelaunay2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkCellArray.h>
#include <vtkCell.h>

#include "cnode.h"
#include "parallel_coordinate.h"
#include "tour_path_generator.h"

Utility::Utility() {
}

Utility::~Utility() {

}

void Utility::Sort(std::vector<int>& index_one, std::vector<int>& index_two) {
	for (int i = 0; i < index_one.size() - 1; ++i)
		for (int j = i + 1; j < index_one.size(); ++j)
			if (index_one[i] > index_one[j]){
				int temp_value = index_one[j];
				index_one[j] = index_one[i];
				index_one[i] = temp_value;

				int temp_index = index_two[i];
				index_two[i] = index_two[j];
				index_two[j] = temp_index;
			}
}

void Utility::Sort(std::vector<float>& index_one, std::vector<int>& index_two) {
	for (int i = 0; i < index_one.size() - 1; ++i)
		for (int j = i + 1; j < index_one.size(); ++j)
			if (index_one[i] > index_one[j]){
				float temp_value = index_one[j];
				index_one[j] = index_one[i];
				index_one[i] = temp_value;

				int temp_index = index_two[i];
				index_two[i] = index_two[j];
				index_two[j] = temp_index;
			}
}

void Utility::VtkTriangulation(std::vector< CNode* >& nodes, std::vector< std::vector< bool > >& connecting_status, float& min_edge_length) {
	connecting_status.resize(nodes.size());
	for (int i = 0; i < nodes.size(); ++i) {
		connecting_status[i].resize(nodes.size());
		connecting_status[i].assign(nodes.size(), false);
	}

	vtkSmartPointer< vtkPoints > points = vtkSmartPointer< vtkPoints >::New();
	for (int i = 0; i < nodes.size(); ++i) {
		points->InsertNextPoint(nodes[i]->center_pos[0], nodes[i]->center_pos[1], 0);
	}
	vtkSmartPointer< vtkPolyData > polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(points);
	vtkSmartPointer< vtkDelaunay2D > delaunay = vtkSmartPointer<vtkDelaunay2D>::New();
	delaunay->SetInputData(polydata);
	delaunay->SetTolerance(0.00001);
	delaunay->SetBoundingTriangulation(false);
	delaunay->Update();
	vtkPolyData* triangle_out = delaunay->GetOutput();

	min_edge_length = 0.0;
	int edge_count = 0;

	vtkIdTypeArray* idarray = triangle_out->GetPolys()->GetData();
	float min_dis = 1e10;
	for (int i = 0; i < triangle_out->GetNumberOfPolys(); ++i){
		vtkCell* cell = triangle_out->GetCell(i);
		int id1 = cell->GetPointId(0);
		int id2 = cell->GetPointId(1);
		int id3 = cell->GetPointId(2);
		connecting_status[id1][id2] = true;
		connecting_status[id2][id1] = true;
		connecting_status[id2][id3] = true;
		connecting_status[id3][id2] = true;
		connecting_status[id1][id3] = true;
		connecting_status[id3][id1] = true;

		float temp_dis;
		temp_dis = sqrt(pow(nodes[id1]->center_pos[0] - nodes[id2]->center_pos[0], 2)
			+ pow(nodes[id1]->center_pos[1] - nodes[id2]->center_pos[1], 2));
		if (temp_dis < min_dis) min_dis = temp_dis;

		temp_dis = sqrt(pow(nodes[id1]->center_pos[0] - nodes[id3]->center_pos[0], 2)
			+ pow(nodes[id1]->center_pos[1] - nodes[id3]->center_pos[1], 2));
		if (temp_dis < min_dis) min_dis = temp_dis;

		temp_dis = sqrt(pow(nodes[id3]->center_pos[0] - nodes[id2]->center_pos[0], 2)
			+ pow(nodes[id3]->center_pos[1] - nodes[id2]->center_pos[1], 2));
		if (temp_dis < min_dis) min_dis = temp_dis;
	}
	min_edge_length = min_dis;

#ifdef DEBUG_ON
	/*for (int i = 0; i < connecting_status.size(); ++i){
	bool is_connecting = false;
	for (int j = 0; j < connecting_status[i].size(); ++j) is_connecting = is_connecting || connecting_status[i][j];
	assert(is_connecting);
	}*/
#endif // DEBUG_ON
}

void Utility::GenerateAxisOrder(ParallelDataset* dataset, std::vector< int >& axis_order)
{
	axis_order.resize(dataset->axis_names.size());
	for (int i = 0; i < dataset->axis_names.size(); ++i) axis_order[i] = i;
	std::vector< std::vector< float > > corr_values;
	corr_values.resize(dataset->axis_names.size());
	for (int i = 0; i < dataset->axis_names.size(); ++i) {
		corr_values[i].resize(dataset->axis_names.size(), 0);
	}
	if (dataset->subset_records.size() == 1) {
		for (int i = 0; i < dataset->axis_names.size() - 1; ++i)
			for (int j = i + 1; j < dataset->axis_names.size(); ++j) {
				float temp_corr = 0;
				for (int k = 0; k < dataset->subset_records[0].size(); ++k)
					temp_corr += (dataset->subset_records[0][k]->values[i] - dataset->var_centers[0][i]) * (dataset->subset_records[0][k]->values[j] - dataset->var_centers[0][j]);
				corr_values[i][j] = temp_corr / (dataset->subset_records[0].size() * dataset->var_std_dev[0][i] * dataset->var_std_dev[0][j]);
				corr_values[i][j] = 1.0 - abs(corr_values[i][j]);
				corr_values[j][i] = corr_values[i][j];
			}
		TourPathGenerator::GenerateRoundPath(corr_values, axis_order);
	}
	else {
		std::vector< float > center_values;
		std::vector< float > std_dev_values;
		center_values.resize(dataset->axis_names.size(), 0);
		std_dev_values.resize(dataset->axis_names.size(), 0);
		for (int i = 0; i < dataset->axis_names.size(); ++i) {
			for (int j = 0; j < dataset->subset_records.size(); ++j)
				center_values[i] += dataset->var_centers[j][i];
			center_values[i] /= dataset->subset_records.size();
		}
		for (int i = 0; i < dataset->axis_names.size(); ++i) {
			for (int j = 0; j < dataset->subset_records.size(); ++j)
				std_dev_values[i] += pow(dataset->var_centers[j][i] - center_values[i], 2);
			std_dev_values[i] = sqrt(std_dev_values[i] / dataset->subset_records.size());
		}

		for (int i = 0; i < dataset->axis_names.size() - 1; ++i)
			for (int j = i + 1; j < dataset->axis_names.size(); ++j) {
				float temp_corr = 0;
				for (int k = 0; k < dataset->subset_records.size(); ++k)
					temp_corr += (dataset->var_centers[k][i] - center_values[i]) * (dataset->var_centers[k][j] - center_values[j]);
				corr_values[i][j] = temp_corr / (dataset->subset_records.size() * std_dev_values[i] * std_dev_values[j]);
				corr_values[i][j] = 1.0 - abs(corr_values[i][j]);
				corr_values[j][i] = corr_values[i][j];
			}
		TourPathGenerator::GenerateRoundPath(corr_values, axis_order);
	}
}

void Utility::GridConnection(std::vector< CNode*>& nodes, int w, int h, std::vector< std::vector< bool > >& connecting_status, float& min_edge_length)
{
    connecting_status.resize(nodes.size());
	for (int i = 0; i < nodes.size(); ++i) {
		connecting_status[i].resize(nodes.size());
		connecting_status[i].assign(nodes.size(), false);
	}
    for (int i = 0; i < h; i++) {
        int n1 = i * w;
        int n2 = (i + 1) * w;
        for (int j = 0; j < w; j++) {
            int p1 = n1 + j;
            int p2 = n1 + j + 1;
            int p3 = n2 + j;
            if (i < h - 1) {
                connecting_status[p1][p3] = true;
                connecting_status[p3][p1] = true;
            }
            if (j < w - 1) {
                connecting_status[p1][p2] = true;
                connecting_status[p2][p1] = true;
            }
        }
    }
}
