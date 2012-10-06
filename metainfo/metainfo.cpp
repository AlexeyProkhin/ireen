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

#include "metainfo_p.h"
#include "metafields_p.h"
#include <QStringList>
#include <QDate>
#include <QEventLoop>

namespace Ireen {

void MetaInfoPrivate::addRequest(AbstractMetaRequest *request)
{
	requests.insert(request->id(), request);
}

bool MetaInfoPrivate::removeRequest(AbstractMetaRequest *request)
{
	return requests.remove(request->id()) > 0;
}

MetaInfo::MetaInfo(Client *client) :
	d(new MetaInfoPrivate)
{
	d->sequence = 0;
	d->client = client;
	m_infos << SNACInfo(ExtensionsFamily, ExtensionsMetaSrvReply)
			<< SNACInfo(ExtensionsFamily, ExtensionsMetaError);
	client->registerHandler(this);
	connect(client, SIGNAL(disconnected()),
			SLOT(onDisconnected()));
}

MetaInfo::~MetaInfo()
{
}

Client *MetaInfo::client() const
{
	return d->client;
}

void MetaInfo::handleSNAC(AbstractConnection *conn, const SNAC &snac)
{
	Q_UNUSED(conn);
	if (snac.family() == ExtensionsFamily && snac.subtype() == ExtensionsMetaSrvReply) {
		TLVMap tlvs = snac.read<TLVMap>();
		if (tlvs.contains(0x01)) {
			DataUnit data(tlvs.value(0x01));
			data.skipData(6); // skip field length + my uin
			quint16 metaType = data.read<quint16>(LittleEndian);
			if (metaType == 0x07da) {
				quint16 reqNumber = data.read<quint16>(LittleEndian);
				QHash<quint16, AbstractMetaRequest*>::iterator itr = d->requests.find(reqNumber);
				quint16 dataType = data.read<quint16>(LittleEndian);
				quint8 success = data.read<quint8>(LittleEndian);
				if (itr == d->requests.end()) {
					debug() << "Unexpected metainfo response" << reqNumber;
					return;
				}
				if (success == 0x0a) {
					if (!itr.value()->handleData(dataType, data))
						debug() << "Unexpected metainfo response with type" << hex << dataType;
				} else {
					debug() << "Meta request failed" << hex << success;
					itr.value()->close(false, AbstractMetaRequest::ProtocolError, tr("Incorrect format of the metarequest"));
				}
			}
		}
	} else if (snac.family() == ExtensionsFamily && snac.subtype() == ExtensionsMetaError) {
		ProtocolError error(snac);
		debug() << QString("MetaInfo service error (%1, %2): %3")
				.arg(error.code(), 2, 16)
				.arg(error.subcode(), 2, 16)
				.arg(error.errorString());
		if (error.tlvs().contains(0x21)) {
			DataUnit data(error.tlvs().value(0x21));
			data.skipData(6); // skip field length + my uin
			quint16 metaType = data.read<quint16>(LittleEndian);
			if (metaType == 0x07d0) {
				quint16 reqNumber = data.read<quint16>(LittleEndian);
				AbstractMetaRequest *request = d->requests.value(reqNumber);
				if (request) {
					request->close(false, AbstractMetaRequest::ProtocolError, error.errorString());
				}
			}
		}
	}
}

void MetaInfo::onDisconnected()
{
	QHash<quint16, AbstractMetaRequest*> requests = d->requests;
	foreach (AbstractMetaRequest *req, requests)
		req->close(false);
	Q_ASSERT(d->requests.isEmpty());
}

} // namespace Ireen
