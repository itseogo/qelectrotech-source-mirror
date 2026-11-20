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
#include "../qetgraphicsitem/dynamicelementtextitem.h"
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
		const int LayerKey			= 0x4c4159; // "LAY"
		const int LayerHiddenKey	= 0x4c4148; // "LAH"
		const int LayerOrigColorKey = 0x4c4143; // "LAC"

		void applyColor(QGraphicsItem* item, const QColor& layer_color)
		{
			if (! item) return;

			if (auto* shape = dynamic_cast<QetShapeItem*>(item))
			{
				if (layer_color.isValid())
				{
					if (! item->data(LayerOrigColorKey).isValid())
					{
						item->setData(LayerOrigColorKey, shape->pen().color());
					}
					QPen p = shape->pen();
					if (p.color() != layer_color)
					{
						p.setColor(layer_color);
						shape->setPen(p);
					}
				}
				else if (item->data(LayerOrigColorKey).isValid())
				{
					const QColor orig =
						item->data(LayerOrigColorKey).value<QColor>();
					item->setData(LayerOrigColorKey, QVariant());
					QPen p = shape->pen();
					if (p.color() != orig)
					{
						p.setColor(orig);
						shape->setPen(p);
					}
				}
			}
			else if (auto* text_item = dynamic_cast<DiagramTextItem*>(item))
			{
				if (layer_color.isValid())
				{
					if (! item->data(LayerOrigColorKey).isValid())
					{
						item->setData(LayerOrigColorKey, text_item->color());
					}
					if (text_item->color() != layer_color)
					{
						text_item->setColor(layer_color);
					}
				}
				else if (item->data(LayerOrigColorKey).isValid())
				{
					const QColor orig =
						item->data(LayerOrigColorKey).value<QColor>();
					item->setData(LayerOrigColorKey, QVariant());
					if (text_item->color() != orig)
					{
						text_item->setColor(orig);
					}
				}
			}
		}

		void applyFixedKindItem(
			QGraphicsItem* item, bool show, ShownKinds::Kind kind)
		{
			if (! item) return;
			if (! show)
			{
				if (item->isVisibleTo(item->parentItem()))
				{
					item->setVisible(false);
					item->setData(LayerHiddenKey, true);
				}
				else if (item->data(LayerHiddenKey).toBool())
				{
					item->setVisible(false);
				}
			}
			else
			{
				if (item->data(LayerHiddenKey).toBool())
				{
					item->setData(LayerHiddenKey, QVariant());
					ShownKinds::setVisible(item, true);
				}
			}
		}
	} // namespace

	QString fixedKindName(FixedKind kind)
	{
		switch (kind)
		{
		case FixedKind::FrameAndTitleBlock: return QStringLiteral("10");
		case FixedKind::WireNumbers: return QStringLiteral("20");
		case FixedKind::SymbolTexts: return QStringLiteral("30");
		case FixedKind::CrossReferences: return QStringLiteral("40");
		case FixedKind::Graphics: return QStringLiteral("50");
		default: return QString();
		}
	}

	QString fixedKindDisplayName(FixedKind kind)
	{
		switch (kind)
		{
		case FixedKind::FrameAndTitleBlock:
			return QObject::tr("Frame and title block");
		case FixedKind::WireNumbers: return QObject::tr("Wire numbers");
		case FixedKind::SymbolTexts: return QObject::tr("Symbol texts");
		case FixedKind::CrossReferences: return QObject::tr("Cross-references");
		case FixedKind::Graphics: return QObject::tr("Graphics");
		default: return QString();
		}
	}

	QString fixedKindAttribute(FixedKind kind)
	{
		switch (kind)
		{
		case FixedKind::FrameAndTitleBlock:
			return QStringLiteral("frame_titleblock");
		case FixedKind::WireNumbers: return QStringLiteral("wire_numbers");
		case FixedKind::SymbolTexts: return QStringLiteral("symbol_texts");
		case FixedKind::CrossReferences:
			return QStringLiteral("cross_references");
		case FixedKind::Graphics: return QStringLiteral("graphics");
		default: return QString();
		}
	}

	FixedKind fixedKindFromAttribute(const QString& attr)
	{
		if (attr == QLatin1String("frame_titleblock"))
			return FixedKind::FrameAndTitleBlock;
		if (attr == QLatin1String("wire_numbers"))
			return FixedKind::WireNumbers;
		if (attr == QLatin1String("symbol_texts"))
			return FixedKind::SymbolTexts;
		if (attr == QLatin1String("cross_references"))
			return FixedKind::CrossReferences;
		if (attr == QLatin1String("graphics")) return FixedKind::Graphics;
		return FixedKind::None;
	}

	FixedKind fixedKindOf(const QString& name)
	{
		if (name == QStringLiteral("10")
			|| name == QStringLiteral("Frame and title block")
			|| name == fixedKindDisplayName(FixedKind::FrameAndTitleBlock))
			return FixedKind::FrameAndTitleBlock;
		if (name == QStringLiteral("20")
			|| name == QStringLiteral("Wire numbers")
			|| name == fixedKindDisplayName(FixedKind::WireNumbers))
			return FixedKind::WireNumbers;
		if (name == QStringLiteral("30")
			|| name == QStringLiteral("Symbol texts")
			|| name == fixedKindDisplayName(FixedKind::SymbolTexts))
			return FixedKind::SymbolTexts;
		if (name == QStringLiteral("40")
			|| name == QStringLiteral("Cross-references")
			|| name == fixedKindDisplayName(FixedKind::CrossReferences))
			return FixedKind::CrossReferences;
		if (name == QStringLiteral("50") || name == QStringLiteral("Graphics")
			|| name == QStringLiteral("Grafik")
			|| name == fixedKindDisplayName(FixedKind::Graphics))
			return FixedKind::Graphics;
		return FixedKind::None;
	}

	bool isFixed(const QString& name)
	{
		return fixedKindOf(name) != FixedKind::None;
	}

	QList<Layer> defaultFixedLayers()
	{
		return {
			{fixedKindName(FixedKind::FrameAndTitleBlock),
			 fixedKindDisplayName(FixedKind::FrameAndTitleBlock),
			 true,
			 true,
			 false,
			 FixedKind::FrameAndTitleBlock},
			{fixedKindName(FixedKind::WireNumbers),
			 fixedKindDisplayName(FixedKind::WireNumbers),
			 true,
			 true,
			 false,
			 FixedKind::WireNumbers},
			{fixedKindName(FixedKind::SymbolTexts),
			 fixedKindDisplayName(FixedKind::SymbolTexts),
			 true,
			 true,
			 false,
			 FixedKind::SymbolTexts},
			{fixedKindName(FixedKind::CrossReferences),
			 fixedKindDisplayName(FixedKind::CrossReferences),
			 true,
			 true,
			 false,
			 FixedKind::CrossReferences},
			{fixedKindName(FixedKind::Graphics),
			 fixedKindDisplayName(FixedKind::Graphics),
			 true,
			 true,
			 false,
			 FixedKind::Graphics}};
	}

	Set::Set() { ensureFixedLayers(); }

	void Set::ensureFixedLayers()
	{
		const auto fixed = defaultFixedLayers();
		for (int i = 0; i < fixed.size(); ++i)
		{
			const auto& fl	  = fixed.at(i);
			bool		found = false;
			for (const Layer& l : layers)
			{
				if (l.fixed_kind == fl.fixed_kind || l.name == fl.name)
				{
					found = true;
					break;
				}
			}
			if (! found) { layers.insert(i, fl); }
		}
	}

	QList<Layer> Set::userLayers() const
	{
		QList<Layer> result;
		for (const Layer& l : layers)
		{
			if (! l.isFixed()) result.append(l);
		}
		return result;
	}

	QList<Layer> Set::fixedLayers() const
	{
		QList<Layer> result;
		for (const Layer& l : layers)
		{
			if (l.isFixed()) result.append(l);
		}
		return result;
	}

	bool Set::isEmpty() const
	{
		if (! groups.isEmpty()) return false;
		if (! userLayers().isEmpty()) return false;
		for (const Layer& layer : layers)
		{
			if (layer.isFixed())
			{
				if (! layer.visible || ! layer.printable || layer.locked
					|| layer.color.isValid()
					|| (! layer.description.isEmpty()
						&& layer.description
							   != fixedKindDisplayName(layer.fixed_kind)))
					return false;
			}
		}
		return true;
	}

	QColor Set::color(FixedKind kind) const
	{
		if (kind == FixedKind::None) return QColor();
		for (const Layer& layer : layers)
		{
			if (layer.fixed_kind == kind || layer.name == fixedKindName(kind))
				return layer.color;
		}
		return QColor();
	}

	QColor Set::color(const QString& name) const
	{
		const FixedKind fk = fixedKindOf(name);
		if (fk != FixedKind::None) return color(fk);
		for (const Layer& layer : layers)
		{
			if (layer.name == name) return layer.color;
		}
		return QColor();
	}

	void Set::setColor(const QString& name, const QColor& color)
	{
		const FixedKind fk = fixedKindOf(name);
		for (Layer& layer : layers)
		{
			if (layer.name == name
				|| (fk != FixedKind::None && layer.fixed_kind == fk))
			{
				layer.color = color;
				return;
			}
		}
		if (fk == FixedKind::None)
		{
			Layer new_layer(name);
			new_layer.color = color;
			layers.append(new_layer);
		}
	}

	bool Set::contains(const QString& name) const
	{
		if (isFixed(name)) return true;
		for (const Layer& layer : layers)
			if (layer.name == name) return true;
		return false;
	}

	bool Set::isVisible(FixedKind kind) const
	{
		if (kind == FixedKind::None) return true;
		for (const Layer& layer : layers)
		{
			if (layer.fixed_kind == kind || layer.name == fixedKindName(kind))
				return layer.visible;
		}
		return true;
	}

	bool Set::isVisible(const QString& name) const
	{
		const FixedKind fk = fixedKindOf(name);
		if (fk != FixedKind::None) return isVisible(fk);
		for (const Layer& layer : layers)
			if (layer.name == name) return layer.visible;
		return true;
	}

	bool Set::isPrintable(FixedKind kind) const
	{
		if (kind == FixedKind::None) return true;
		for (const Layer& layer : layers)
		{
			if (layer.fixed_kind == kind || layer.name == fixedKindName(kind))
				return layer.printable;
		}
		return true;
	}

	bool Set::isPrintable(const QString& name) const
	{
		const FixedKind fk = fixedKindOf(name);
		if (fk != FixedKind::None) return isPrintable(fk);
		for (const Layer& layer : layers)
			if (layer.name == name) return layer.printable;
		return true;
	}

	bool Set::isLocked(FixedKind kind) const
	{
		if (kind == FixedKind::None) return false;
		for (const Layer& layer : layers)
		{
			if (layer.fixed_kind == kind || layer.name == fixedKindName(kind))
				return layer.locked;
		}
		return false;
	}

	bool Set::isLocked(const QString& name) const
	{
		const FixedKind fk = fixedKindOf(name);
		if (fk != FixedKind::None) return isLocked(fk);
		for (const Layer& layer : layers)
			if (layer.name == name) return layer.locked;
		return false;
	}

	QString Set::description(const QString& name) const
	{
		const FixedKind fk = fixedKindOf(name);
		for (const Layer& layer : layers)
		{
			if (layer.name == name
				|| (fk != FixedKind::None && layer.fixed_kind == fk))
				return layer.description;
		}
		if (fk != FixedKind::None) return fixedKindDisplayName(fk);
		for (const Layer& preset : standardPresetLayers())
		{
			if (preset.name == name) return preset.description;
		}
		return QString();
	}

	QStringList Set::names() const
	{
		QStringList list;
		for (const Layer& layer : layers) list << layer.name;
		return list;
	}

	Set Set::showing(const Group& group) const
	{
		Set set = *this;
		for (Layer& layer : set.layers)
			layer.visible = group.layers.contains(layer.name);
		return set;
	}

	Set Set::forPrinting() const
	{
		Set set = *this;
		for (Layer& layer : set.layers)
		{
			if (! layer.printable) { layer.visible = false; }
		}
		return set;
	}

	QDomElement Set::toXml(QDomDocument& document) const
	{
		QDomElement root = document.createElement(QStringLiteral("layers"));
		for (const Layer& layer : layers)
		{
			QDomElement element =
				document.createElement(QStringLiteral("layer"));
			element.setAttribute(QStringLiteral("name"), layer.name);
			if (! layer.description.isEmpty())
			{
				element.setAttribute(
					QStringLiteral("description"),
					layer.description);
			}
			element.setAttribute(
				QStringLiteral("visible"),
				layer.visible ? QStringLiteral("true") :
								QStringLiteral("false"));
			if (! layer.printable)
			{
				element.setAttribute(
					QStringLiteral("printable"),
					QStringLiteral("false"));
			}
			if (layer.locked)
			{
				element.setAttribute(
					QStringLiteral("locked"),
					QStringLiteral("true"));
			}
			if (layer.color.isValid())
			{
				element.setAttribute(
					QStringLiteral("color"),
					layer.color.name());
			}
			if (layer.isFixed())
			{
				element.setAttribute(
					QStringLiteral("fixed"),
					fixedKindAttribute(layer.fixed_kind));
			}
			root.appendChild(element);
		}
		for (const Group& group : groups)
		{
			QDomElement element =
				document.createElement(QStringLiteral("layer_group"));
			element.setAttribute(QStringLiteral("name"), group.name);
			for (const QString& name : group.layers)
			{
				QDomElement member =
					document.createElement(QStringLiteral("member"));
				member.setAttribute(QStringLiteral("name"), name);
				element.appendChild(member);
			}
			root.appendChild(element);
		}
		return root;
	}

	Set Set::fromXml(const QDomElement& element)
	{
		Set set;
		if (element.isNull()) return set;

		for (QDomElement e = element.firstChildElement(QStringLiteral("layer"));
			 ! e.isNull();
			 e = e.nextSiblingElement(QStringLiteral("layer")))
		{
			const QString name = e.attribute(QStringLiteral("name")).trimmed();
			if (name.isEmpty()) continue;
			const QString desc = e.attribute(QStringLiteral("description"));
			const bool	  visible =
				(e.attribute(QStringLiteral("visible"))
				 != QLatin1String("false"));
			const bool printable =
				(e.attribute(QStringLiteral("printable"))
				 != QLatin1String("false"));
			const bool locked =
				(e.attribute(QStringLiteral("locked"))
				 == QLatin1String("true"));
			const QColor color =
				e.hasAttribute(QStringLiteral("color")) ?
					QColor(e.attribute(QStringLiteral("color"))) :
					QColor();

			FixedKind fk = FixedKind::None;
			if (e.hasAttribute(QStringLiteral("fixed")))
			{
				fk = fixedKindFromAttribute(
					e.attribute(QStringLiteral("fixed")));
			}
			else
			{
				fk = fixedKindOf(name);
			}

			bool found = false;
			for (Layer& l : set.layers)
			{
				if ((fk != FixedKind::None && l.fixed_kind == fk)
					|| l.name == name)
				{
					l.description = desc.isEmpty() && fk != FixedKind::None ?
										fixedKindDisplayName(fk) :
										desc;
					l.color		  = color;
					l.visible	  = visible;
					l.printable	  = printable;
					l.locked	  = locked;
					l.fixed_kind  = fk;
					found		  = true;
					break;
				}
			}
			if (! found)
			{
				set.layers.append(
					{name, desc, visible, printable, locked, fk, color});
			}
		}
		set.groups.clear();
		for (QDomElement e =
				 element.firstChildElement(QStringLiteral("layer_group"));
			 ! e.isNull();
			 e = e.nextSiblingElement(QStringLiteral("layer_group")))
		{
			Group group;
			group.name = e.attribute(QStringLiteral("name")).trimmed();
			for (QDomElement m = e.firstChildElement(QStringLiteral("member"));
				 ! m.isNull();
				 m = m.nextSiblingElement(QStringLiteral("member")))
				group.layers << m.attribute(QStringLiteral("name"));
			if (! group.name.isEmpty()) set.groups << group;
		}
		return set;
	}

	QString layerOf(const QGraphicsItem* item)
	{
		return item ? item->data(LayerKey).toString() : QString();
	}

	void setLayerOf(QGraphicsItem* item, const QString& layer)
	{
		if (! item) return;
		item->setData(LayerKey, layer.isEmpty() ? QVariant() : QVariant(layer));
	}

	void writeAttribute(const QGraphicsItem* item, QDomElement& element)
	{
		const QString layer = layerOf(item);
		if (! layer.isEmpty())
			element.setAttribute(QStringLiteral("layer"), layer);
	}

	void readAttribute(QGraphicsItem* item, const QDomElement& element)
	{
		setLayerOf(item, element.attribute(QStringLiteral("layer")));
	}

	bool canBeOnLayer(const QGraphicsItem* item)
	{
		// A structure box is a shape with a type of its own
		return qgraphicsitem_cast<const QetShapeItem*>(item)
			   || qgraphicsitem_cast<const StructureBoxItem*>(item)
			   || qgraphicsitem_cast<const IndependentTextItem*>(item)
			   || qgraphicsitem_cast<const DiagramImageItem*>(item)
			   || qgraphicsitem_cast<const DynamicElementTextItem*>(item);
	}

	/**
		@return whether @p item is hidden by its layer, for code walking the
		items itself (DXF export)
	*/
	bool isHidden(const QGraphicsItem* item)
	{
		for (const QGraphicsItem* p = item; p; p = p->parentItem())
		{
			if (! layerOf(p).isEmpty() && ! p->isVisible()) return true;
			if (p->data(LayerHiddenKey).toBool()) return true;
		}
		return false;
	}

	/**
		@brief apply
		Shows or hides @p item as its layer says; through ShownKinds, so
		that View > Show and the layers each only veto.
	*/
	void apply(QGraphicsItem* item, const Set& set)
	{
		if (! canBeOnLayer(item)) return;
		const QString layer = layerOf(item);
		ShownKinds::setVisible(item, set.isVisible(layer));
		const bool locked = set.isLocked(layer);
		item->setFlag(QGraphicsItem::ItemIsMovable, ! locked);
		item->setFlag(QGraphicsItem::ItemIsSelectable, ! locked);
		if (locked && item->isSelected()) { item->setSelected(false); }

		const QColor layer_color = set.color(layer);
		applyColor(item, layer_color);
	}

	void applyTo(Diagram* folio, const Set& set)
	{
		if (! folio) return;

		const bool show_frame = set.isVisible(FixedKind::FrameAndTitleBlock);
		if (folio->useBorder() != show_frame)
		{
			folio->setUseBorder(show_frame);
			folio->update();
		}

		const bool show_wires	= set.isVisible(FixedKind::WireNumbers);
		const bool show_symbols = set.isVisible(FixedKind::SymbolTexts);
		const bool show_xrefs	= set.isVisible(FixedKind::CrossReferences);

		const bool lock_wires	= set.isLocked(FixedKind::WireNumbers);
		const bool lock_symbols = set.isLocked(FixedKind::SymbolTexts);
		const bool lock_xrefs	= set.isLocked(FixedKind::CrossReferences);

		for (QGraphicsItem* item : folio->items())
		{
			const bool is_dyn_text =
				qgraphicsitem_cast<const DynamicElementTextItem*>(item)
				!= nullptr;
			if (canBeOnLayer(item))
			{
				const QString item_layer = layerOf(item);
				if (! item_layer.isEmpty() || ! is_dyn_text)
				{
					apply(item, set);
					continue;
				}
			}
			const QVariant val = item->data(ShownKinds::data_key);
			if (! val.isValid()) continue;
			const int kind = val.toInt();
			if (kind == ShownKinds::WireNumbers)
			{
				applyFixedKindItem(item, show_wires, ShownKinds::WireNumbers);
				item->setFlag(QGraphicsItem::ItemIsMovable, ! lock_wires);
				item->setFlag(QGraphicsItem::ItemIsSelectable, ! lock_wires);
				if (lock_wires && item->isSelected()) item->setSelected(false);
				applyColor(item, set.color(FixedKind::WireNumbers));
			}
			else if (kind == ShownKinds::SymbolTexts)
			{
				applyFixedKindItem(item, show_symbols, ShownKinds::SymbolTexts);
				item->setFlag(QGraphicsItem::ItemIsMovable, ! lock_symbols);
				item->setFlag(QGraphicsItem::ItemIsSelectable, ! lock_symbols);
				if (lock_symbols && item->isSelected())
					item->setSelected(false);
				applyColor(item, set.color(FixedKind::SymbolTexts));
			}
			else if (kind == ShownKinds::CrossReferences)
			{
				applyFixedKindItem(
					item,
					show_xrefs,
					ShownKinds::CrossReferences);
				item->setFlag(QGraphicsItem::ItemIsMovable, ! lock_xrefs);
				item->setFlag(QGraphicsItem::ItemIsSelectable, ! lock_xrefs);
				if (lock_xrefs && item->isSelected()) item->setSelected(false);
				applyColor(item, set.color(FixedKind::CrossReferences));
			}
		}
	}

	namespace
	{
		QString escapeCsvField(const QString& field, QChar delimiter)
		{
			if (field.contains(delimiter) || field.contains(QLatin1Char('"'))
				|| field.contains(QLatin1Char('\n'))
				|| field.contains(QLatin1Char('\r')))
			{
				QString escaped = field;
				escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
				return QStringLiteral("\"%1\"").arg(escaped);
			}
			return field;
		}

		QStringList parseCsvLine(const QString& line, QChar delimiter)
		{
			QStringList fields;
			QString		current;
			bool		in_quotes = false;
			for (int i = 0; i < line.length(); ++i)
			{
				const QChar c = line.at(i);
				if (c == QLatin1Char('"'))
				{
					if (in_quotes && i + 1 < line.length()
						&& line.at(i + 1) == QLatin1Char('"'))
					{
						current += QLatin1Char('"');
						++i;
					}
					else
					{
						in_quotes = ! in_quotes;
					}
				}
				else if (c == delimiter && ! in_quotes)
				{
					fields.append(current.trimmed());
					current.clear();
				}
				else
				{
					current += c;
				}
			}
			fields.append(current.trimmed());
			return fields;
		}

		bool parseBoolValue(const QString& val, bool default_val = true)
		{
			const QString s = val.trimmed().toLower();
			if (s.isEmpty()) return default_val;
			if (s == QLatin1String("true") || s == QLatin1String("1")
				|| s == QLatin1String("yes") || s == QLatin1String("ja")
				|| s == QLatin1String("[v]") || s == QLatin1String("[x]")
				|| s == QLatin1String("v") || s == QLatin1String("x")
				|| s == QLatin1String("on"))
			{
				return true;
			}
			if (s == QLatin1String("false") || s == QLatin1String("0")
				|| s == QLatin1String("no") || s == QLatin1String("nein")
				|| s == QLatin1String("[ ]") || s == QLatin1String("[]")
				|| s == QLatin1String("off"))
			{
				return false;
			}
			return default_val;
		}
	} // namespace

	QString exportToCsv(const Set& set)
	{
		QString result;
		result +=
			QStringLiteral("Name,Description,Visible,Printable,Locked,Color\n");
		for (const Layer& layer : set.layers)
		{
			QStringList fields;
			fields << escapeCsvField(layer.name, QLatin1Char(','));
			fields << escapeCsvField(layer.description, QLatin1Char(','));
			fields
				<< (layer.visible ? QStringLiteral("true") :
									QStringLiteral("false"));
			fields
				<< (layer.printable ? QStringLiteral("true") :
									  QStringLiteral("false"));
			fields
				<< (layer.locked ? QStringLiteral("true") :
								   QStringLiteral("false"));
			fields << (layer.color.isValid() ? layer.color.name() : QString());
			result += fields.join(QLatin1Char(',')) + QLatin1Char('\n');
		}
		return result;
	}

	bool
	importFromCsv(Set& set, const QString& csv_content, int* imported_count)
	{
		if (imported_count) *imported_count = 0;
		const QStringList lines =
			csv_content.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")));
		if (lines.isEmpty()) return false;

		// Detect delimiter
		QChar		  delimiter	 = QLatin1Char(',');
		const QString first_line = lines.first();
		if (first_line.contains(QLatin1Char('\t')))
			delimiter = QLatin1Char('\t');
		else if (first_line.contains(QLatin1Char(';')))
			delimiter = QLatin1Char(';');

		int name_col = -1, desc_col = -1, color_col = -1, visible_col = -1,
			print_col = -1, lock_col = -1;
		int start_line = 0;

		const QStringList header_fields = parseCsvLine(first_line, delimiter);
		for (int i = 0; i < header_fields.size(); ++i)
		{
			const QString h = header_fields.at(i).toLower();
			if (name_col < 0
				&& (h == QLatin1String("name") || h == QLatin1String("ebene")
					|| h == QLatin1String("layer")))
			{
				name_col = i;
			}
			else if (
				desc_col < 0
				&& (h.contains(QLatin1String("beschreib"))
					|| h.contains(QLatin1String("desc"))))
			{
				desc_col = i;
			}
			else if (
				color_col < 0
				&& (h.contains(QLatin1String("farb"))
					|| h.contains(QLatin1String("color"))
					|| h.contains(QLatin1String("colour"))))
			{
				color_col = i;
			}
			else if (
				visible_col < 0
				&& (h.contains(QLatin1String("sichtbar"))
					|| h.contains(QLatin1String("visible"))
					|| h.contains(QLatin1String("shown"))))
			{
				visible_col = i;
			}
			else if (
				print_col < 0
				&& (h.contains(QLatin1String("druck"))
					|| h.contains(QLatin1String("print"))))
			{
				print_col = i;
			}
			else if (
				lock_col < 0
				&& (h.contains(QLatin1String("sperr"))
					|| h.contains(QLatin1String("lock"))))
			{
				lock_col = i;
			}
		}

		if (name_col >= 0) { start_line = 1; }
		else
		{
			// No recognized header row: assume default column order
			name_col	= 0;
			desc_col	= 1;
			color_col	= 2;
			visible_col = 3;
			print_col	= 4;
			lock_col	= 5;
		}

		int count = 0;
		for (int l = start_line; l < lines.size(); ++l)
		{
			const QString line = lines.at(l).trimmed();
			if (line.isEmpty()) continue;
			const QStringList fields = parseCsvLine(line, delimiter);
			if (name_col >= fields.size()) continue;

			const QString name = fields.at(name_col).trimmed();
			if (name.isEmpty()) continue;

			const QString desc = (desc_col >= 0 && desc_col < fields.size()) ?
									 fields.at(desc_col).trimmed() :
									 QString();
			QColor		  layer_color;
			if (color_col >= 0 && color_col < fields.size())
			{
				const QString c_str = fields.at(color_col).trimmed();
				if (! c_str.isEmpty() && c_str != QLatin1String("-5")
					&& c_str != QLatin1String("0"))
				{
					if (c_str.startsWith(QLatin1Char('#')))
						layer_color = QColor(c_str);
					else if (QColor::isValidColorName(c_str))
						layer_color = QColor(c_str);
				}
			}
			const bool visible =
				(visible_col >= 0 && visible_col < fields.size()) ?
					parseBoolValue(fields.at(visible_col), true) :
					true;
			const bool printable =
				(print_col >= 0 && print_col < fields.size()) ?
					parseBoolValue(fields.at(print_col), true) :
					true;
			const bool locked = (lock_col >= 0 && lock_col < fields.size()) ?
									parseBoolValue(fields.at(lock_col), false) :
									false;

			const FixedKind fk	  = fixedKindOf(name);
			bool			found = false;
			for (Layer& existing : set.layers)
			{
				if ((fk != FixedKind::None && existing.fixed_kind == fk)
					|| existing.name == name)
				{
					if (! desc.isEmpty()) existing.description = desc;
					if (layer_color.isValid()) existing.color = layer_color;
					existing.visible   = visible;
					existing.printable = printable;
					existing.locked	   = locked;
					found			   = true;
					break;
				}
			}
			if (! found)
			{
				set.layers.append(
					{name, desc, visible, printable, locked, fk, layer_color});
			}
			++count;
		}
		if (imported_count) *imported_count = count;
		return count > 0;
	}

	QByteArray exportToXml(const Set& set)
	{
		QDomDocument doc;
		doc.appendChild(set.toXml(doc));
		return doc.toByteArray(4);
	}

	bool
	importFromXml(Set& set, const QByteArray& xml_content, int* imported_count)
	{
		if (imported_count) *imported_count = 0;
		QDomDocument doc;
		if (! doc.setContent(xml_content)) return false;

		QDomElement layers_elem = doc.documentElement();
		if (layers_elem.tagName() != QLatin1String("layers"))
		{
			layers_elem =
				layers_elem.firstChildElement(QStringLiteral("layers"));
		}
		if (layers_elem.isNull()) return false;

		const Set imported_set = Set::fromXml(layers_elem);
		int		  count		   = 0;
		for (const Layer& imported_layer : imported_set.layers)
		{
			bool found = false;
			for (Layer& existing : set.layers)
			{
				if ((imported_layer.isFixed()
					 && existing.fixed_kind == imported_layer.fixed_kind)
					|| existing.name == imported_layer.name)
				{
					if (! imported_layer.description.isEmpty())
						existing.description = imported_layer.description;
					if (imported_layer.color.isValid())
						existing.color = imported_layer.color;
					existing.visible   = imported_layer.visible;
					existing.printable = imported_layer.printable;
					existing.locked	   = imported_layer.locked;
					found			   = true;
					break;
				}
			}
			if (! found) { set.layers.append(imported_layer); }
			++count;
		}

		for (const Group& group : imported_set.groups)
		{
			bool found = false;
			for (Group& existing_group : set.groups)
			{
				if (existing_group.name == group.name)
				{
					existing_group.layers = group.layers;
					found				  = true;
					break;
				}
			}
			if (! found) { set.groups.append(group); }
		}

		if (imported_count) *imported_count = count;
		return count > 0;
	}

	QList<Layer> standardPresetLayers()
	{
		return {
			{QStringLiteral("100"),
			 QObject::tr("Graphics.General"),
			 true,
			 true,
			 false},
			{QStringLiteral("101"),
			 QObject::tr("Graphics.Wide lines"),
			 true,
			 true,
			 false},
			{QStringLiteral("102"),
			 QObject::tr("Graphics.Thin lines"),
			 true,
			 true,
			 false},
			{QStringLiteral("107"),
			 QObject::tr("Graphics.Dimensions"),
			 true,
			 true,
			 false},
			{QStringLiteral("108"),
			 QObject::tr("Graphics.Texts"),
			 true,
			 true,
			 false},
			{QStringLiteral("109"),
			 QObject::tr("Graphics.Comments"),
			 true,
			 false,
			 false},
			{QStringLiteral("201"),
			 QObject::tr("Graphics.Folio border"),
			 true,
			 true,
			 false},
			{QStringLiteral("307"),
			 QObject::tr("Symbols.Structure boxes"),
			 true,
			 true,
			 false},
			{QStringLiteral("400"),
			 QObject::tr("Element texts.Device tags (BMK)"),
			 true,
			 true,
			 false},
			{QStringLiteral("405"),
			 QObject::tr("Conductor texts.Wire numbers"),
			 true,
			 true,
			 false},
			{QStringLiteral("420"),
			 QObject::tr("Element texts.Cross-references"),
			 true,
			 true,
			 false}};
	}

	QList<Group> standardPresetGroups()
	{
		QStringList without_bmk;
		without_bmk << fixedKindName(FixedKind::FrameAndTitleBlock);
		without_bmk << fixedKindName(FixedKind::WireNumbers);
		without_bmk << fixedKindName(FixedKind::CrossReferences);
		without_bmk << fixedKindName(FixedKind::Graphics);
		without_bmk << QStringLiteral("100") << QStringLiteral("101")
					<< QStringLiteral("102") << QStringLiteral("107")
					<< QStringLiteral("108") << QStringLiteral("109")
					<< QStringLiteral("201") << QStringLiteral("307")
					<< QStringLiteral("405") << QStringLiteral("420");
		return {{QObject::tr("Without device tags"), without_bmk}};
	}

	void loadStandardPresets(Set& set)
	{
		for (const Layer& preset : standardPresetLayers())
		{
			bool found = false;
			for (Layer& existing : set.layers)
			{
				if (existing.name == preset.name)
				{
					if (existing.description.isEmpty())
						existing.description = preset.description;
					found = true;
					break;
				}
			}
			if (! found) { set.layers.append(preset); }
		}

		for (const Group& preset_group : standardPresetGroups())
		{
			bool found = false;
			for (Group& existing_group : set.groups)
			{
				if (existing_group.name == preset_group.name)
				{
					found = true;
					break;
				}
			}
			if (! found) { set.groups.append(preset_group); }
		}
	}

	bool ensureLayer(Set& set, const QString& name)
	{
		if (name.isEmpty() || set.contains(name)) return false;
		for (const Layer& preset : standardPresetLayers())
		{
			if (preset.name == name)
			{
				set.layers.append(preset);
				return true;
			}
		}
		return false;
	}

	QList<QPair<QString, QString>> availableLayerChoices(const Set& set)
	{
		QList<QPair<QString, QString>> list;
		QSet<QString>				   seen;

		for (const Layer& layer : set.layers)
		{
			if (! seen.contains(layer.name))
			{
				seen.insert(layer.name);
				list.append({layer.name, layer.description});
			}
		}

		return list;
	}
} // namespace DrawingLayers
