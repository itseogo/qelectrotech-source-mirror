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

#include <QColor>
#include <QDomElement>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class Diagram;
class QGraphicsItem;

/**
	@brief The DrawingLayers namespace
	Named layers for a folio's free drawing: lines and shapes, free texts,
	pictures. A layer switched off hides what is on it on the screen, in
	print and in exports. Symbols and wires are never on a user layer, so hiding
	one cannot change wiring, numbering or lists.

	Built-in fixed layers control fixed kinds (frame and title block, wire
   numbers, symbol texts, cross-references) across the project and in layer
   groups.

	A layer group is a named set of layers to show together: the layers
	of a group on, the others off.
	A project with no layers looks and saves as before.
*/
namespace DrawingLayers
{
	enum class FixedKind
	{
		None = 0,
		FrameAndTitleBlock,
		WireNumbers,
		SymbolTexts,
		CrossReferences,
		Graphics,
		Pictures
	};

	QString	  fixedKindName(FixedKind kind);
	QString	  fixedKindDisplayName(FixedKind kind);
	QString	  fixedKindAttribute(FixedKind kind);
	FixedKind fixedKindOf(const QString& name);
	FixedKind fixedKindFromAttribute(const QString& attr);
	bool	  isFixed(const QString& name);

	struct Layer
	{
		QString	  name;
		QString	  description;
		QColor	  color;
		bool	  visible	 = true;
		bool	  printable	 = true;
		bool	  locked	 = false;
		FixedKind fixed_kind = FixedKind::None;

		Layer() = default;
		Layer(
			const QString& n,
			bool		   v  = true,
			FixedKind	   fk = FixedKind::None,
			const QColor&  c  = QColor()) :
			name(n), color(c), visible(v), fixed_kind(fk)
		{
		}
		Layer(
			const QString& n,
			const QString& d,
			bool		   v  = true,
			bool		   p  = true,
			bool		   l  = false,
			FixedKind	   fk = FixedKind::None,
			const QColor&  c  = QColor()) :
			name(n),
			description(d),
			color(c),
			visible(v),
			printable(p),
			locked(l),
			fixed_kind(fk)
		{
		}

		bool isFixed() const { return fixed_kind != FixedKind::None; }
		bool operator==(const Layer& o) const
		{
			return name == o.name && description == o.description
				   && color == o.color && visible == o.visible
				   && printable == o.printable && locked == o.locked
				   && fixed_kind == o.fixed_kind;
		}
	};

	QList<Layer> defaultFixedLayers();

	struct Group
	{
		QString		name;
		QStringList layers;
		bool		operator==(const Group& o) const
		{ return name == o.name && layers == o.layers; }
	};

	struct Set
	{
		Set();
		QList<Layer> layers;
		QList<Group> groups;

		void		 ensureFixedLayers();
		QList<Layer> userLayers() const;
		QList<Layer> fixedLayers() const;

		bool isEmpty() const;
		bool operator==(const Set& o) const
		{ return layers == o.layers && groups == o.groups; }
		bool		operator!=(const Set& o) const { return ! (*this == o); }
		bool		contains(const QString& name) const;
		bool		isVisible(const QString& name) const;
		bool		isVisible(FixedKind kind) const;
		bool		isPrintable(const QString& name) const;
		bool		isPrintable(FixedKind kind) const;
		bool		isLocked(const QString& name) const;
		bool		isLocked(FixedKind kind) const;
		QColor		color(const QString& name) const;
		QColor		color(FixedKind kind) const;
		void		setColor(const QString& name, const QColor& color);
		QString		description(const QString& name) const;
		QStringList names() const;
		Set			showing(const Group& group) const;
		Set			forPrinting() const;

		QDomElement toXml(QDomDocument& document) const;
		static Set	fromXml(const QDomElement& element);
	};

	QString layerOf(const QGraphicsItem* item);
	void	setLayerOf(QGraphicsItem* item, const QString& layer);
	void	writeAttribute(const QGraphicsItem* item, QDomElement& element);
	void	readAttribute(QGraphicsItem* item, const QDomElement& element);

	bool	canBeOnLayer(const QGraphicsItem* item);
	bool	isHidden(const QGraphicsItem* item);
	QString lockedLayerOf(const QGraphicsItem* item);
	bool	isLocked(const QGraphicsItem* item);
	void apply(QGraphicsItem* item, const Set& set);
	void applyTo(Diagram* folio, const Set& set);

	QString exportToCsv(const Set& set);
	bool	importFromCsv(
		Set& set, const QString& csv_content, int* imported_count = nullptr);
	QByteArray exportToXml(const Set& set);
	bool	   importFromXml(
		Set& set, const QByteArray& xml_content, int* imported_count = nullptr);

	QList<Layer>				   standardPresetLayers();
	QList<Group>				   standardPresetGroups();
	void						   loadStandardPresets(Set& set);
	bool						   ensureLayer(Set& set, const QString& name, bool create_custom = false);
	QList<QPair<QString, QString>> availableLayerChoices(const Set& set);
	QString						   dxfLayerName(const Layer& layer);
} // namespace DrawingLayers

#endif // DRAWINGLAYERS_H
