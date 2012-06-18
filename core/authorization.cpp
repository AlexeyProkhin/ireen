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

#include "authorization.h"

namespace Ireen {

AuthorizationRequest::AuthorizationRequest(const QString &uin, const QString &reason) :
	SNAC(ListsFamily, ListsRequestAuth)
{
	append<quint8>(uin);
	append<quint16>(reason);
	append<quint16>(0); // unknown
}

AuthorizationReply::AuthorizationReply(const QString &uin, bool isAccepted, const QString &reason) :
	SNAC(ListsFamily, ListsCliAuthResponse)
{
	append<quint8>(uin);
	append<quint8>(isAccepted ? 0x01 : 0x00);
	append<quint16>(reason);
	append<quint16>(0); // unknown
}

GrantAuthorization::GrantAuthorization(const QString &uin, const QString &reason) :
	SNAC(ListsFamily, ListsGrantAuth)
{
	append<quint8>(uin);
	append<quint16>(reason);
	append<quint16>(0); // unknown
}


} // namespace Ireen
