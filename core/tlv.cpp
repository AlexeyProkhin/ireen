/****************************************************************************
**
** Ireen — cross-platform OSCAR protocol library
**
** Copyright © 2012 Ruslan Nigmatullin <euroelessar@yandex.ru>
**                  Alexey Prokhin <alexey.prokhin@yandex.ru>
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

#include "tlv.h"
#include <QDataStream>

namespace Ireen {

quint32 TLVMap::valuesSize() const
{
	quint32 size = 0;
	foreach(const TLV &tlv, *this)
		size = size + tlv.data().size() + 4;
	return size;
}

TLVMap::operator QByteArray() const
{
	QByteArray data;
	foreach(const TLV &tlv, *this)
		data += tlv;
	return data;
}

} // namespace Ireen

