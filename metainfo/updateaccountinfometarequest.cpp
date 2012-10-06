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

#include "updateaccountinfometarequest.h"
#include "tlvbasedmetarequest_p.h"

namespace Ireen {

class UpdateAccountInfoMetaRequestPrivate : public TlvBasedMetaRequestPrivate
{

};

UpdateAccountInfoMetaRequest::UpdateAccountInfoMetaRequest(MetaInfo *metaInfo, const MetaInfoValuesHash &values) :
	TlvBasedMetaRequest(metaInfo, new TlvBasedMetaRequestPrivate)
{
	Q_D(UpdateAccountInfoMetaRequest);
	d->values = values;
}

void UpdateAccountInfoMetaRequest::send() const
{
	sendTlvBasedRequest(0x0C3A);
}

bool UpdateAccountInfoMetaRequest::handleData(quint16 type, const DataUnit &data)
{
	Q_UNUSED(data);
	if (type == 0x0c3f) {
		debug() << "Account info successfully has been updated";
		emit infoUpdated();
		return true;
	}
	return false;
}

} // namespace Ireen
