#include "tree_map_item.h"
#include <QtGui/QPen>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsSceneMouseEvent>

TreeMapItem::TreeMapItem() {

}

TreeMapItem::~TreeMapItem() {

}

void TreeMapItem::SetData(CNode* data) {
	root_ = data;

	this->item_map_.clear();
	int bottom = title_height;
	int item_width = left_margin;
	this->UpdateSize(root_, bottom, item_width);

	this->total_height = bottom;
	this->total_width = item_width + left_margin;

	this->prepareGeometryChange();
	this->update(QRectF(0, 0, total_width, total_height));
}

void TreeMapItem::UpdateSize(CNode* node, int& bottom, int& item_width) {
	if (node == NULL) return;

	this->item_map_.insert(std::map< int, CNode* >::value_type(node->id, node));

	int temp_left = item_width;
	// paint the child nodes
	if (node->type() == CNode::BRANCH && node->is_expanded) {
		CBranch* branch = dynamic_cast<CBranch*>(node);
		for (int i = 0; i < branch->linked_nodes.size(); ++i) {
			UpdateSize(branch->linked_nodes[i], bottom, item_width);
			if (i != branch->linked_nodes.size() - 1) item_width += item_margin;
		}
	}

	// paint the item glyph
	if (item_width - temp_left < item_size)  item_width += item_size;
	int bottom_y = node->level() * (item_size + transition_width) + item_size + title_height;
	if (bottom_y > bottom) bottom = bottom_y;
}

QSize TreeMapItem::GetSize() {
	return QSize(total_width, total_height);
}

QRectF TreeMapItem::boundingRect() const {
	return QRectF(0, 0, total_width, total_height);
}

void TreeMapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	item_pos_map_.clear();

	if (root_ == NULL) return;

	// paint title
	QPen title_pen;
	title_pen.setColor(Qt::black);
	title_pen.setWidth(2.0);
	painter->drawText(QRect(0, 0, this->total_width, title_height), Qt::AlignCenter, QString("Hierarchical Tree"));

	int item_width = left_margin;

	PaintItem(painter, root_, item_width);

	total_width = item_width;
}

void TreeMapItem::PaintItem(QPainter* painter, CNode* node, int& item_width) {
	if (node == NULL) return;

	int temp_left = item_width;
	// paint the child nodes
	std::vector< int > linked_pos;
	if (node->type() == CNode::BRANCH && node->is_expanded) {
		CBranch* branch = dynamic_cast<CBranch*>(node);
		for (int i = 0; i < branch->linked_nodes.size(); ++i) {
			float item_left = item_width;
			PaintItem(painter, branch->linked_nodes[i], item_width);
			linked_pos.push_back((item_left + item_width) / 2);
			if (i != branch->linked_nodes.size() - 1) {
				item_width += item_margin;
			}
		}
	}

	bool is_child_all_leaf = true;
	if (node->type() == CNode::BRANCH) {
		CBranch* branch = dynamic_cast<CBranch*>(node);
		for (int i = 0; i < branch->linked_nodes.size(); ++i) {
			if (branch->linked_nodes[i]->type() != CNode::LEAF) is_child_all_leaf = false;
			}
		}

	// paint the item glyph
	int topy, center_x, center_y;
	if (item_width - temp_left < item_size)  item_width += item_size;
	center_x = (item_width + temp_left) / 2;
	topy = node->level() * (item_size + transition_width) + left_margin + title_height;
	center_y = topy + 0.5 * item_size;

	// paint the radar glyph
	if (node->level() == 0) {
		QPen normal_pen;
		normal_pen.setColor(Qt::gray);
		normal_pen.setWidth(2.0);

		painter->setPen(normal_pen);
		painter->drawEllipse(center_x - item_size / 2, topy, item_size, item_size);
	} else {
		QPen axis_pen;
		axis_pen.setColor(QColor(220, 220, 220));
		axis_pen.setWidth(2.0);
		painter->setPen(axis_pen);

		std::vector< float > x_vec;
		std::vector< float > y_vec;

		for (int i = 0; i < node->average_values.size(); ++i) {
			float end_arc = i * 3.14159 * 2 / node->average_values.size();

			float temp_radius = item_size / 2 * 0.8;
			float x = temp_radius * cos(end_arc);
			float y = temp_radius * sin(end_arc);

			x_vec.push_back(x * node->average_values[i]);
			y_vec.push_back(y * node->average_values[i]);

			painter->drawLine(center_x, center_y, center_x + x, center_y + y);
		}

		axis_pen.setColor(QColor(128, 128, 128));
		painter->setPen(axis_pen);
		for (int i = 0; i < node->average_values.size(); ++i) {
			painter->drawLine(center_x + x_vec[i], center_y + y_vec[i], center_x + x_vec[(i + 1) % node->average_values.size()], center_y + y_vec[(i + 1) % node->average_values.size()]);
		}

		QPen normal_pen;
		normal_pen.setColor(Qt::gray);
		normal_pen.setWidth(2.0);
		painter->setPen(normal_pen);

		if (is_child_all_leaf) {
			painter->drawRoundedRect(QRectF(center_x - item_size / 2, topy, item_size, item_size), 2, 2);
		} else {
			painter->drawEllipse(center_x - item_size / 2, topy, item_size, item_size);
		}
	}

	QRectF item_rect = QRectF(center_x - item_size / 2, topy, item_size, item_size);
	this->item_pos_map_.insert(std::map< int, QRectF >::value_type(node->id, item_rect));
	if (node->is_highlighted) {
		painter->setPen(Qt::black);
		painter->drawRect(item_rect);
	}

	// paint linkage
	painter->setPen(Qt::red);
	int bottom_y = topy + item_size;
	int control_y1 = bottom_y + transition_width * 0.2;
	int control_y2 = bottom_y + transition_width * 0.5;
	int nexty = bottom_y + transition_width;
	for (int i = 0; i < linked_pos.size(); ++i) {
		QPainterPath temp_path;
		temp_path.moveTo(center_x, bottom_y);
		temp_path.cubicTo(center_x, control_y1, linked_pos[i], control_y2, linked_pos[i], nexty);
		painter->drawPath(temp_path);
	}
}

void TreeMapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	int x = event->pos().x();
	int y = event->pos().y();

	std::map< int, QRectF >::iterator iter = this->item_pos_map_.begin();
	int id_index = -1;
	while (iter != item_pos_map_.end()) {
		QRectF rect = iter->second;
		if (x > rect.left() && x < rect.right() && y > rect.top() && y < rect.bottom()) {
			id_index = iter->first;
			break;
		}
		iter++;
	}

	if (id_index == -1) return;

	if (event->button() == Qt::LeftButton) {
		std::map< int, CNode* >::iterator iter = item_map_.find(id_index);
		if (iter != item_map_.end()) {
			iter->second->is_highlighted = !iter->second->is_highlighted;
			emit NodeSelected(id_index);
			this->update();
			return;
		}
	} else if (event->button() == Qt::RightButton) {
		std::map< int, CNode* >::iterator iter = item_map_.find(id_index);
		if (iter != item_map_.end() && iter->second->type() == CNode::BRANCH) {
			bool is_all_leaf = true;
			CBranch* branch = dynamic_cast<CBranch*>(iter->second);
			for (int i = 0; i < branch->linked_nodes.size(); ++i)
				if (branch->linked_nodes[i]->type() == CNode::BRANCH) {
					is_all_leaf = false;
					break;
				}
			if (!is_all_leaf) {
				iter->second->is_expanded = !iter->second->is_expanded;
				emit NodeSelected(id_index);
			}
			this->item_map_.clear();
			int bottom = 0;
			int item_width = 0;
			this->UpdateSize(root_, bottom, item_width);

			this->total_height = bottom;
			this->total_width = item_width + left_margin;

			this->prepareGeometryChange();
			this->update(QRectF(0, 0, total_width, total_height));

			return;
		}
	}
}