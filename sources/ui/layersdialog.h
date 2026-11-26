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
#ifndef LAYERSDIALOG_H
#define LAYERSDIALOG_H

#include <QDialog>
#include <QPointer>

class Diagram;
class QGraphicsObject;
class QGroupBox;
class QLabel;
class QETProject;
class QListWidget;
class QTableWidget;

class QLineEdit;
class QToolButton;

/**
	@brief The LayersDialog class
	Project > Layers...: the project's layers of free drawing, each on or
	off, what is selected put on one, and the groups of layers.
	Every change is one step of the project's undo stack.
*/
class LayersDialog : public QDialog
{
	Q_OBJECT
  public:
	explicit LayersDialog(QETProject* project, QWidget* parent = nullptr);

  public slots:
	void refresh();
	bool addLayer(const QString& name, const QString& description = QString());
	bool renameLayer(const QString& name, const QString& new_name);
	void removeLayer(const QString& name);
	void setLayerVisible(const QString& name, bool visible);
	void setLayerPrintable(const QString& name, bool printable);
	void setLayerLocked(const QString& name, bool locked);
	void setLayerColor(const QString& name, const QColor& color);
	void chooseLayerColor(const QString& name, QWidget* button = nullptr);
	void setLayerDescription(const QString& name, const QString& description);
	int	 putSelectionOnLayer(const QString& layer, Diagram* folio = nullptr);
	bool addGroup(const QString& name);
	void removeGroup(const QString& name);
	void setGroupLayers(const QString& name, const QStringList& layers);
	void showGroup(const QString& name);
	void filterLayers(const QString& filter_text);
	bool exportLayers(const QString& file_path = QString());
	bool importLayers(const QString& file_path = QString());
	void loadStandardLayers();

  private slots:
	void updateButtonStates();

  private:
	struct LayerItemEntry
	{
		QPointer<Diagram>		  folio;
		QPointer<QGraphicsObject> item;
	};

	QString currentLayer() const;
	QString currentGroup() const;
	void	showGroupMembers();
	void	showLayerItems();
	void	navigateToLayerItem(int row);
	void	buildFolios();

	QPointer<QETProject>  m_project;
	QLineEdit*			  m_filter_edit = nullptr;
	QTableWidget*		  m_layers;
	QGroupBox*			  m_items_box	= nullptr;
	QTableWidget*		  m_layer_items = nullptr;
	QList<LayerItemEntry> m_item_entries;
	QListWidget*		  m_groups;
	QListWidget*		  m_members;
	QToolButton*		  m_groups_toggle  = nullptr;
	QWidget*			  m_groups_content = nullptr;
	QLabel*				  m_summary;
	QPushButton*		  m_rename_btn = nullptr;
	QPushButton*		  m_remove_btn = nullptr;
	QPushButton*		  m_put_btn	   = nullptr;
	QPushButton*		  m_take_btn   = nullptr;
	bool				  m_refreshing = false;
};

#endif // LAYERSDIALOG_H
