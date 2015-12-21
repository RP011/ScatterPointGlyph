#include "multi_label_processor.h"
#include <time.h>
#include <queue>
#include "./gco-v3.0/GCoptimization.h"
#include "./gco-v3.0/energy.h"

MultiLabelProcessor::MultiLabelProcessor() {
	max_radius_ = -1;
	sample_num_ = 0;
}

MultiLabelProcessor::~MultiLabelProcessor() {

}

void MultiLabelProcessor::SetData(std::vector< std::vector< float > >& pos, std::vector< std::vector< float > >& value, std::vector < float >& weights, std::vector< std::vector< bool > >& edges) {
	this->point_num_ = pos.size();
	this->point_pos_ = pos;
	this->point_value_ = value;
	this->var_weights_ = weights;
	this->edges_ = edges;

	this->sample_num_ = point_num_;

	this->edge_weights_.resize(edges.size());
	for (int i = 0; i < edges.size(); ++i) {
		this->edge_weights_[i].resize(edges[i].size());
		this->edge_weights_[i].assign(edges_[i].size(), 0);
	}
}

void MultiLabelProcessor::SetEdgeWeights(std::vector< std::vector< float > >& weights) {
	this->edge_weights_ = weights;
}

void MultiLabelProcessor::SetLabelEstimationRadius(float radius) {
	this->max_radius_ = radius;
}

void MultiLabelProcessor::SetSampleNumber(int num) {
	this->sample_num_ = num;
}

std::vector< int >& MultiLabelProcessor::GetResultLabel() {
	return result_label_;
}

void MultiLabelProcessor::UpdateEnergyCost() {
	point_un_.resize(point_num_);
	point_un_.assign(point_num_, 0);

	std::vector< int > point_edge_num;
	point_edge_num.resize(point_num_, 0);
	for (int i = 0; i < edges_.size() - 1; ++i)
		for (int j = i + 1; j < edges_.size(); ++j)
			if (edges_[i][j]) {
				point_un_[i] += edge_weights_[i][j];
				point_un_[j] += edge_weights_[i][j];
				point_edge_num[i]++;
				point_edge_num[j]++;
			}
	for (int i = 0; i < point_num_; ++i) {
		if (point_edge_num[i] != 0) point_un_[i] /= point_edge_num[i];
		else point_un_[i] = 1.0;
	}

	ExtractEstimatedModels();

	// extract cost
	int label_num = sample_num_;
	int site_num = point_num_;

	label_cost_.resize(label_num);
	label_cost_.assign(label_num, -1);

	smooth_cost_.resize(label_num);
	for (int i = 0; i < label_num; ++i) {
		smooth_cost_[i].resize(label_num);
		smooth_cost_[i].assign(label_num, -1);
	}

	data_cost_.resize(site_num);
	for (int i = 0; i < site_num; ++i) {
		data_cost_[i].resize(label_num);
		data_cost_[i].assign(label_num, -1);
	}

	std::vector< std::vector< float > > average_values;
	std::vector< float > average_un;
	average_values.resize(label_num);
	average_un.resize(label_num, 0);

	for (int i = 0; i < label_num; ++i) {
		int model_size = estimated_models_[i].size();

		average_values[i].resize(var_weights_.size(), 0);

		float center_x = 0, center_y = 0;
		for (int j = 0; j < model_size; ++j) {
			int site_index = estimated_models_[i][j];
			for (int k = 0; k < var_weights_.size(); ++k)
				average_values[i][k] += point_value_[site_index][k];
			center_x += point_pos_[site_index][0];
			center_y += point_pos_[site_index][1];
			average_un[i] += point_un_[site_index];
		}
		for (int k = 0; k < var_weights_.size(); ++k)
			average_values[i][k] /= model_size;
		center_x /= model_size;
		center_y /= model_size;
		average_un[i] /= model_size;

		float value_var = 0;
		for (int j = 0; j < model_size; ++j) {
			int site_index = estimated_models_[i][j];
			float value_dis = 0;
			for (int k = 0; k < var_weights_.size(); ++k)
				value_dis += abs(point_value_[site_index][k] - average_values[i][k]) * var_weights_[k] * 1;
			float pos_dis = sqrt(pow(point_pos_[site_index][0] - center_x, 2) + pow(point_pos_[site_index][1] - center_y, 2))/* / max_radius_*/;
			//if (pos_dis > 1.0) pos_dis = 1.0 + (pos_dis - 1.0) * 5;

			data_cost_[site_index][i] = data_dis_scale_ * value_dis + (1.0 - data_dis_scale_) * pos_dis;
			//data_cost_[site_index][i] *= average_un[i];
			//data_cost_[site_index][i] *= 10;

			value_var += pow(value_dis, 2);
		}
		value_var = sqrt(value_var / model_size) + 0.01;
		label_cost_[i] = (value_var + average_un[i]) * 500;
	}

	for (int i = 0; i < label_num; ++i){
		smooth_cost_[i][i] = 0;

		for (int j = i + 1; j < label_num; ++j) {
			smooth_cost_[i][j] = 0;
			for (int k = 0; k < var_weights_.size(); ++k)
				smooth_cost_[i][j] += abs(average_values[i][k] - average_values[j][k]) * var_weights_[k];
			smooth_cost_[i][j] += 0.1;
			smooth_cost_[j][i] = smooth_cost_[i][j];
		}
	}

	for (int i = 0; i < label_cost_.size(); ++i) {
		if (label_cost_[i] < 0) label_cost_[i] = 100.0;
	}
	for (int i = 0; i < data_cost_.size(); ++i)
		for (int j = 0; j < data_cost_[i].size(); ++j) {
			if (data_cost_[i][j] < 0) data_cost_[i][j] = 100.0;
		}
}

void MultiLabelProcessor::ExtractEstimatedModels() {
	// TODO: update the sample strategy
	// while (sample_num_ > point_num_ / 2) sample_num_ /= 2;

	std::vector< int > sample_center_index;
	if (sample_num_ != point_num_) {
		srand((unsigned int)time(0));
		std::vector< bool > is_selected;
		is_selected.resize(point_num_, false);
		for (int i = 0; i < sample_num_; ++i) {
			int temp_index;
			do {
				temp_index = (int)((float)rand() / RAND_MAX * (point_num_ - 1) + 0.499);
			} while (is_selected[temp_index]);
			sample_center_index.push_back(temp_index);
			is_selected[temp_index] = true;
		}
	} else {
		for (int i = 0; i < point_num_; ++i) sample_center_index.push_back(i);
	}

	this->estimated_models_.clear();

	point_dis_.resize(point_num_);
	for (int i = 0; i < point_num_; ++i) {
		point_dis_[i].resize(point_num_);
		point_dis_[i].assign(point_num_, 0);
	}
	for (int i = 0; i < point_num_ - 1; ++i)
		for (int j = i + 1; j < point_num_; ++j)
			if (edges_[i][j]) {
				point_dis_[i][j] = 0;
				/*for (int k = 0; k < var_weights_.size(); ++k)
					point_dis_[i][j] += data_dis_scale_ * abs(point_value_[i][k] - point_value_[j][k]) * var_weights_[k];*/
				point_dis_[i][j] += sqrt(pow(point_pos_[i][0] - point_pos_[j][0], 2) + pow(point_pos_[i][1] - point_pos_[j][1], 2));
				//point_dis_[i][j] *= edge_weights_[i][j];
				point_dis_[j][i] = point_dis_[i][j];
			} else {
				point_dis_[i][j] = 1e10;
				point_dis_[j][i] = point_dis_[i][j];
			}

	std::vector< float > node_distance;
	node_distance.resize(point_num_);
	std::vector< bool > is_reached;
	is_reached.resize(point_num_);

	for (int i = 0; i < sample_num_; ++i) {
		node_distance.assign(point_num_, 1e10);
		is_reached.assign(point_num_, false);
		node_distance[sample_center_index[i]] = 0;

		for (int j = 0; j < point_num_; ++j) {
			float min_dist = 1e20;
			int min_index = -1;
			for (int k = 0; k < point_num_; ++k)
				if (!is_reached[k] && node_distance[k] < min_dist && node_distance[k] < max_radius_ 
					&& abs(point_value_[k][0] - point_value_[sample_center_index[i]][0]) <= 0.3 * point_value_[sample_center_index[i]][0] + 0.2) {
					min_dist = node_distance[k];
					min_index = k;
				}
			if (min_index == -1) break;
			is_reached[min_index] = true;

			for (int k = 0; k < point_num_; ++k)
				if (!is_reached[k] && edges_[min_index][k]) {
					if (node_distance[k] > node_distance[min_index] + point_dis_[min_index][k]) node_distance[k] = node_distance[min_index] + point_dis_[min_index][k];
				}
		}

		std::vector< int > model;
		for (int j = 0; j < is_reached.size(); ++j)
			if (is_reached[j]) model.push_back(j);
		this->estimated_models_.push_back(model);
	}
}

void MultiLabelProcessor::GenerateCluster() {
	if (max_radius_ <= 0 || sample_num_ <= 0) return;

	UpdateEnergyCost();

	/*result_label_.resize(point_num_);
	result_label_.assign(point_num_, -1);
	for (int i = 0; i < estimated_models_.size(); ++i)
	for (int j = 0; j < estimated_models_[i].size(); ++j)
	result_label_[estimated_models_[i][j]] = i;
	return;*/

	try {
		int label_num = sample_num_;
		int site_num = point_pos_.size();

		// Step 1: construct class
		GCoptimizationGeneralGraph* gc = new GCoptimizationGeneralGraph(site_num, label_num);

		// Step 2: multi-label graph cut
		for (int i = 0; i < site_num - 1; ++i)
			for (int j = i + 1; j < site_num; ++j) {
				if (edges_[i][j]) gc->setNeighbors(i, j);
			}

		for (int i = 0; i < site_num; ++i) {
			for (int j = 0; j < label_num; ++j) {
				gc->setDataCost(i, j, (int)(data_cost_[i][j] * 1000));
			}
		}
		for (int i = 0; i < label_num; ++i)
			for (int j = 0; j < label_num; ++j) {
				gc->setSmoothCost(i, j, (int)(smooth_cost_[i][j] * 1000));
			}

		std::vector< int > temp_label_cost;
		temp_label_cost.resize(label_num);
		for (int i = 0; i < label_num; ++i) temp_label_cost[i] = (int)(label_cost_[i] * 1000);
		gc->setLabelCost(temp_label_cost.data());

		gc->expansion(2); // run expansion for 2 iterations. For swap use gc->swap(num_iterations);

		result_label_.resize(site_num);
		for (int i = 0; i < site_num; ++i) result_label_[i] = gc->whatLabel(i);

		delete gc;
	}
	catch (GCException e) {
		e.Report();
	}
}