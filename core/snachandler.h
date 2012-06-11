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

#ifndef IREEN_SNACHANDLER_H
#define IREEN_SNACHANDLER_H

#include <QObject>
#include <QPair>
#include "snac.h"
#include "util.h"

namespace Ireen {

class AbstractConnection;

typedef QPair<quint16, quint16> SNACInfo;

class IREEN_EXPORT SNACHandler
{
public:
	virtual ~SNACHandler();
	const QList<SNACInfo> &infos() { return m_infos; }
	virtual void handleSNAC(AbstractConnection *conn, const SNAC &snac) = 0;
protected:
	QList<SNACInfo> m_infos;
};

} // namespace Ireen

Q_DECLARE_INTERFACE(Ireen::SNACHandler, "org.qutim.ireen.SNACHandler")

#endif // IREEN_SNACHANDLER_H

