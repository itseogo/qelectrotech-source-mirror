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
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "drawinglayers/drawinglayers.h"
#include "print/projectprintwindow.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/crossrefitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "qetresult.h"
#include "shownkinds.h"
#include "ui/dynamicelementtextmodel.h"
#include "ui/layersdialog.h"
#include "undocommand/drawinglayerscommand.h"

#include <QComboBox>
#include <QFile>
#include <QHash>
#include <QListWidget>
#include <QPrinter>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

/**
	Layers of a folio's free drawing: kept with the project, hiding what is
	on them, a project without any saving as before.
*/
class tst_drawinglayers : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	static DrawingLayers::Set twoLayers(bool first = true, bool second = true)
	{
		DrawingLayers::Set set;
		set.layers.append({QStringLiteral("Quote"), first});
		set.layers.append({QStringLiteral("Mounting"), second});
		set.groups = {
			{QStringLiteral("For the customer"), {QStringLiteral("Quote")}}};
		return set;
	}

	static QetShapeItem* line(Diagram* folio, const QString& layer)
	{
		auto* shape = new QetShapeItem(
			QPointF(10, 10),
			QPointF(100, 10),
			QetShapeItem::Line);
		folio->addItem(shape);
		DrawingLayers::setLayerOf(shape, layer);
		return shape;
	}

	static IndependentTextItem* text(Diagram* folio, const QString& layer)
	{
		auto* item = new IndependentTextItem(QStringLiteral("NOTE"));
		folio->addItem(item);
		DrawingLayers::setLayerOf(item, layer);
		return item;
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
		QET::QetMessageBox::setNonInteractive(true);
	}

	void setsReadBackAndGroupsShow()
	{
		QDomDocument			 doc;
		const DrawingLayers::Set read =
			DrawingLayers::Set::fromXml(twoLayers(true, false).toXml(doc));
		QVERIFY(read == twoLayers(true, false));
		QVERIFY(read.isVisible(QStringLiteral("Quote")));
		QVERIFY(! read.isVisible(QStringLiteral("Mounting")));
		QVERIFY(read.isVisible(
			QStringLiteral("Unknown"))); // a layer the project does not know
		const auto shown = twoLayers(false, true).showing(read.groups.first());
		QVERIFY(shown.isVisible(QStringLiteral("Quote")));
		QVERIFY(! shown.isVisible(QStringLiteral("Mounting")));
	}

	void onlyFreeDrawingIsOnALayer()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		QVERIFY(DrawingLayers::canBeOnLayer(line(folio, QString())));
		QVERIFY(DrawingLayers::canBeOnLayer(text(folio, QString())));
		// A structure box is a shape too, of a type of its own
		auto* box = new StructureBoxItem(QPointF(0, 0), QPointF(100, 100));
		folio->addItem(box);
		QVERIFY(DrawingLayers::canBeOnLayer(box));
		QGraphicsRectItem other;
		QVERIFY(! DrawingLayers::canBeOnLayer(&other));
	}

	void hidingALayerHidesWhatIsOnIt()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		project.setDrawingLayers(twoLayers());
		auto* quote		= line(folio, QStringLiteral("Quote"));
		auto* mounting	= text(folio, QStringLiteral("Mounting"));
		auto* free_item = line(folio, QString());
		project.setDrawingLayers(twoLayers(false, true));
		QVERIFY(! quote->isVisible());
		QVERIFY(mounting->isVisible());
		QVERIFY(free_item->isVisible());
		QVERIFY(DrawingLayers::isHidden(quote));
		QVERIFY(! DrawingLayers::isHidden(free_item));
		project.setDrawingLayers(twoLayers());
		QVERIFY(quote->isVisible());
	}

	void layersAndViewShowEachOnlyVeto()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		project.setDrawingLayers(twoLayers());
		auto* shape = line(folio, QStringLiteral("Quote"));
		// Off by its layer: showing its kind again does not bring it back
		project.setDrawingLayers(twoLayers(false, true));
		ShownKinds::setShown(ShownKinds::Shapes, false);
		ShownKinds::apply(folio, ShownKinds::Shapes);
		ShownKinds::setShown(ShownKinds::Shapes, true);
		ShownKinds::apply(folio, ShownKinds::Shapes);
		QVERIFY(! shape->isVisible());
		project.setDrawingLayers(twoLayers());
		QVERIFY(shape->isVisible());
	}

	void keptWithTheProject_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("layers.qet");
		QTest::newRow("qetdb") << QStringLiteral("layers.qetdb");
	}

	void keptWithTheProject()
	{
		QFETCH(QString, file);
		const QString path = m_dir.filePath(file);
		{
			QETProject project;
			Diagram*   folio = project.addNewDiagram();
			project.setDrawingLayers(twoLayers(false, true));
			line(folio, QStringLiteral("Quote"));
			text(folio, QStringLiteral("Mounting"));
			line(folio, QString());
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QVERIFY(again.drawingLayers() == twoLayers(false, true));
		again.loadFolios();
		int hidden = 0, on_quote = 0, on_mounting = 0, none = 0;
		for (QGraphicsItem* item : again.diagrams().first()->items())
		{
			if (! DrawingLayers::canBeOnLayer(item)) continue;
			const QString layer = DrawingLayers::layerOf(item);
			on_quote += layer == QLatin1String("Quote");
			on_mounting += layer == QLatin1String("Mounting");
			none += layer.isEmpty();
			hidden += ! item->isVisible();
		}
		QCOMPARE(on_quote, 1);
		QCOMPARE(on_mounting, 1);
		QCOMPARE(none, 1);
		QCOMPARE(hidden, 1);
	}

	void aProjectWithoutLayersSavesAsBefore()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		line(folio, QString());
		const QString path = m_dir.filePath(QStringLiteral("none.qet"));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
		QFile f(path);
		QVERIFY(f.open(QIODevice::ReadOnly));
		const QByteArray saved = f.readAll();
		QVERIFY(! saved.contains("<layers"));
		QVERIFY(! saved.contains("layer="));
	}

	void dialogManagesLayers()
	{
		QETProject	 project;
		Diagram*	 folio = project.addNewDiagram();
		auto*		 shape = line(folio, QString());
		LayersDialog dialog(&project);
		auto*		 table =
			dialog.findChild<QTableWidget*>(QStringLiteral("layersTable"));
		QVERIFY(table);
		QVERIFY(dialog.addLayer(QStringLiteral("Quote")));
		QVERIFY(! dialog.addLayer(QStringLiteral("Quote")));
		QCOMPARE(table->rowCount(), 5);
		QCOMPARE(table->item(4, 0)->text(), QStringLiteral("Quote"));
		shape->setSelected(true);
		QCOMPARE(dialog.putSelectionOnLayer(QStringLiteral("Quote"), folio), 1);
		QCOMPARE(DrawingLayers::layerOf(shape), QStringLiteral("Quote"));
		// Unticking in the table hides what is on it
		table->item(4, 2)->setCheckState(Qt::Unchecked);
		QVERIFY(! project.drawingLayers().isVisible(QStringLiteral("Quote")));
		QVERIFY(! shape->isVisible());
		table->item(4, 2)->setCheckState(Qt::Checked);
		QVERIFY(shape->isVisible());
		// A rename follows on the items and in the groups
		QVERIFY(dialog.addGroup(QStringLiteral("Customer")));
		dialog.setGroupLayers(
			QStringLiteral("Customer"),
			{QStringLiteral("Quote")});
		QVERIFY(dialog.renameLayer(
			QStringLiteral("Quote"),
			QStringLiteral("Offer")));
		QCOMPARE(DrawingLayers::layerOf(shape), QStringLiteral("Offer"));
		QCOMPARE(
			project.drawingLayers().groups.first().layers,
			QStringList({QStringLiteral("Offer")}));
		// Undo through the stack comes back to the dialog
		project.undoStack()->undo();
		QCOMPARE(DrawingLayers::layerOf(shape), QStringLiteral("Quote"));
		QCOMPARE(table->item(4, 0)->text(), QStringLiteral("Quote"));
		project.undoStack()->redo();
		// Removing a layer keeps what was on it, on no layer
		dialog.removeLayer(QStringLiteral("Offer"));
		QVERIFY(project.drawingLayers().userLayers().isEmpty());
		QCOMPARE(project.drawingLayers().fixedLayers().size(), 4);
		QCOMPARE(DrawingLayers::layerOf(shape), QString());
		QVERIFY(shape->isVisible());
		QVERIFY(project.drawingLayers().groups.first().layers.isEmpty());
	}

	void showingAGroup()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		project.setDrawingLayers(twoLayers());
		auto*		 quote	  = line(folio, QStringLiteral("Quote"));
		auto*		 mounting = line(folio, QStringLiteral("Mounting"));
		LayersDialog dialog(&project);
		dialog.showGroup(QStringLiteral("For the customer"));
		QVERIFY(quote->isVisible());
		QVERIFY(! mounting->isVisible());
		project.undoStack()->undo();
		QVERIFY(mounting->isVisible());
	}

	void aGroupIsChosenForOnePrint()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		project.setDrawingLayers(twoLayers());
		auto* quote	   = line(folio, QStringLiteral("Quote"));
		auto* mounting = line(folio, QStringLiteral("Mounting"));
		{
			ProjectPrintWindow window(&project, new QPrinter());
			auto*			   choice = window.findChild<QComboBox*>(
				 QStringLiteral("m_layer_group_cb"));
			QVERIFY(choice);
			QCOMPARE(choice->count(), 2);
			choice->setCurrentIndex(1);
			QVERIFY(quote->isVisible());
			QVERIFY(! mounting->isVisible());
			// For this print only: the project is as it was
			QVERIFY(
				project.drawingLayers().isVisible(QStringLiteral("Mounting")));
		}
		QVERIFY(mounting->isVisible());
	}

	void withoutGroupsThePrintWindowHasNoChoice()
	{
		QETProject project;
		project.addNewDiagram();
		ProjectPrintWindow window(&project, new QPrinter());
		QVERIFY(
			! window.findChild<QComboBox*>(QStringLiteral("m_layer_group_cb")));
	}

	void undoGivesBackLayersAndItems()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		project.setDrawingLayers(twoLayers());
		auto*							 shape = line(folio, QString());
		QHash<QGraphicsObject*, QString> items;
		items.insert(shape, QStringLiteral("Quote"));
		DrawingLayersCommand command(&project, twoLayers(false, true), items);
		command.redo();
		QCOMPARE(DrawingLayers::layerOf(shape), QStringLiteral("Quote"));
		QVERIFY(! shape->isVisible());
		command.undo();
		QCOMPARE(DrawingLayers::layerOf(shape), QString());
		QVERIFY(shape->isVisible());
		QVERIFY(project.drawingLayers() == twoLayers());
	}

	void fixedLayersCannotBeRemovedOrRenamed()
	{
		QETProject	  project;
		Diagram*	  folio = project.addNewDiagram();
		auto*		  shape = line(folio, QString());
		LayersDialog  dialog(&project);
		const QString wire_name =
			DrawingLayers::fixedKindName(DrawingLayers::FixedKind::WireNumbers);

		QVERIFY(DrawingLayers::isFixed(wire_name));
		QVERIFY(! dialog.addLayer(wire_name));
		QVERIFY(! dialog.renameLayer(wire_name, QStringLiteral("CustomWires")));
		dialog.removeLayer(wire_name);
		QVERIFY(project.drawingLayers().contains(wire_name));
		QCOMPARE(dialog.putSelectionOnLayer(wire_name, folio), 0);
		QCOMPARE(DrawingLayers::layerOf(shape), QString());
	}

	void fixedLayersControlBorderAndShownKinds()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		QVERIFY(folio->useBorder());

		auto* cond_text = new ConductorTextItem(QStringLiteral("W1"));
		folio->addItem(cond_text);
		QVERIFY(cond_text->isVisible());

		auto* sym_text = new QGraphicsTextItem(QStringLiteral("K1"));
		ShownKinds::tag(sym_text, ShownKinds::SymbolTexts);
		folio->addItem(sym_text);
		QVERIFY(sym_text->isVisible());

		auto* xref = new QGraphicsTextItem(QStringLiteral("/2.3"));
		ShownKinds::tag(xref, ShownKinds::CrossReferences);
		folio->addItem(xref);
		QVERIFY(xref->isVisible());

		// Turn off wire numbers
		DrawingLayers::Set set = project.drawingLayers();
		const QString	   wire_name =
			DrawingLayers::fixedKindName(DrawingLayers::FixedKind::WireNumbers);
		for (auto& layer : set.layers)
		{
			if (layer.name == wire_name) layer.visible = false;
		}
		project.setDrawingLayers(set);
		QVERIFY(! cond_text->isVisible());
		QVERIFY(DrawingLayers::isHidden(cond_text));
		QVERIFY(sym_text->isVisible());
		QVERIFY(xref->isVisible());
		QVERIFY(folio->useBorder());

		// Turn off frame and title block
		const QString frame_name = DrawingLayers::fixedKindName(
			DrawingLayers::FixedKind::FrameAndTitleBlock);
		for (auto& layer : set.layers)
		{
			if (layer.name == frame_name) layer.visible = false;
		}
		project.setDrawingLayers(set);
		QVERIFY(! folio->useBorder());

		// Turn off symbol texts
		const QString sym_name =
			DrawingLayers::fixedKindName(DrawingLayers::FixedKind::SymbolTexts);
		for (auto& layer : set.layers)
		{
			if (layer.name == sym_name) layer.visible = false;
		}
		project.setDrawingLayers(set);
		QVERIFY(! sym_text->isVisible());
		QVERIFY(DrawingLayers::isHidden(sym_text));

		// Turn off cross-references
		const QString xref_name = DrawingLayers::fixedKindName(
			DrawingLayers::FixedKind::CrossReferences);
		for (auto& layer : set.layers)
		{
			if (layer.name == xref_name) layer.visible = false;
		}
		project.setDrawingLayers(set);
		QVERIFY(! xref->isVisible());
		QVERIFY(DrawingLayers::isHidden(xref));

		// Turn them back on
		for (auto& layer : set.layers) layer.visible = true;
		project.setDrawingLayers(set);
		QVERIFY(cond_text->isVisible());
		QVERIFY(! DrawingLayers::isHidden(cond_text));
		QVERIFY(sym_text->isVisible());
		QVERIFY(! DrawingLayers::isHidden(sym_text));
		QVERIFY(xref->isVisible());
		QVERIFY(! DrawingLayers::isHidden(xref));
		QVERIFY(folio->useBorder());
	}

	void fixedLayersInGroupsAndPrinting()
	{
		QETProject project;
		Diagram*   folio	 = project.addNewDiagram();
		auto*	   cond_text = new ConductorTextItem(QStringLiteral("W2"));
		folio->addItem(cond_text);

		DrawingLayers::Set	 set = project.drawingLayers();
		DrawingLayers::Group group;
		group.name = QStringLiteral("No Wire Numbers");
		// Include frame and title block but NOT wire numbers
		group.layers << DrawingLayers::fixedKindName(
			DrawingLayers::FixedKind::FrameAndTitleBlock);
		set.groups.append(group);
		project.setDrawingLayers(set);

		QVERIFY(cond_text->isVisible());
		QVERIFY(folio->useBorder());

		{
			ProjectPrintWindow window(&project, new QPrinter());
			auto*			   choice = window.findChild<QComboBox*>(
				 QStringLiteral("m_layer_group_cb"));
			QVERIFY(choice);
			choice->setCurrentIndex(1); // Select "No Wire Numbers"
			QVERIFY(! cond_text->isVisible());
			QVERIFY(folio->useBorder());
			// Project itself is not permanently changed
			QVERIFY(project.drawingLayers().isVisible(
				DrawingLayers::FixedKind::WireNumbers));
		}
		// When print window closes, project layers are restored
		QVERIFY(cond_text->isVisible());
	}

	void layerDescriptionPrintableAndLocked()
	{
		QETProject project;
		Diagram*   folio = project.addNewDiagram();
		auto*	   shape = line(folio, QStringLiteral("Background"));

		DrawingLayers::Set	 set = project.drawingLayers();
		DrawingLayers::Layer bg;
		bg.name		   = QStringLiteral("Background");
		bg.description = QStringLiteral("DXF background floor plan");
		bg.visible	   = true;
		bg.printable   = false;
		bg.locked	   = true;
		set.layers.append(bg);
		project.setDrawingLayers(set);

		QVERIFY(project.drawingLayers().contains(QStringLiteral("Background")));
		QCOMPARE(
			project.drawingLayers().description(QStringLiteral("Background")),
			QStringLiteral("DXF background floor plan"));
		QVERIFY(
			project.drawingLayers().isVisible(QStringLiteral("Background")));
		QVERIFY(! project.drawingLayers().isPrintable(
			QStringLiteral("Background")));
		QVERIFY(project.drawingLayers().isLocked(QStringLiteral("Background")));

		// On screen: visible, but not selectable or movable because it is
		// locked
		QVERIFY(shape->isVisible());
		QVERIFY(! (shape->flags() & QGraphicsItem::ItemIsMovable));
		QVERIFY(! (shape->flags() & QGraphicsItem::ItemIsSelectable));

		// forPrinting: automatically hidden because printable == false
		const DrawingLayers::Set print_set =
			project.drawingLayers().forPrinting();
		QVERIFY(! print_set.isVisible(QStringLiteral("Background")));

		// Unlock layer
		LayersDialog dialog(&project);
		dialog.setLayerLocked(QStringLiteral("Background"), false);
		QVERIFY(
			! project.drawingLayers().isLocked(QStringLiteral("Background")));
		QVERIFY(shape->flags() & QGraphicsItem::ItemIsMovable);
		QVERIFY(shape->flags() & QGraphicsItem::ItemIsSelectable);

		// XML round trip preserves description, printable, and locked
		dialog.setLayerLocked(QStringLiteral("Background"), true);
		QDomDocument			 doc;
		const DrawingLayers::Set read =
			DrawingLayers::Set::fromXml(project.drawingLayers().toXml(doc));
		QCOMPARE(
			read.description(QStringLiteral("Background")),
			QStringLiteral("DXF background floor plan"));
		QVERIFY(! read.isPrintable(QStringLiteral("Background")));
		QVERIFY(read.isLocked(QStringLiteral("Background")));
	}

	void filterLayersInDialog()
	{
		QETProject	 project;
		LayersDialog dialog(&project);
		auto*		 table =
			dialog.findChild<QTableWidget*>(QStringLiteral("layersTable"));
		QVERIFY(table);

		dialog.addLayer(
			QStringLiteral("Mechanics"),
			QStringLiteral("3D layout"));
		dialog.addLayer(QStringLiteral("Electronics"), QStringLiteral("PCBs"));

		// 4 fixed layers + 2 added = 6 rows
		QCOMPARE(table->rowCount(), 6);

		// Filter by "mechanic" -> only Mechanics should be visible
		dialog.filterLayers(QStringLiteral("mechanic"));
		for (int row = 0; row < table->rowCount(); ++row)
		{
			const QString name = table->item(row, 0)->text();
			if (name == QLatin1String("Mechanics"))
			{
				QVERIFY(! table->isRowHidden(row));
			}
			else
			{
				QVERIFY(table->isRowHidden(row));
			}
		}

		// Filter by description "PCBs" -> only Electronics should be visible
		dialog.filterLayers(QStringLiteral("PCBs"));
		for (int row = 0; row < table->rowCount(); ++row)
		{
			const QString name = table->item(row, 0)->text();
			if (name == QLatin1String("Electronics"))
			{
				QVERIFY(! table->isRowHidden(row));
			}
			else
			{
				QVERIFY(table->isRowHidden(row));
			}
		}

		// Clear filter -> all rows visible
		dialog.filterLayers(QString());
		for (int row = 0; row < table->rowCount(); ++row)
		{
			QVERIFY(! table->isRowHidden(row));
		}
	}

	void importAndExportCsvAndXml()
	{
		// 1. Export Set to CSV
		DrawingLayers::Set orig;
		orig.layers.append(
			{QStringLiteral("Notes"),
			 QStringLiteral("Customer notes"),
			 true,
			 false,
			 true});
		const QString csv = DrawingLayers::exportToCsv(orig);
		QVERIFY(csv.contains(
			QStringLiteral("Name,Description,Visible,Printable,Locked")));
		QVERIFY(csv.contains(
			QStringLiteral("Notes,Customer notes,true,false,true")));

		// 2. Import standard CSV
		DrawingLayers::Set imported_csv_set;
		int				   count = 0;
		QVERIFY(DrawingLayers::importFromCsv(imported_csv_set, csv, &count));
		QCOMPARE(count, 5); // 4 fixed system layers + 1 user layer
		QVERIFY(imported_csv_set.contains(QStringLiteral("Notes")));
		QCOMPARE(
			imported_csv_set.description(QStringLiteral("Notes")),
			QStringLiteral("Customer notes"));
		QVERIFY(imported_csv_set.isVisible(QStringLiteral("Notes")));
		QVERIFY(! imported_csv_set.isPrintable(QStringLiteral("Notes")));
		QVERIFY(imported_csv_set.isLocked(QStringLiteral("Notes")));

		// 3. Import TSV table with German headers and CAD bracket booleans ([v]
		// / [ ])
		const QString tsv_data = QStringLiteral(
			"Ebene\tBeschreibung\tSichtbar\tDrucken\tGesperrt\n"
			"100_Grafik\tAllgemeine Grafik\t[v]\t[v]\t[ ]\n"
			"109_Kommentar\tInterne Notizen\t[v]\t[ ]\t[v]\n");
		DrawingLayers::Set tsv_set;
		QVERIFY(DrawingLayers::importFromCsv(tsv_set, tsv_data, &count));
		QCOMPARE(count, 2);
		QVERIFY(tsv_set.contains(QStringLiteral("100_Grafik")));
		QCOMPARE(
			tsv_set.description(QStringLiteral("100_Grafik")),
			QStringLiteral("Allgemeine Grafik"));
		QVERIFY(tsv_set.isVisible(QStringLiteral("100_Grafik")));
		QVERIFY(tsv_set.isPrintable(QStringLiteral("100_Grafik")));
		QVERIFY(! tsv_set.isLocked(QStringLiteral("100_Grafik")));

		QVERIFY(tsv_set.contains(QStringLiteral("109_Kommentar")));
		QCOMPARE(
			tsv_set.description(QStringLiteral("109_Kommentar")),
			QStringLiteral("Interne Notizen"));
		QVERIFY(tsv_set.isVisible(QStringLiteral("109_Kommentar")));
		QVERIFY(! tsv_set.isPrintable(QStringLiteral("109_Kommentar")));
		QVERIFY(tsv_set.isLocked(QStringLiteral("109_Kommentar")));

		// 4. XML export and import round-trip
		const QByteArray xml_bytes = DrawingLayers::exportToXml(tsv_set);
		QVERIFY(! xml_bytes.isEmpty());
		DrawingLayers::Set xml_set;
		QVERIFY(DrawingLayers::importFromXml(xml_set, xml_bytes, &count));
		QCOMPARE(count, 6); // 4 fixed layers + 2 user layers
		QVERIFY(xml_set.contains(QStringLiteral("100_Grafik")));
		QVERIFY(xml_set.contains(QStringLiteral("109_Kommentar")));
		QCOMPARE(
			xml_set.description(QStringLiteral("109_Kommentar")),
			QStringLiteral("Interne Notizen"));
		QVERIFY(! xml_set.isPrintable(QStringLiteral("109_Kommentar")));
		QVERIFY(xml_set.isLocked(QStringLiteral("109_Kommentar")));

		// 5. LayersDialog export and import with UndoStack
		QETProject project1;
		project1.setDrawingLayers(tsv_set);
		LayersDialog  dlg1(&project1);
		const QString export_file =
			m_dir.filePath(QStringLiteral("test_layers.csv"));
		QVERIFY(dlg1.exportLayers(export_file));
		QVERIFY(QFile::exists(export_file));

		QETProject	 project2;
		LayersDialog dlg2(&project2);
		QVERIFY(dlg2.importLayers(export_file));
		QVERIFY(
			project2.drawingLayers().contains(QStringLiteral("100_Grafik")));
		QVERIFY(
			project2.drawingLayers().contains(QStringLiteral("109_Kommentar")));

		// Verify undo reverts the import
		QVERIFY(project2.undoStack()->canUndo());
		project2.undoStack()->undo();
		QVERIFY(
			! project2.drawingLayers().contains(QStringLiteral("100_Grafik")));
		QVERIFY(! project2.drawingLayers().contains(
			QStringLiteral("109_Kommentar")));
	}

	void standardPresetsAndDynamicElementTextLayers()
	{
		// 1. Standard presets loading
		DrawingLayers::Set preset_set;
		DrawingLayers::loadStandardPresets(preset_set);
		QCOMPARE(
			preset_set.layers.size(),
			15); // 4 fixed system layers + 11 preset layers

		const QStringList expected_layers = {
			QStringLiteral("100"),
			QStringLiteral("101"),
			QStringLiteral("102"),
			QStringLiteral("107"),
			QStringLiteral("108"),
			QStringLiteral("109"),
			QStringLiteral("201"),
			QStringLiteral("307"),
			QStringLiteral("400"),
			QStringLiteral("405"),
			QStringLiteral("420")};
		for (const QString& name : expected_layers)
		{
			QVERIFY(preset_set.contains(name));
		}

		QCOMPARE(
			preset_set.description(QStringLiteral("201")),
			QObject::tr("Graphics.Folio border"));
		QCOMPARE(
			preset_set.description(QStringLiteral("400")),
			QObject::tr("Element texts.Device tags (BMK)"));
		QVERIFY(preset_set.isVisible(QStringLiteral("109")));
		QVERIFY(! preset_set.isPrintable(QStringLiteral("109")));

		// 2. Preset group "Without device tags"
		QCOMPARE(preset_set.groups.size(), 1);
		QCOMPARE(
			preset_set.groups.first().name,
			QObject::tr("Without device tags"));

		const DrawingLayers::Set without_bmk =
			preset_set.showing(preset_set.groups.first());
		QVERIFY(! without_bmk.isVisible(QStringLiteral("400")));
		QVERIFY(! without_bmk.isVisible(
			DrawingLayers::fixedKindName(
				DrawingLayers::FixedKind::SymbolTexts)));
		QVERIFY(without_bmk.isVisible(QStringLiteral("100")));
		QVERIFY(without_bmk.isVisible(QStringLiteral("201")));
		QVERIFY(without_bmk.isVisible(QStringLiteral("405")));
		QVERIFY(without_bmk.isVisible(
			DrawingLayers::fixedKindName(
				DrawingLayers::FixedKind::WireNumbers)));

		// 3. DynamicElementTextItem layer assignment and XML round-trip
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element* e = nullptr;
		for (Diagram* d : project.diagrams())
		{
			for (Element* el : d->elements())
			{
				if (el)
				{
					e = el;
					break;
				}
			}
			if (e) break;
		}
		QVERIFY(e);
		Diagram* folio = e->diagram();
		auto*	 deti  = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);

		QVERIFY(DrawingLayers::canBeOnLayer(deti));
		QVERIFY(deti->layer().isEmpty());

		deti->setLayer(QStringLiteral("400"));
		QCOMPARE(deti->layer(), QStringLiteral("400"));
		QCOMPARE(DrawingLayers::layerOf(deti), QStringLiteral("400"));

		QDomDocument	  doc;
		const QDomElement deti_xml = deti->toXml(doc);
		QCOMPARE(
			deti_xml.attribute(QStringLiteral("layer")),
			QStringLiteral("400"));

		auto* restored_deti = new DynamicElementTextItem(e);
		restored_deti->fromXml(deti_xml);
		QCOMPARE(restored_deti->layer(), QStringLiteral("400"));
		delete restored_deti;

		// 4. Folio visibility applying
		DrawingLayers::applyTo(folio, preset_set);
		QVERIFY(deti->isVisible());

		DrawingLayers::applyTo(folio, without_bmk);
		QVERIFY(! deti->isVisible());

		// Verify DynamicElementTextModel creates layer entry
		DynamicElementTextModel model(e);
		QUndoCommand*			model_undo = model.undoForEditedText(deti);
		delete model_undo;

		// 5. Load standard layers in LayersDialog with undo/redo
		QETProject	 project_dlg;
		LayersDialog dlg(&project_dlg);
		dlg.loadStandardLayers();

		QVERIFY(project_dlg.drawingLayers().contains(QStringLiteral("100")));
		QVERIFY(project_dlg.drawingLayers().contains(QStringLiteral("400")));
		QCOMPARE(project_dlg.drawingLayers().groups.size(), 1);

		QVERIFY(project_dlg.undoStack()->canUndo());
		project_dlg.undoStack()->undo();
		QVERIFY(! project_dlg.drawingLayers().contains(QStringLiteral("100")));
		QVERIFY(! project_dlg.drawingLayers().contains(QStringLiteral("400")));
		QCOMPARE(project_dlg.drawingLayers().groups.size(), 0);

		project_dlg.undoStack()->redo();
		QVERIFY(project_dlg.drawingLayers().contains(QStringLiteral("100")));
		QVERIFY(project_dlg.drawingLayers().contains(QStringLiteral("400")));
	}
};

QET_APP_TEST_MAIN(tst_drawinglayers)
#include "tst_drawinglayers.moc"
