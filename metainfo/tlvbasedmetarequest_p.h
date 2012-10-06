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

#ifndef IREEN_TLVBASEDMETAREQUEST_P_H
#define IREEN_TLVBASEDMETAREQUEST_P_H

#include "tlvbasedmetarequest.h"
#include "abstractmetarequest_p.h"
#include "../core/tlv.h"

namespace Ireen {

class TlvBasedMetaRequestPrivate : public AbstractMetaRequestPrivate
{
public:
	void addString(quint16 id, MetaFieldEnum key, DataUnit &data, bool test = true) const;
	void addString(const QString &str, DataUnit &data) const;
	template <typename T>
	void addField(quint16 id, MetaFieldEnum key, DataUnit &data, bool test = true) const;
	template <typename T>
	void addCategoryId(quint16 id, MetaFieldEnum key, DataUnit &data, const FieldNamesList &list) const;
	void addCategory(quint16 id, MetaFieldEnum key, DataUnit &data, const FieldNamesList &list) const;
	MetaInfoValuesHash values;
};

} // namespace Ireen

#endif // IREEN_TLVBASEDMETAINFOREQUEST_P_H

