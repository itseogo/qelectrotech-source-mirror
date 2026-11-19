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
#ifndef DRAWINGLAYERS_H
#define DRAWINGLAYERS_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QDomElement>

class Diagram;
class QGraphicsItem;

/**
	@brief The DrawingLayers namespace
	Named layers for a folio's free drawing: lines and shapes, free texts,
	pictures. A layer switched off hides what is on it on the screen, in
	print and in exports. Symbols and wires are never on a user layer, so hiding
	one cannot change wiring, numbering or lists.

	Built-in fixed layers control fixed kinds (frame and title block, wire numbers,
	symbol texts, cross-references) across the project and in layer groups.

	A layer group is a named set of layers to show together: the layers
	of a group on, the others off.
	A project with no layers looks and saves as before.
*/
namespace DrawingLayers
{
	enum class FixedKind {
		None = 0,
		FrameAndTitleBlock,
		WireNumbers,
		SymbolTexts,
		CrossReferences
	};

	QString fixedKindName(FixedKind kind);
	QString fixedKindDisplayName(FixedKind kind);
	QString fixedKindAttribute(FixedKind kind);
	FixedKind fixedKindOf(const QString &name);
	FixedKind fixedKindFromAttribute(const QString &attr);
	bool isFixed(const QString &name);

	struct Layer
	{
		QString name;
		bool visible = true;
		FixedKind fixed_kind = FixedKind::None;

		bool isFixed() const {return fixed_kind != FixedKind::None;}
		bool operator==(const Layer &o) const {return name == o.name && visible == o.visible && isFixed() == o.isFixed();}
	};

	QList<Layer> defaultFixedLayers();

	struct Group
	{
		QString name;
		QStringList layers;
		bool operator==(const Group &o) const {return name == o.name && layers == o.layers;}
	};

	struct Set
	{
		Set();
		QList<Layer> layers;
		QList<Group> groups;

		void ensureFixedLayers();
		QList<Layer> userLayers() const;
		QList<Layer> fixedLayers() const;

		bool isEmpty() const;
		bool operator==(const Set &o) const {return layers == o.layers && groups == o.groups;}
		bool operator!=(const Set &o) const {return !(*this == o);}
		bool contains(const QString &name) const;
		bool isVisible(const QString &name) const;
		bool isVisible(FixedKind kind) const;
		QStringList names() const;
		Set showing(const Group &group) const;

		QDomElement toXml(QDomDocument &document) const;
		static Set fromXml(const QDomElement &element);
	};

	QString layerOf(const QGraphicsItem *item);
	void setLayerOf(QGraphicsItem *item, const QString &layer);
	void writeAttribute(const QGraphicsItem *item, QDomElement &element);
	void readAttribute(QGraphicsItem *item, const QDomElement &element);

	bool canBeOnLayer(const QGraphicsItem *item);
	bool isHidden(const QGraphicsItem *item);
	void apply(QGraphicsItem *item, const Set &set);
	void applyTo(Diagram *folio, const Set &set);
}

#endif // DRAWINGLAYERS_H
