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

#ifndef IREEN_METAINFO_H
#define IREEN_METAINFO_H

#include "../ireen_global.h"
#include "../core/snachandler.h"
#include <QScopedPointer>

namespace Ireen {

class Client;
class MetaInfoPrivate;

class MetaInfo : public QObject, public SNACHandler
{
	Q_OBJECT
	Q_INTERFACES(Ireen::SNACHandler)
public:
	MetaInfo(Client *client);
	~MetaInfo();
	Client *client() const;
protected:
	void handleSNAC(AbstractConnection *conn, const SNAC &snac);
private slots:
	void onDisconnected();
private:
	friend class AbstractMetaRequest;
	QScopedPointer<MetaInfoPrivate> d;
};

} // namespace Ireen

#endif // IREEN_METAINFO_H
