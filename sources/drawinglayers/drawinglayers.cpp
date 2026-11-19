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
#include "drawinglayers.h"

#include "../diagram.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../qetgraphicsitem/structureboxitem.h"
#include "../shownkinds.h"

#include <QGraphicsItem>
#include <QObject>

namespace DrawingLayers
{
	namespace
	{
		const int LayerKey = 0x4c4159;       // "LAY"
		const int LayerHiddenKey = 0x4c4148; // "LAH"

		void applyFixedKindItem(QGraphicsItem *item, bool show, ShownKinds::Kind kind)
		{
			if (!item) return;
			if (!show) {
				if (item->isVisibleTo(item->parentItem())) {
					item->setVisible(false);
					item->setData(LayerHiddenKey, true);
				} else if (item->data(LayerHiddenKey).toBool()) {
					item->setVisible(false);
				}
			} else {
				if (item->data(LayerHiddenKey).toBool()) {
					item->setData(LayerHiddenKey, QVariant());
					ShownKinds::setVisible(item, true);
				}
			}
		}
	}

	QString fixedKindName(FixedKind kind)
	{
		switch (kind) {
			case FixedKind::FrameAndTitleBlock:
				return QStringLiteral("Frame and title block");
			case FixedKind::WireNumbers:
				return QStringLiteral("Wire numbers");
			case FixedKind::SymbolTexts:
				return QStringLiteral("Symbol texts");
			case FixedKind::CrossReferences:
				return QStringLiteral("Cross-references");
			default:
				return QString();
		}
	}

	QString fixedKindDisplayName(FixedKind kind)
	{
		switch (kind) {
			case FixedKind::FrameAndTitleBlock:
				return QObject::tr("Frame and title block");
			case FixedKind::WireNumbers:
				return QObject::tr("Wire numbers");
			case FixedKind::SymbolTexts:
				return QObject::tr("Symbol texts");
			case FixedKind::CrossReferences:
				return QObject::tr("Cross-references");
			default:
				return QString();
		}
	}

	QString fixedKindAttribute(FixedKind kind)
	{
		switch (kind) {
			case FixedKind::FrameAndTitleBlock:
				return QStringLiteral("frame_titleblock");
			case FixedKind::WireNumbers:
				return QStringLiteral("wire_numbers");
			case FixedKind::SymbolTexts:
				return QStringLiteral("symbol_texts");
			case FixedKind::CrossReferences:
				return QStringLiteral("cross_references");
			default:
				return QString();
		}
	}

	FixedKind fixedKindFromAttribute(const QString &attr)
	{
		if (attr == QLatin1String("frame_titleblock")) return FixedKind::FrameAndTitleBlock;
		if (attr == QLatin1String("wire_numbers")) return FixedKind::WireNumbers;
		if (attr == QLatin1String("symbol_texts")) return FixedKind::SymbolTexts;
		if (attr == QLatin1String("cross_references")) return FixedKind::CrossReferences;
		return FixedKind::None;
	}

	FixedKind fixedKindOf(const QString &name)
	{
		if (name == fixedKindName(FixedKind::FrameAndTitleBlock) || name == fixedKindDisplayName(FixedKind::FrameAndTitleBlock))
			return FixedKind::FrameAndTitleBlock;
		if (name == fixedKindName(FixedKind::WireNumbers) || name == fixedKindDisplayName(FixedKind::WireNumbers))
			return FixedKind::WireNumbers;
		if (name == fixedKindName(FixedKind::SymbolTexts) || name == fixedKindDisplayName(FixedKind::SymbolTexts))
			return FixedKind::SymbolTexts;
		if (name == fixedKindName(FixedKind::CrossReferences) || name == fixedKindDisplayName(FixedKind::CrossReferences))
			return FixedKind::CrossReferences;
		return FixedKind::None;
	}

	bool isFixed(const QString &name)
	{
		return fixedKindOf(name) != FixedKind::None;
	}

	QList<Layer> defaultFixedLayers()
	{
		return {
			{fixedKindName(FixedKind::FrameAndTitleBlock), true, FixedKind::FrameAndTitleBlock},
			{fixedKindName(FixedKind::WireNumbers),        true, FixedKind::WireNumbers},
			{fixedKindName(FixedKind::SymbolTexts),        true, FixedKind::SymbolTexts},
			{fixedKindName(FixedKind::CrossReferences),    true, FixedKind::CrossReferences}
		};
	}

	Set::Set()
	{
		ensureFixedLayers();
	}

	void Set::ensureFixedLayers()
	{
		const auto fixed = defaultFixedLayers();
		for (int i = 0 ; i < fixed.size() ; ++i) {
			const auto &fl = fixed.at(i);
			bool found = false;
			for (const Layer &l : layers) {
				if (l.fixed_kind == fl.fixed_kind || l.name == fl.name) {
					found = true;
					break;
				}
			}
			if (!found) {
				layers.insert(i, fl);
			}
		}
	}

	QList<Layer> Set::userLayers() const
	{
		QList<Layer> result;
		for (const Layer &l : layers) {
			if (!l.isFixed()) result.append(l);
		}
		return result;
	}

	QList<Layer> Set::fixedLayers() const
	{
		QList<Layer> result;
		for (const Layer &l : layers) {
			if (l.isFixed()) result.append(l);
		}
		return result;
	}

	bool Set::isEmpty() const
	{
		if (!groups.isEmpty()) return false;
		if (!userLayers().isEmpty()) return false;
		for (const Layer &layer : layers) {
			if (layer.isFixed() && !layer.visible) return false;
		}
		return true;
	}

	bool Set::contains(const QString &name) const
	{
		if (isFixed(name)) return true;
		for (const Layer &layer : layers)
			if (layer.name == name) return true;
		return false;
	}

	bool Set::isVisible(FixedKind kind) const
	{
		if (kind == FixedKind::None) return true;
		for (const Layer &layer : layers) {
			if (layer.fixed_kind == kind || layer.name == fixedKindName(kind))
				return layer.visible;
		}
		return true;
	}

	bool Set::isVisible(const QString &name) const
	{
		const FixedKind fk = fixedKindOf(name);
		if (fk != FixedKind::None) return isVisible(fk);
		for (const Layer &layer : layers)
			if (layer.name == name) return layer.visible;
		return true;
	}

	QStringList Set::names() const
	{
		QStringList list;
		for (const Layer &layer : layers) list << layer.name;
		return list;
	}

	Set Set::showing(const Group &group) const
	{
		Set set = *this;
		for (Layer &layer : set.layers) layer.visible = group.layers.contains(layer.name);
		return set;
	}

	QDomElement Set::toXml(QDomDocument &document) const
	{
		QDomElement root = document.createElement(QStringLiteral("layers"));
		for (const Layer &layer : layers) {
			QDomElement element = document.createElement(QStringLiteral("layer"));
			element.setAttribute(QStringLiteral("name"), layer.name);
			element.setAttribute(QStringLiteral("visible"), layer.visible ? QStringLiteral("true") : QStringLiteral("false"));
			if (layer.isFixed()) {
				element.setAttribute(QStringLiteral("fixed"), fixedKindAttribute(layer.fixed_kind));
			}
			root.appendChild(element);
		}
		for (const Group &group : groups) {
			QDomElement element = document.createElement(QStringLiteral("layer_group"));
			element.setAttribute(QStringLiteral("name"), group.name);
			for (const QString &name : group.layers) {
				QDomElement member = document.createElement(QStringLiteral("member"));
				member.setAttribute(QStringLiteral("name"), name);
				element.appendChild(member);
			}
			root.appendChild(element);
		}
		return root;
	}

	Set Set::fromXml(const QDomElement &element)
	{
		Set set;
		if (element.isNull()) return set;

		for (QDomElement e = element.firstChildElement(QStringLiteral("layer")) ; !e.isNull()
			 ; e = e.nextSiblingElement(QStringLiteral("layer"))) {
			const QString name = e.attribute(QStringLiteral("name")).trimmed();
			if (name.isEmpty()) continue;
			const bool visible = (e.attribute(QStringLiteral("visible")) != QLatin1String("false"));
			FixedKind fk = FixedKind::None;
			if (e.hasAttribute(QStringLiteral("fixed"))) {
				fk = fixedKindFromAttribute(e.attribute(QStringLiteral("fixed")));
			} else {
				fk = fixedKindOf(name);
			}

			bool found = false;
			for (Layer &l : set.layers) {
				if ((fk != FixedKind::None && l.fixed_kind == fk) || l.name == name) {
					l.visible = visible;
					l.fixed_kind = fk;
					found = true;
					break;
				}
			}
			if (!found) {
				set.layers.append({name, visible, fk});
			}
		}
		set.groups.clear();
		for (QDomElement e = element.firstChildElement(QStringLiteral("layer_group")) ; !e.isNull()
			 ; e = e.nextSiblingElement(QStringLiteral("layer_group"))) {
			Group group;
			group.name = e.attribute(QStringLiteral("name")).trimmed();
			for (QDomElement m = e.firstChildElement(QStringLiteral("member")) ; !m.isNull()
				 ; m = m.nextSiblingElement(QStringLiteral("member")))
				group.layers << m.attribute(QStringLiteral("name"));
			if (!group.name.isEmpty()) set.groups << group;
		}
		return set;
	}

	QString layerOf(const QGraphicsItem *item)
	{
		return item ? item->data(LayerKey).toString() : QString();
	}

	void setLayerOf(QGraphicsItem *item, const QString &layer)
	{
		if (!item) return;
		item->setData(LayerKey, layer.isEmpty() ? QVariant() : QVariant(layer));
	}

	void writeAttribute(const QGraphicsItem *item, QDomElement &element)
	{
		const QString layer = layerOf(item);
		if (!layer.isEmpty()) element.setAttribute(QStringLiteral("layer"), layer);
	}

	void readAttribute(QGraphicsItem *item, const QDomElement &element)
	{
		setLayerOf(item, element.attribute(QStringLiteral("layer")));
	}

	bool canBeOnLayer(const QGraphicsItem *item)
	{
			//A structure box is a shape with a type of its own
		return qgraphicsitem_cast<const QetShapeItem *>(item)
				|| qgraphicsitem_cast<const StructureBoxItem *>(item)
				|| qgraphicsitem_cast<const IndependentTextItem *>(item)
				|| qgraphicsitem_cast<const DiagramImageItem *>(item);
	}

	/**
		@return whether @p item is hidden by its layer, for code walking the
		items itself (DXF export)
	*/
	bool isHidden(const QGraphicsItem *item)
	{
		for (const QGraphicsItem *p = item; p; p = p->parentItem()) {
			if (!layerOf(p).isEmpty() && !p->isVisible()) return true;
			if (p->data(LayerHiddenKey).toBool()) return true;
		}
		return false;
	}

	/**
		@brief apply
		Shows or hides @p item as its layer says; through ShownKinds, so
		that View > Show and the layers each only veto.
	*/
	void apply(QGraphicsItem *item, const Set &set)
	{
		if (!canBeOnLayer(item)) return;
		ShownKinds::setVisible(item, set.isVisible(layerOf(item)));
	}

	void applyTo(Diagram *folio, const Set &set)
	{
		if (!folio) return;

		const bool show_frame = set.isVisible(FixedKind::FrameAndTitleBlock);
		if (folio->useBorder() != show_frame) {
			folio->setUseBorder(show_frame);
			folio->update();
		}

		const bool show_wires = set.isVisible(FixedKind::WireNumbers);
		const bool show_symbols = set.isVisible(FixedKind::SymbolTexts);
		const bool show_xrefs = set.isVisible(FixedKind::CrossReferences);

		for (QGraphicsItem *item : folio->items()) {
			if (canBeOnLayer(item)) {
				apply(item, set);
				continue;
			}
			const QVariant val = item->data(ShownKinds::data_key);
			if (!val.isValid()) continue;
			const int kind = val.toInt();
			if (kind == ShownKinds::WireNumbers) {
				applyFixedKindItem(item, show_wires, ShownKinds::WireNumbers);
			} else if (kind == ShownKinds::SymbolTexts) {
				applyFixedKindItem(item, show_symbols, ShownKinds::SymbolTexts);
			} else if (kind == ShownKinds::CrossReferences) {
				applyFixedKindItem(item, show_xrefs, ShownKinds::CrossReferences);
			}
		}
	}
}
