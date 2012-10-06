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

#ifndef IREEN_METAINFO_P_H
#define IREEN_METAINFO_P_H

#include "metainfo.h"
#include "abstractmetarequest_p.h"
#include "../client.h"

namespace Ireen {

class MetaInfoPrivate
{
public:
	void addRequest(AbstractMetaRequest *request);
	bool removeRequest(AbstractMetaRequest *request);
	quint16 nextId() { return ++sequence; }
public:
	quint16 sequence;
	Client *client;
	QHash<quint16, AbstractMetaRequest*> requests;
};

} // namespace Ireen

#endif // IREEN_METAINFO_P_H

