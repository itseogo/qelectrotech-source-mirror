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
#include <QPushButton>
#include <QTableWidget>
#include <QUndoStack>
#include <QVBoxLayout>
#include <dataBase/projectdatabase.h>
#include <diagram.h>
#include <diagramview.h>
#include <drawinglayers/drawinglayers.h>
#include <projectview.h>
#include <qet.h>
#include <qetdiagrameditor.h>
#include <qetmessagebox.h>
#include <qetproject.h>
#include <undocommand/drawinglayerscommand.h>

using DrawingLayers::Set;

LayersDialog::LayersDialog(QETProject* project, QWidget* parent) :
	QDialog(parent),
	m_project(project),
	m_layers(new QTableWidget(0, 5, this)),
	m_groups(new QListWidget(this)),
	m_members(new QListWidget(this)),
	m_summary(new QLabel(this))
{
	setWindowTitle(tr("Layers"));
	resize(720, 540);
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
	m_layers->setHorizontalHeaderLabels(
		{tr("Layer"),
		 tr("Description"),
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
	layout->addWidget(layers_box, 1);

	auto* groups_box =
		new QGroupBox(tr("Layer groups: the layers to show together"), this);
	auto* groups_layout = new QHBoxLayout(groups_box);
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
	layout->addWidget(groups_box, 1);

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
		&LayersDialog::updateButtonStates);
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
			else if (item->column() == 2)
			{
				setLayerVisible(layer_name, item->checkState() == Qt::Checked);
			}
			else if (item->column() == 3)
			{
				setLayerPrintable(
					layer_name,
					item->checkState() == Qt::Checked);
			}
			else if (item->column() == 4)
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

	if (m_rename_btn) m_rename_btn->setEnabled(has_selection && ! fixed);
	if (m_remove_btn) m_remove_btn->setEnabled(has_selection && ! fixed);
	if (m_put_btn) m_put_btn->setEnabled(has_selection && ! fixed);
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
		const QString display_name =
			l.isFixed() ? DrawingLayers::fixedKindDisplayName(l.fixed_kind) :
						  l.name;
		auto* name_item = new QTableWidgetItem(display_name);
		name_item->setData(Qt::UserRole, l.name);
		name_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		m_layers->setItem(row, 0, name_item);

		// Col 1: Description
		auto* desc_item = new QTableWidgetItem(l.description);
		desc_item->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
		m_layers->setItem(row, 1, desc_item);

		// Col 2: Shown
		auto* shown = new QTableWidgetItem();
		shown->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		shown->setCheckState(l.visible ? Qt::Checked : Qt::Unchecked);
		m_layers->setItem(row, 2, shown);

		// Col 3: Print
		auto* printable = new QTableWidgetItem();
		printable->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		printable->setCheckState(l.printable ? Qt::Checked : Qt::Unchecked);
		m_layers->setItem(row, 3, printable);

		// Col 4: Locked
		auto* locked = new QTableWidgetItem();
		locked->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		locked->setCheckState(l.locked ? Qt::Checked : Qt::Unchecked);
		m_layers->setItem(row, 4, locked);

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
	if (m_filter_edit && ! m_filter_edit->text().isEmpty())
	{
		filterLayers(m_filter_edit->text());
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
	if (! m_project || DrawingLayers::isFixed(layer)) return 0;
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
