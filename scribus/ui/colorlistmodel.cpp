/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "colorlistmodel.h"

#include "colorlistbox.h"
#include "commonstrings.h"

ColorPixmapValue ColorListModel::m_NoneColor(ScColor(), 0, CommonStrings::None);

ColorListModel::ColorListModel(QObject *parent)
	          : QAbstractItemModel(parent)
{
	m_isNoneColorShown = false;
	m_sortRule = SortByName;
}

void ColorListModel::clear()
{
	beginResetModel();
	m_colors.clear();
	endResetModel();
}

int ColorListModel::columnCount(const QModelIndex &/*parent*/) const
{
	return 1;
}

QVariant ColorListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	ColorPixmapValue* pColorValue = static_cast<ColorPixmapValue*>(index.internalPointer());
	if (!pColorValue)
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		if (pColorValue->m_name == CommonStrings::None)
			return CommonStrings::tr_NoneColor;
		return pColorValue->m_name;
	}

	if (role == Qt::ToolTipRole)
	{
		const ScColor& color = pColorValue->m_color;
		if (color.getColorModel() == colorModelRGB)
		{
			int r, g, b;
			color.getRawRGBColor(&r, &g, &b);
			return tr("R: %1 G: %2 B: %3").arg(r).arg(g).arg(b);
		}
		else if (color.getColorModel() == colorModelCMYK)
		{
			int c, m, y, k;
			color.getCMYK(&c, &m, &y, &k);
			return tr("C: %1% M: %2% Y: %3% K: %4%").arg(qRound(c / 2.55)).arg(qRound(m / 2.55)).arg(qRound(y / 2.55)).arg(qRound(k / 2.55));
		}
		else if (color.getColorModel() == colorModelLab)
		{
			double L, a, b;
			color.getLab(&L, &a, &b);
			return tr("L: %1% a: %2% b: %3%").arg(L, 0, 'f', 2).arg(a, 0, 'f', 2).arg(b, 0, 'f', 2);
		}
		return QVariant();
	}

	if (role == Qt::UserRole)
	{
		if (pColorValue->m_name == CommonStrings::None)
			return CommonStrings::None;
		return QVariant::fromValue(*pColorValue);
	}

	return QVariant();
}

Qt::ItemFlags ColorListModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	return flags;
}

QModelIndex ColorListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	ColorPixmapValue* pColorValue = static_cast<ColorPixmapValue*>(parent.internalPointer());
	if (pColorValue)
		return QModelIndex();

	int colCount = columnCount();
	if (row < 0 || (row >= rowCount()) || (column != 0))
		return QModelIndex();

	const ColorPixmapValue& colorValue = m_colors.at(row);
	return createIndex(row, column, const_cast<ColorPixmapValue*>(&colorValue));
}

void ColorListModel::insert(int i, const ColorPixmapValue& value)
{
	beginInsertRows(QModelIndex(), i, i);
	m_colors.insert(i, value);
	endInsertRows();
}

QModelIndex ColorListModel::parent(const QModelIndex &/*child*/) const
{
	return QModelIndex();
}

bool ColorListModel::removeRow(int row, const QModelIndex& parent)
{
	if (row < 0 || row >= rowCount())
		return false;

	beginRemoveRows(parent, row, row);
	m_colors.remove(row);
	endRemoveRows();

	return true;
}

bool ColorListModel::removeRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || row >= rowCount())
		return false;
	if (count <= 0)
		return false;

	beginRemoveRows(parent, row, row + count - 1);
	m_colors.remove(row, count);
	endRemoveRows();

	return true;
}

int ColorListModel::rowCount(const QModelIndex &parent) const
{
	if (m_colors.count() == 0)
		return 0;

	ColorPixmapValue* pColorValue = static_cast<ColorPixmapValue*>(parent.internalPointer());
	if (pColorValue)
		return 0;

	return m_colors.count();
}

void ColorListModel::setColorList(const ColorList& colorList)
{
	setColorList(colorList, m_isNoneColorShown);
}

void ColorListModel::setColorList(const ColorList& colorList, bool showNone)
{
	ScribusDoc* doc = colorList.document();

	beginResetModel();

	m_isNoneColorShown = showNone;
	m_colors.clear();
	m_colors.reserve(colorList.count());

	if (m_isNoneColorShown)
		m_colors.append(ColorPixmapValue(ScColor(), 0, CommonStrings::None));

	ColorList::const_iterator iter;
	for (iter = colorList.begin(); iter != colorList.end(); ++iter)
	{
		QString colorName = iter.key();
		const ScColor& color = iter.value();
		m_colors.append(ColorPixmapValue(color, doc, colorName));
	}

	if (m_sortRule != SortByName)
	{
		if (m_sortRule == SortByValues)
			qSort(m_colors.begin(), m_colors.end(), compareColorValues);
		else if (m_sortRule == SortByType)
			qSort(m_colors.begin(), m_colors.end(), compareColorTypes);
	}

	endResetModel();
}

void ColorListModel::setShowNoneColor(bool showNone)
{
	if (m_isNoneColorShown == showNone)
		return;

	beginResetModel();
	m_isNoneColorShown = showNone;
	endResetModel();
}

void ColorListModel::setSortRule(SortRule sortRule)
{
	if (m_sortRule == sortRule)
		return;

	beginResetModel();

	m_sortRule = sortRule;
	if (m_sortRule == SortByValues)
		qSort(m_colors.begin(), m_colors.end(), compareColorValues);
	else if (m_sortRule == SortByType)
		qSort(m_colors.begin(), m_colors.end(), compareColorTypes);
	else
		qSort(m_colors.begin(), m_colors.end(), compareColorNames);

	endResetModel();
}

bool ColorListModel::compareColorNames(const ColorPixmapValue& v1, const ColorPixmapValue& v2)
{
	if (v1.m_name == CommonStrings::None || v1.m_name == CommonStrings::tr_None)
		return true;
	if (v2.m_name == CommonStrings::None || v2.m_name == CommonStrings::tr_None)
		return false;

	return (v1.m_name < v2.m_name);
}

bool ColorListModel::compareColorValues(const ColorPixmapValue& v1, const ColorPixmapValue& v2)
{
	if (v1.m_name == CommonStrings::None || v1.m_name == CommonStrings::tr_None)
		return true;
	if (v2.m_name == CommonStrings::None || v2.m_name == CommonStrings::tr_None)
		return false;

	QColor c1 = v1.m_color.getRawRGBColor();
	QColor c2 = v2.m_color.getRawRGBColor();

	QString sortString1 = QString("%1-%2-%3-%4").arg(c1.hue(), 3, 10, QChar('0')).arg(c1.saturation(), 3, 10, QChar('0')).arg(c1.value(), 3, 10, QChar('0')).arg(v1.m_name);
	QString sortString2 = QString("%1-%2-%3-%4").arg(c2.hue(), 3, 10, QChar('0')).arg(c2.saturation(), 3, 10, QChar('0')).arg(c2.value(), 3, 10, QChar('0')).arg(v2.m_name);
	return (sortString1 < sortString2);
}

bool ColorListModel::compareColorTypes(const ColorPixmapValue& v1, const ColorPixmapValue& v2)
{
	if (v1.m_name == CommonStrings::None || v1.m_name == CommonStrings::tr_None)
		return true;
	if (v2.m_name == CommonStrings::None || v2.m_name == CommonStrings::tr_None)
		return false;

	QString sortString1 = QString("%1-%2");
	QString sortString2 = QString("%1-%2");

	if (v1.m_color.isRegistrationColor())
		sortString1 = sortString1.arg("A").arg(v1.m_name);
	else if (v1.m_color.isSpotColor())
		sortString1 = sortString1.arg("B").arg(v1.m_name);
	else if (v1.m_color.getColorModel() == colorModelCMYK)
		sortString1 = sortString1.arg("C").arg(v1.m_name);
	else
		sortString1 = sortString1.arg("D").arg(v1.m_name);

	if (v2.m_color.isRegistrationColor())
		sortString2 = sortString2.arg("A").arg(v2.m_name);
	else if (v2.m_color.isSpotColor())
		sortString2 = sortString2.arg("B").arg(v2.m_name);
	else if (v2.m_color.getColorModel() == colorModelCMYK)
		sortString2 = sortString2.arg("C").arg(v2.m_name);
	else
		sortString2 = sortString2.arg("D").arg(v2.m_name);

	return (sortString1 < sortString2);
}