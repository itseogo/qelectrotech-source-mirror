/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "layersdialog.h"

#include <QApplication>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsObject>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>
#include <dataBase/projectdatabase.h>
#include <diagram.h>
#include <diagramview.h>
#include <drawinglayers/drawinglayers.h>
#include <projectview.h>
#include <qet.h>
#include <qetdiagrameditor.h>
#include <qetgraphicsitem/conductortextitem.h>
#include <qetgraphicsitem/crossrefitem.h>
#include <qetgraphicsitem/diagramimageitem.h>
#include <qetgraphicsitem/diagramtextitem.h>
#include <qetgraphicsitem/dynamicelementtextitem.h>
#include <qetgraphicsitem/element.h>
#include <qetgraphicsitem/independenttextitem.h>
#include <qetgraphicsitem/qetshapeitem.h>
#include <qetgraphicsitem/structureboxitem.h>
#include <qetmessagebox.h>
#include <qetproject.h>
#include <shownkinds.h>
#include <undocommand/drawinglayerscommand.h>

namespace
{
	void paintColorButton(QPushButton* button, const QColor& color)
	{
		const QString obj = button->objectName().isEmpty() ?
								QStringLiteral("layerColorBtn") :
								button->objectName();
		if (button->objectName().isEmpty()) button->setObjectName(obj);

		if (color.isValid())
		{
			const QString hex = color.name().toUpper();
			const double  lum = 0.299 * color.red() + 0.587 * color.green()
								+ 0.114 * color.blue();
			const QString text_color = lum > 140 ? QStringLiteral("#000000") :
												   QStringLiteral("#FFFFFF");
			button->setStyleSheet(
				QStringLiteral(
					"#%1 { background-color: %2; color: %3; "
					"font-weight: bold; border: 1px solid #555555; "
					"border-radius: 2px; padding: 2px 6px; }")
					.arg(obj, hex, text_color));
			button->setIcon(QIcon());
			button->setText(hex);
			button->setToolTip(
				hex + QStringLiteral(" (") + QObject::tr("Right-click to reset")
				+ QStringLiteral(")"));
		}
		else
		{
			button->setStyleSheet(
				QStringLiteral(
					"#%1 { background-color: #000000; color: #FFFFFF; "
					"font-weight: bold; border: 1px solid #333333; "
					"border-radius: "
					"2px; padding: 2px 6px; }")
					.arg(obj));
			button->setIcon(QIcon());
			button->setText(QObject::tr("Default"));
			button->setToolTip(
				QObject::tr("Individual element color (default: #000000)"));
		}
	}
} // namespace

using DrawingLayers::Set;

LayersDialog::LayersDialog(QETProject* project, QWidget* parent) :
	QDialog(parent),
	m_project(project),
	m_layers(new QTableWidget(0, 6, this)),
	m_groups(new QListWidget(this)),
	m_members(new QListWidget(this)),
	m_summary(new QLabel(this))
{
	setWindowTitle(tr("Layers"));
	resize(780, 680);
	auto* layout = new QVBoxLayout(this);
	m_summary->setWordWrap(true);
	m_summary->setObjectName(QStringLiteral("layersSummary"));
	m_summary->setText(
		tr("A layer holds lines, shapes, free texts and pictures. "
		   "What is on a layer switched off is hidden on the screen, in print "
		   "and in exports."));
	layout->addWidget(m_summary);

	auto* layers_box	= new QGroupBox(tr("Layers"), this);
	auto* layers_layout = new QHBoxLayout(layers_box);
	auto* layers_vbox	= new QVBoxLayout();

	auto* filter_layout = new QHBoxLayout();
	auto* filter_label	= new QLabel(tr("Filter:"), this);
	m_filter_edit		= new QLineEdit(this);
	m_filter_edit->setObjectName(QStringLiteral("layerFilterEdit"));
	m_filter_edit->setPlaceholderText(tr("Search by name or description..."));
	m_filter_edit->setClearButtonEnabled(true);
	filter_layout->addWidget(filter_label);
	filter_layout->addWidget(m_filter_edit);
	layers_vbox->addLayout(filter_layout);

	m_layers->setObjectName(QStringLiteral("layersTable"));
	m_layers->setColumnCount(6);
	m_layers->setHorizontalHeaderLabels(
		{tr("Layer"),
		 tr("Description"),
		 tr("Color"),
		 tr("Shown"),
		 tr("Print"),
		 tr("Locked")});
	m_layers->verticalHeader()->setVisible(false);
	m_layers->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_layers->setSelectionMode(QAbstractItemView::SingleSelection);
	m_layers->horizontalHeader()->setSectionResizeMode(
		0,
		QHeaderView::ResizeToContents);
	m_layers->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_layers->horizontalHeader()->setSectionResizeMode(
		2,
		QHeaderView::ResizeToContents);
	m_layers->horizontalHeader()->setSectionResizeMode(
		3,
		QHeaderView::ResizeToContents);
	m_layers->horizontalHeader()->setSectionResizeMode(
		4,
		QHeaderView::ResizeToContents);
	m_layers->horizontalHeader()->setSectionResizeMode(
		5,
		QHeaderView::ResizeToContents);
	layers_vbox->addWidget(m_layers, 1);
	layers_layout->addLayout(layers_vbox, 1);

	auto*	   layer_buttons = new QVBoxLayout();
	const auto button =
		[this](QVBoxLayout* column, const QString& text, const char* name)
	{
		auto* b = new QPushButton(text, this);
		b->setObjectName(QLatin1String(name));
		b->setAutoDefault(false);
		column->addWidget(b);
		return b;
	};
	auto* add	 = button(layer_buttons, tr("Add..."), "layerAdd");
	m_rename_btn = button(layer_buttons, tr("Rename..."), "layerRename");
	m_remove_btn = button(layer_buttons, tr("Remove"), "layerRemove");
	m_put_btn =
		button(layer_buttons, tr("Put the selection on it"), "layerPut");
	m_take_btn =
		button(layer_buttons, tr("Take the selection off"), "layerTake");
	auto* import_btn = button(layer_buttons, tr("Import..."), "layerImport");
	auto* export_btn = button(layer_buttons, tr("Export..."), "layerExport");
	auto* load_standards_btn =
		button(layer_buttons, tr("Load standard layers"), "layerLoadStandards");
	layer_buttons->addStretch();
	layers_layout->addLayout(layer_buttons);
	layout->addWidget(layers_box, 2);

	m_items_box = new QGroupBox(tr("Elements on selected layer"), this);
	auto* items_layout = new QVBoxLayout(m_items_box);
	m_layer_items	   = new QTableWidget(0, 3, this);
	m_layer_items->setObjectName(QStringLiteral("layerItemsTable"));
	m_layer_items->setHorizontalHeaderLabels(
		{tr("Folio"), tr("Type"), tr("Element / Text")});
	m_layer_items->verticalHeader()->setVisible(false);
	m_layer_items->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_layer_items->setSelectionMode(QAbstractItemView::SingleSelection);
	m_layer_items->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_layer_items->setContextMenuPolicy(Qt::CustomContextMenu);
	m_layer_items->horizontalHeader()->setSectionResizeMode(
		0,
		QHeaderView::ResizeToContents);
	m_layer_items->horizontalHeader()->setSectionResizeMode(
		1,
		QHeaderView::ResizeToContents);
	m_layer_items->horizontalHeader()->setSectionResizeMode(
		2,
		QHeaderView::Stretch);
	items_layout->addWidget(m_layer_items);
	layout->addWidget(m_items_box, 2);

	auto* groups_container = new QWidget(this);
	auto* groups_vbox	   = new QVBoxLayout(groups_container);
	groups_vbox->setContentsMargins(0, 0, 0, 0);
	groups_vbox->setSpacing(4);

	m_groups_toggle = new QToolButton(this);
	m_groups_toggle->setObjectName(QStringLiteral("layerGroupsToggle"));
	m_groups_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_groups_toggle->setArrowType(Qt::RightArrow);
	m_groups_toggle->setText(tr("Layer groups: the layers to show together"));
	m_groups_toggle->setCheckable(true);
	m_groups_toggle->setChecked(false);
	m_groups_toggle->setAutoRaise(true);
	groups_vbox->addWidget(m_groups_toggle, 0, Qt::AlignLeft);

	m_groups_content = new QWidget(this);
	m_groups_content->setObjectName(QStringLiteral("layerGroupsContent"));
	auto* groups_layout = new QHBoxLayout(m_groups_content);
	groups_layout->setContentsMargins(0, 0, 0, 0);
	m_groups->setObjectName(QStringLiteral("layerGroups"));
	m_members->setObjectName(QStringLiteral("layerGroupMembers"));
	groups_layout->addWidget(m_groups, 1);
	groups_layout->addWidget(m_members, 1);
	auto* group_buttons = new QVBoxLayout();
	auto* add_group		= button(group_buttons, tr("Add..."), "groupAdd");
	auto* remove_group	= button(group_buttons, tr("Remove"), "groupRemove");
	auto* show_group =
		button(group_buttons, tr("Show this group"), "groupShow");
	group_buttons->addStretch();
	groups_layout->addLayout(group_buttons);
	m_groups_content->setVisible(false);
	groups_vbox->addWidget(m_groups_content);
	layout->addWidget(groups_container, 0);

	connect(
		m_groups_toggle,
		&QToolButton::toggled,
		this,
		[this](bool expanded)
		{
			m_groups_toggle->setArrowType(
				expanded ? Qt::DownArrow : Qt::RightArrow);
			m_groups_content->setVisible(expanded);
		});

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);

	connect(
		m_filter_edit,
		&QLineEdit::textChanged,
		this,
		&LayersDialog::filterLayers);

	connect(
		load_standards_btn,
		&QPushButton::clicked,
		this,
		&LayersDialog::loadStandardLayers);

	connect(
		import_btn,
		&QPushButton::clicked,
		this,
		[this]()
		{
			const QString file = QFileDialog::getOpenFileName(
				this,
				tr("Import layers"),
				QString(),
				tr("Layer files (*.csv *.qetlayers *.xml);;CSV files "
				   "(*.csv);;XML files (*.qetlayers *.xml)"));
			if (! file.isEmpty()) importLayers(file);
		});
	connect(
		export_btn,
		&QPushButton::clicked,
		this,
		[this]()
		{
			const QString file = QFileDialog::getSaveFileName(
				this,
				tr("Export layers"),
				QStringLiteral("layers.csv"),
				tr("CSV files (*.csv);;XML files (*.qetlayers *.xml)"));
			if (! file.isEmpty()) exportLayers(file);
		});

	connect(
		add,
		&QPushButton::clicked,
		this,
		[this]()
		{
			const QString name = QInputDialog::getText(
				this,
				tr("Add a layer"),
				tr("Name of the layer:"));
			if (! name.trimmed().isEmpty() && ! addLayer(name.trimmed()))
				QET::QetMessageBox::warning(
					this,
					tr("Layers"),
					tr("There already is a layer of this name."));
		});
	connect(
		m_rename_btn,
		&QPushButton::clicked,
		this,
		[this]()
		{
			const QString current = currentLayer();
			if (current.isEmpty()) return;
			const QString name = QInputDialog::getText(
				this,
				tr("Rename the layer"),
				tr("Name of the layer:"),
				QLineEdit::Normal,
				current);
			if (! name.trimmed().isEmpty() && name.trimmed() != current
				&& ! renameLayer(current, name.trimmed()))
				QET::QetMessageBox::warning(
					this,
					tr("Layers"),
					tr("There already is a layer of this name."));
		});
	connect(
		m_remove_btn,
		&QPushButton::clicked,
		this,
		[this]()
		{
			if (! currentLayer().isEmpty()) removeLayer(currentLayer());
		});
	connect(
		m_put_btn,
		&QPushButton::clicked,
		this,
		[this]()
		{
			if (! currentLayer().isEmpty()) putSelectionOnLayer(currentLayer());
		});
	connect(
		m_take_btn,
		&QPushButton::clicked,
		this,
		[this]() { putSelectionOnLayer(QString()); });
	connect(
		m_layers,
		&QTableWidget::itemSelectionChanged,
		this,
		[this]()
		{
			updateButtonStates();
			showLayerItems();
		});
	connect(
		m_layer_items,
		&QTableWidget::cellDoubleClicked,
		this,
		[this](int row, int /*column*/) { navigateToLayerItem(row); });
	connect(
		m_layer_items,
		&QWidget::customContextMenuRequested,
		this,
		[this](const QPoint& pos)
		{
			auto* item = m_layer_items->itemAt(pos);
			if (! item) return;
			const int row = item->row();
			m_layer_items->selectRow(row);
			QMenu menu(this);
			auto* show_action = menu.addAction(tr("Show"));
			if (menu.exec(m_layer_items->viewport()->mapToGlobal(pos))
				== show_action)
			{
				navigateToLayerItem(row);
			}
		});
	connect(
		m_layers,
		&QTableWidget::itemChanged,
		this,
		[this](QTableWidgetItem* item)
		{
			if (m_refreshing) return;
			auto* name_item = m_layers->item(item->row(), 0);
			if (! name_item) return;
			const QVariant data = name_item->data(Qt::UserRole);
			const QString  layer_name =
				data.isValid() ? data.toString() : name_item->text();

			if (item->column() == 1)
			{
				setLayerDescription(layer_name, item->text().trimmed());
			}
			else if (item->column() == 3)
			{
				setLayerVisible(layer_name, item->checkState() == Qt::Checked);
			}
			else if (item->column() == 4)
			{
				setLayerPrintable(
					layer_name,
					item->checkState() == Qt::Checked);
			}
			else if (item->column() == 5)
			{
				setLayerLocked(layer_name, item->checkState() == Qt::Checked);
			}
		});
	connect(
		add_group,
		&QPushButton::clicked,
		this,
		[this]()
		{
			const QString name = QInputDialog::getText(
				this,
				tr("Add a layer group"),
				tr("Name of the group:"));
			if (! name.trimmed().isEmpty() && ! addGroup(name.trimmed()))
				QET::QetMessageBox::warning(
					this,
					tr("Layers"),
					tr("There already is a group of this name."));
		});
	connect(
		remove_group,
		&QPushButton::clicked,
		this,
		[this]()
		{
			if (! currentGroup().isEmpty()) removeGroup(currentGroup());
		});
	connect(
		show_group,
		&QPushButton::clicked,
		this,
		[this]()
		{
			if (! currentGroup().isEmpty()) showGroup(currentGroup());
		});
	connect(
		m_groups,
		&QListWidget::currentRowChanged,
		this,
		&LayersDialog::showGroupMembers);
	connect(
		m_members,
		&QListWidget::itemChanged,
		this,
		[this]()
		{
			if (m_refreshing || currentGroup().isEmpty()) return;
			QStringList layers;
			for (int i = 0; i < m_members->count(); ++i)
			{
				if (m_members->item(i)->checkState() == Qt::Checked)
				{
					const QVariant data =
						m_members->item(i)->data(Qt::UserRole);
					layers
						<< (data.isValid() ? data.toString() :
											 m_members->item(i)->text());
				}
			}
			setGroupLayers(currentGroup(), layers);
		});
	if (project)
	{
		connect(project, &QObject::destroyed, this, &QDialog::close);
		connect(
			project->undoStack(),
			&QUndoStack::indexChanged,
			this,
			&LayersDialog::refresh);
	}
	QET::trackDialogGeometry(this);
	refresh();
}

/**
	@brief LayersDialog::buildFolios
	Renaming or removing a layer concerns what is on it on every folio.
*/
void LayersDialog::buildFolios()
{
	if (m_project && m_project->unloadedFolioCount())
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_project->loadFolios();
		QApplication::restoreOverrideCursor();
	}
}

void LayersDialog::updateButtonStates()
{
	const QString current		= currentLayer();
	const bool	  has_selection = ! current.isEmpty();
	const bool	  fixed			= DrawingLayers::isFixed(current);
	const bool	  can_put = has_selection
							&& (! fixed
								|| current
									   == DrawingLayers::fixedKindName(
										   DrawingLayers::FixedKind::Graphics)
								|| current
									   == DrawingLayers::fixedKindName(
										   DrawingLayers::FixedKind::Pictures));

	if (m_rename_btn) m_rename_btn->setEnabled(has_selection && ! fixed);
	if (m_remove_btn) m_remove_btn->setEnabled(has_selection && ! fixed);
	if (m_put_btn) m_put_btn->setEnabled(can_put);
}

void LayersDialog::filterLayers(const QString& filter_text)
{
	const QString text = filter_text.trimmed();
	for (int row = 0; row < m_layers->rowCount(); ++row)
	{
		if (text.isEmpty())
		{
			m_layers->setRowHidden(row, false);
			continue;
		}
		auto*		  name_item = m_layers->item(row, 0);
		auto*		  desc_item = m_layers->item(row, 1);
		const QString name		= name_item ? name_item->text() : QString();
		const QString desc		= desc_item ? desc_item->text() : QString();
		const bool	  match		= name.contains(text, Qt::CaseInsensitive)
								  || desc.contains(text, Qt::CaseInsensitive);
		m_layers->setRowHidden(row, ! match);
	}
}

QString LayersDialog::currentLayer() const
{
	const int row = m_layers->currentRow();
	if (row < 0 || ! m_layers->item(row, 0)) return QString();
	const QVariant data = m_layers->item(row, 0)->data(Qt::UserRole);
	return data.isValid() ? data.toString() : m_layers->item(row, 0)->text();
}

QString LayersDialog::currentGroup() const
{
	return m_groups->currentItem() ? m_groups->currentItem()->text() :
									 QString();
}

void LayersDialog::refresh()
{
	if (! m_project) return;
	m_refreshing		= true;
	const QString layer = currentLayer(), group = currentGroup();
	const Set&	  set = m_project->drawingLayers();
	m_layers->setRowCount(int(set.layers.size()));
	for (int row = 0; row < set.layers.size(); ++row)
	{
		const auto& l = set.layers.at(row);

		// Col 0: Name
		auto* name_item = new QTableWidgetItem(l.name);
		name_item->setData(Qt::UserRole, l.name);
		name_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		m_layers->setItem(row, 0, name_item);

		// Col 1: Description
		auto* desc_item = new QTableWidgetItem(l.description);
		desc_item->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
		m_layers->setItem(row, 1, desc_item);

		// Col 2: Color
		auto* color_btn = new QPushButton(this);
		color_btn->setObjectName(
			QStringLiteral("layerColorBtn_%1").arg(l.name));
		paintColorButton(color_btn, l.color);
		color_btn->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(
			color_btn,
			&QPushButton::clicked,
			this,
			[this, layer_name = l.name, color_btn]()
			{ chooseLayerColor(layer_name, color_btn); });
		connect(
			color_btn,
			&QPushButton::customContextMenuRequested,
			this,
			[this, layer_name = l.name, color_btn](const QPoint& pos)
			{
				QMenu menu;
				auto* reset_action =
					menu.addAction(tr("Reset to individual color"));
				auto* choose_action = menu.addAction(tr("Choose color..."));
				auto* selected		= menu.exec(color_btn->mapToGlobal(pos));
				if (selected == reset_action)
				{
					setLayerColor(layer_name, QColor());
				}
				else if (selected == choose_action)
				{
					chooseLayerColor(layer_name, color_btn);
				}
			});
		m_layers->setCellWidget(row, 2, color_btn);

		// Col 3: Shown
		auto* shown = new QTableWidgetItem();
		shown->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		shown->setCheckState(l.visible ? Qt::Checked : Qt::Unchecked);
		m_layers->setItem(row, 3, shown);

		// Col 4: Print
		auto* printable = new QTableWidgetItem();
		printable->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		printable->setCheckState(l.printable ? Qt::Checked : Qt::Unchecked);
		m_layers->setItem(row, 4, printable);

		// Col 5: Locked
		auto* locked = new QTableWidgetItem();
		locked->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		locked->setCheckState(l.locked ? Qt::Checked : Qt::Unchecked);
		m_layers->setItem(row, 5, locked);

		if (l.name == layer) m_layers->setCurrentCell(row, 0);
	}
	m_groups->clear();
	for (const auto& g : set.groups)
	{
		m_groups->addItem(g.name);
		if (g.name == group) m_groups->setCurrentRow(m_groups->count() - 1);
	}
	m_refreshing = false;
	showGroupMembers();
	updateButtonStates();
	showLayerItems();
	if (m_filter_edit && ! m_filter_edit->text().isEmpty())
	{
		filterLayers(m_filter_edit->text());
	}
}

void LayersDialog::showLayerItems()
{
	if (! m_layer_items || ! m_items_box) return;
	m_layer_items->setRowCount(0);
	m_item_entries.clear();

	const QString layer = currentLayer();
	if (! m_project || layer.isEmpty())
	{
		m_items_box->setTitle(tr("Elements on selected layer"));
		return;
	}

	const DrawingLayers::FixedKind fk	  = DrawingLayers::fixedKindOf(layer);
	const QString				   desc	  = m_project->drawingLayers().description(layer);
	const QString				   header = desc.isEmpty() ?
												layer :
												QStringLiteral("%1 - %2").arg(layer, desc);

	auto addRow = [this](
					  Diagram*		   folio,
					  QGraphicsObject* obj,
					  const QString&   folio_label,
					  const QString&   type_label,
					  const QString&   detail)
	{
		const int row = m_layer_items->rowCount();
		m_layer_items->insertRow(row);
		m_layer_items->setItem(row, 0, new QTableWidgetItem(folio_label));
		m_layer_items->setItem(row, 1, new QTableWidgetItem(type_label));
		m_layer_items->setItem(row, 2, new QTableWidgetItem(detail));
		m_item_entries.append({folio, obj});
	};

	int folio_index = 0;
	for (Diagram* folio : m_project->diagrams())
	{
		++folio_index;
		const QString folio_title = folio->title().trimmed();
		const QString folio_label =
			folio_title.isEmpty() ?
				tr("Folio %1").arg(folio_index) :
				QStringLiteral("%1 - %2").arg(folio_index).arg(folio_title);

		if (fk == DrawingLayers::FixedKind::FrameAndTitleBlock)
		{
			addRow(
				folio,
				nullptr,
				folio_label,
				tr("Frame and title block"),
				folio_title.isEmpty() ? tr("Sheet border") : folio_title);
			continue;
		}

		for (QGraphicsItem* item : folio->items())
		{
			bool		  matched		 = false;
			const QString explicit_layer = DrawingLayers::layerOf(item);

			if (! explicit_layer.isEmpty())
			{
				if (explicit_layer == layer
					|| (fk != DrawingLayers::FixedKind::None
						&& DrawingLayers::fixedKindOf(explicit_layer) == fk))
				{
					matched = true;
				}
			}
			else
			{
				const QVariant val = item->data(ShownKinds::data_key);
				if (val.isValid())
				{
					const int kind = val.toInt();
					if ((kind == ShownKinds::WireNumbers
						 && fk == DrawingLayers::FixedKind::WireNumbers)
						|| (kind == ShownKinds::SymbolTexts
							&& fk == DrawingLayers::FixedKind::SymbolTexts)
						|| (kind == ShownKinds::CrossReferences
							&& fk == DrawingLayers::FixedKind::CrossReferences))
					{
						matched = true;
					}
				}
			}

			if (! matched) continue;

			QString type_label;
			QString detail;

			if (auto* box = qgraphicsitem_cast<StructureBoxItem*>(item))
			{
				type_label = tr("Structure box");
				detail	   = box->structureId();
				if (detail.isEmpty()) detail = tr("Structure box");
			}
			else if (auto* shape = qgraphicsitem_cast<QetShapeItem*>(item))
			{
				switch (shape->shapeType())
				{
				case QetShapeItem::Line: type_label = tr("Line"); break;
				case QetShapeItem::Rectangle:
					type_label = tr("Rectangle");
					break;
				case QetShapeItem::Ellipse: type_label = tr("Ellipse"); break;
				case QetShapeItem::Polygon: type_label = tr("Polygon"); break;
				case QetShapeItem::Path: type_label = tr("Path"); break;
				default: type_label = tr("Shape"); break;
				}
				const QPointF pos = shape->scenePos();
				detail = QStringLiteral("(%1, %2)")
							 .arg(qRound(pos.x()))
							 .arg(qRound(pos.y()));
			}
			else if (auto* text = qgraphicsitem_cast<IndependentTextItem*>(item))
			{
				type_label = tr("Free text");
				detail	   = text->toPlainText().simplified();
			}
			else if (auto* img = qgraphicsitem_cast<DiagramImageItem*>(item))
			{
				type_label = tr("Picture");
				const QPixmap pm = img->pixmap();
				detail = QStringLiteral("%1 x %2 px")
							 .arg(pm.width())
							 .arg(pm.height());
			}
			else if (
				auto* dyn = qgraphicsitem_cast<DynamicElementTextItem*>(item))
			{
				type_label		  = tr("Symbol text");
				const QString txt = dyn->toPlainText().simplified();
				QString		  parent_label;
				if (Element* el = dyn->parentElement())
				{
					parent_label =
						el->elementInformations()[QStringLiteral("label")]
							.toString()
							.trimmed();
				}
				if (! parent_label.isEmpty() && parent_label != txt)
					detail = QStringLiteral("%1: %2").arg(parent_label, txt);
				else
					detail = txt;
			}
			else if (auto* ct = qgraphicsitem_cast<ConductorTextItem*>(item))
			{
				type_label = tr("Wire number");
				detail	   = ct->toPlainText().simplified();
			}
			else if (auto* xref = qgraphicsitem_cast<CrossRefItem*>(item))
			{
				type_label = tr("Cross-reference");
				Element* el = qgraphicsitem_cast<Element*>(xref->parentItem());
				if (! el)
				{
					if (auto* dyn_parent =
							qgraphicsitem_cast<DynamicElementTextItem*>(
								xref->parentItem()))
						el = dyn_parent->parentElement();
				}
				if (el)
				{
					detail = el->elementInformations()[QStringLiteral("label")]
								 .toString()
								 .trimmed();
				}
				if (detail.isEmpty()) detail = tr("Cross-reference");
			}
			else if (auto* el = qgraphicsitem_cast<Element*>(item))
			{
				type_label = tr("Symbol");
				detail = el->elementInformations()[QStringLiteral("label")]
							 .toString()
							 .trimmed();
				if (detail.isEmpty()) detail = el->name();
			}
			else if (auto* dt = dynamic_cast<DiagramTextItem*>(item))
			{
				type_label = tr("Text");
				detail	   = dt->toPlainText().simplified();
			}
			else
			{
				type_label = tr("Element");
			}

			addRow(
				folio,
				item->toGraphicsObject(),
				folio_label,
				type_label,
				detail);
		}
	}

	m_items_box->setTitle(
		tr("Elements on layer \"%1\" (%2)")
			.arg(header, QString::number(m_layer_items->rowCount())));
}

void LayersDialog::navigateToLayerItem(int row)
{
	if (row < 0 || row >= m_item_entries.size()) return;
	const LayerItemEntry& entry = m_item_entries.at(row);
	if (! entry.folio) return;

	if (auto* editor = qobject_cast<QETDiagramEditor*>(parentWidget()))
	{
		if (ProjectView* view = editor->currentProjectView())
		{
			view->showDiagram(entry.folio);
			if (entry.item && entry.item->scene() == entry.folio)
			{
				entry.folio->clearSelection();
				if (entry.item->flags() & QGraphicsItem::ItemIsSelectable)
					entry.item->setSelected(true);
				if (DiagramView* dv = view->currentDiagram())
					dv->centerOn(entry.item);
			}
		}
	}
}

void LayersDialog::showGroupMembers()
{
	if (m_refreshing || ! m_project) return;
	m_refreshing = true;
	m_members->clear();
	const QString name = currentGroup();
	if (! name.isEmpty())
	{
		QStringList in_group;
		for (const auto& g : m_project->drawingLayers().groups)
			if (g.name == name) in_group = g.layers;
		for (const auto& layer : m_project->drawingLayers().layers)
		{
			const QString display_name =
				layer.isFixed() ?
					DrawingLayers::fixedKindDisplayName(layer.fixed_kind) :
					layer.name;
			auto* item = new QListWidgetItem(display_name, m_members);
			item->setData(Qt::UserRole, layer.name);
			item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
			item->setCheckState(
				in_group.contains(layer.name) ? Qt::Checked : Qt::Unchecked);
		}
	}
	m_refreshing = false;
}

bool LayersDialog::addLayer(const QString& name, const QString& description)
{
	if (! m_project || name.isEmpty() || DrawingLayers::isFixed(name)
		|| m_project->drawingLayers().contains(name))
		return false;
	Set set = m_project->drawingLayers();
	set.layers.append(
		{name, description, true, true, false, DrawingLayers::FixedKind::None});
	m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
	return true;
}

bool LayersDialog::renameLayer(const QString& name, const QString& new_name)
{
	if (! m_project || new_name.isEmpty() || DrawingLayers::isFixed(name)
		|| DrawingLayers::isFixed(new_name)
		|| m_project->drawingLayers().contains(new_name))
		return false;
	buildFolios();
	Set set = m_project->drawingLayers();
	for (auto& layer : set.layers)
		if (layer.name == name) layer.name = new_name;
	for (auto& group : set.groups)
		group.layers.replaceInStrings(
			QRegularExpression(
				QStringLiteral("^") + QRegularExpression::escape(name)
				+ QStringLiteral("$")),
			new_name);
	QHash<QGraphicsObject*, QString> items;
	for (Diagram* folio : m_project->diagrams())
		for (QGraphicsItem* item : folio->items())
			if (DrawingLayers::layerOf(item) == name
				&& item->toGraphicsObject())
				items.insert(item->toGraphicsObject(), new_name);
	m_project->undoStack()->push(
		new DrawingLayersCommand(m_project, set, items));
	return true;
}

/**
	@brief LayersDialog::removeLayer
	What is on it stays on the folios, on no layer.
*/
void LayersDialog::removeLayer(const QString& name)
{
	if (! m_project || DrawingLayers::isFixed(name)) return;
	buildFolios();
	Set set = m_project->drawingLayers();
	set.layers.erase(
		std::remove_if(
			set.layers.begin(),
			set.layers.end(),
			[&name](const DrawingLayers::Layer& layer)
			{ return layer.name == name; }),
		set.layers.end());
	for (auto& group : set.groups) group.layers.removeAll(name);
	QHash<QGraphicsObject*, QString> items;
	for (Diagram* folio : m_project->diagrams())
		for (QGraphicsItem* item : folio->items())
			if (DrawingLayers::layerOf(item) == name
				&& item->toGraphicsObject())
				items.insert(item->toGraphicsObject(), QString());
	m_project->undoStack()->push(
		new DrawingLayersCommand(m_project, set, items));
}

void LayersDialog::setLayerVisible(const QString& name, bool visible)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	for (auto& layer : set.layers)
		if (layer.name == name) layer.visible = visible;
	if (set != m_project->drawingLayers())
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

void LayersDialog::setLayerPrintable(const QString& name, bool printable)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	for (auto& layer : set.layers)
		if (layer.name == name) layer.printable = printable;
	if (set != m_project->drawingLayers())
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

void LayersDialog::setLayerLocked(const QString& name, bool locked)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	for (auto& layer : set.layers)
		if (layer.name == name) layer.locked = locked;
	if (set != m_project->drawingLayers())
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

void LayersDialog::setLayerColor(const QString& name, const QColor& color)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	set.setColor(name, color);
	if (set != m_project->drawingLayers())
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

void LayersDialog::chooseLayerColor(const QString& name, QWidget* /*button*/)
{
	if (! m_project) return;
	const QColor current_color = m_project->drawingLayers().color(name);
	const QColor chosen		   = QColorDialog::getColor(
		current_color.isValid() ? current_color : Qt::black,
		this,
		tr("Choose layer color"),
		QColorDialog::ShowAlphaChannel);
	if (chosen.isValid()) { setLayerColor(name, chosen); }
}

void LayersDialog::setLayerDescription(
	const QString& name, const QString& description)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	for (auto& layer : set.layers)
		if (layer.name == name) layer.description = description;
	if (set != m_project->drawingLayers())
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

/**
	@brief LayersDialog::putSelectionOnLayer
	@param folio : the folio whose selection it is; by default, the one the
	editor shows
	@return how many items were put on it. Symbols and wires selected with
	them are left as they are.
*/
int LayersDialog::putSelectionOnLayer(const QString& layer, Diagram* folio)
{
	const bool is_fixed = DrawingLayers::isFixed(layer)
						  && layer
								 != DrawingLayers::fixedKindName(
									 DrawingLayers::FixedKind::Graphics)
						  && layer
								 != DrawingLayers::fixedKindName(
									 DrawingLayers::FixedKind::Pictures);
	if (! m_project || is_fixed) return 0;
	if (! folio)
	{
		if (auto* editor = qobject_cast<QETDiagramEditor*>(parentWidget()))
			if (ProjectView* view = editor->currentProjectView())
				if (DiagramView* diagram_view = view->currentDiagram())
					folio = diagram_view->diagram();
	}
	if (! folio || folio->project() != m_project) return 0;
	QHash<QGraphicsObject*, QString> items;
	for (QGraphicsItem* item : folio->selectedItems())
		if (DrawingLayers::canBeOnLayer(item) && item->toGraphicsObject())
			items.insert(item->toGraphicsObject(), layer);
	if (items.isEmpty()) return 0;
	m_project->undoStack()->push(
		new DrawingLayersCommand(m_project, m_project->drawingLayers(), items));
	return int(items.size());
}

bool LayersDialog::addGroup(const QString& name)
{
	if (! m_project || name.isEmpty()) return false;
	Set set = m_project->drawingLayers();
	for (const auto& group : set.groups)
		if (group.name == name) return false;
	set.groups.append({name, {}});
	m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
	return true;
}

void LayersDialog::removeGroup(const QString& name)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	set.groups.erase(
		std::remove_if(
			set.groups.begin(),
			set.groups.end(),
			[&name](const DrawingLayers::Group& group)
			{ return group.name == name; }),
		set.groups.end());
	m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

void LayersDialog::setGroupLayers(
	const QString& name, const QStringList& layers)
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	for (auto& group : set.groups)
		if (group.name == name) group.layers = layers;
	if (set != m_project->drawingLayers())
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
}

/**
	@brief LayersDialog::showGroup
	The layers of the group on, the others off.
*/
void LayersDialog::showGroup(const QString& name)
{
	if (! m_project) return;
	for (const auto& group : m_project->drawingLayers().groups)
	{
		if (group.name != name) continue;
		const Set set = m_project->drawingLayers().showing(group);
		if (set != m_project->drawingLayers())
			m_project->undoStack()->push(
				new DrawingLayersCommand(m_project, set));
		return;
	}
}

bool LayersDialog::exportLayers(const QString& file_path)
{
	if (! m_project || file_path.isEmpty()) return false;
	QFile f(file_path);
	if (! f.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QET::QetMessageBox::critical(
			this,
			tr("Export layers"),
			tr("Unable to open file for writing: %1").arg(file_path));
		return false;
	}
	if (file_path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)
		|| file_path.endsWith(
			QStringLiteral(".qetlayers"),
			Qt::CaseInsensitive))
	{
		f.write(DrawingLayers::exportToXml(m_project->drawingLayers()));
	}
	else
	{
		f.write(
			DrawingLayers::exportToCsv(m_project->drawingLayers()).toUtf8());
	}
	f.close();
	QET::QetMessageBox::information(
		this,
		tr("Export layers"),
		tr("Layers exported successfully."));
	return true;
}

bool LayersDialog::importLayers(const QString& file_path)
{
	if (! m_project || file_path.isEmpty()) return false;
	QFile f(file_path);
	if (! f.open(QIODevice::ReadOnly))
	{
		QET::QetMessageBox::critical(
			this,
			tr("Import layers"),
			tr("Unable to open file for reading: %1").arg(file_path));
		return false;
	}
	const QByteArray data = f.readAll();
	f.close();

	Set	 set   = m_project->drawingLayers();
	int	 count = 0;
	bool ok	   = false;
	if (file_path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)
		|| file_path.endsWith(
			QStringLiteral(".qetlayers"),
			Qt::CaseInsensitive))
	{
		ok = DrawingLayers::importFromXml(set, data, &count);
	}
	else
	{
		ok = DrawingLayers::importFromCsv(set, QString::fromUtf8(data), &count);
	}
	if (! ok || count == 0)
	{
		QET::QetMessageBox::warning(
			this,
			tr("Import layers"),
			tr("No valid layers found in file."));
		return false;
	}
	m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
	QET::QetMessageBox::information(
		this,
		tr("Import layers"),
		tr("Successfully imported %1 layers.").arg(count));
	return true;
}

void LayersDialog::loadStandardLayers()
{
	if (! m_project) return;
	Set set = m_project->drawingLayers();
	DrawingLayers::loadStandardPresets(set);
	if (set != m_project->drawingLayers())
	{
		m_project->undoStack()->push(new DrawingLayersCommand(m_project, set));
	}
}
