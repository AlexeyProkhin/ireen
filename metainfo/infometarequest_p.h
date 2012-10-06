/****************************************************************************
**
** Ireen — cross-platform OSCAR protocol library
**
** Copyright © 2012 Alexey Prokhin <alexey.prokhin@yandex.ru>
**
*****************************************************************************
**
** $IREEN_BEGIN_LICENSE$
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as
** published by the Free Software Foundation, either version 3
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see http://www.gnu.org/licenses/.
** $IREEN_END_LICENSE$
**
****************************************************************************/

#ifndef IREEN_INFOMETAREQUEST_P_H
#define IREEN_INFOMETAREQUEST_P_H

#include "infometarequest.h"
#include "abstractmetarequest_p.h"

namespace Ireen {

class ShortInfoMetaRequestPrivate : public AbstractMetaRequestPrivate
{
public:
	MetaInfoValuesHash values;
	quint32 uin;
	inline void readString(MetaFieldEnum value, const DataUnit &data);
	inline void readFlag(MetaFieldEnum value, const DataUnit &data);
	void dump();
};

class FullInfoMetaRequestPrivate : public ShortInfoMetaRequestPrivate
{
public:
	template <typename T>
	void readField(MetaFieldEnum value, const DataUnit &data, const FieldNamesList &list);
	void readCategories(MetaFieldEnum value, const DataUnit &data, const FieldNamesList &list);
	void handleBasicInfo(const DataUnit &data);
	void handleMoreInfo(const DataUnit &data);
	void handleEmails(const DataUnit &data);
	void handleHomepage(const DataUnit &data);
	void handleWork(const DataUnit &data);
};

} // namespace Ireen

#endif // IREEN_INFOMETAREQUEST_P_H

