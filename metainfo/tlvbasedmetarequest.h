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

#ifndef IREEN_TLVBASEDMETAREQUEST_H
#define IREEN_TLVBASEDMETAREQUEST_H

#include "abstractmetarequest.h"

namespace Ireen {

class Client;
class TlvBasedMetaRequestPrivate;

class IREEN_EXPORT TlvBasedMetaRequest : public AbstractMetaRequest
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(TlvBasedMetaRequest)
public:
	void setValue(const MetaFieldKey &key, const QVariant &value);
protected:
	TlvBasedMetaRequest(MetaInfo *metaInfo, TlvBasedMetaRequestPrivate *d);
	void sendTlvBasedRequest(quint16 type) const;
};

} // namespace Ireen

#endif // IREEN_TLVBASEDMETAINFOREQUEST_H

