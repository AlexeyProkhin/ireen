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

#ifndef IREEN_INFOMETAREQUEST_H
#define IREEN_INFOMETAREQUEST_H

#include "abstractmetarequest.h"

namespace Ireen {

class IcqAccount;
class IcqContact;
class ShortInfoMetaRequestPrivate;
class FullInfoMetaRequestPrivate;

class IREEN_EXPORT ShortInfoMetaRequest : public AbstractMetaRequest
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(ShortInfoMetaRequest)
public:
	// If uin is empty, the information will be requested for the account
	ShortInfoMetaRequest(MetaInfo *metaInfo, const QString &uin = QString());
	MetaInfoValuesHash values() const;
	QVariant value(MetaFieldKey key, const QVariant &defaultValue = QVariant()) const;
	template <typename T>
	T value(MetaFieldKey key, const T &defaultValue = T());
	virtual void send() const;
protected:
	ShortInfoMetaRequest();
	virtual bool handleData(quint16 type, const DataUnit &data);
};

class IREEN_EXPORT FullInfoMetaRequest : public ShortInfoMetaRequest
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(FullInfoMetaRequest)
public:
	enum State {
		StateBasicInfo = 0x00c8,
		StateMoreInfo = 0x00dc,
		StateEmails = 0x00eb,
		StateHomepage = 0x010e,
		StateWork = 0x00d2,
		StateNotes = 0x00e6,
		StateInterests = 0x00f0,
		StateAffilations = 0x00fa
	};
	FullInfoMetaRequest(MetaInfo *metaInfo, const QString &uin = QString());
	virtual void send() const;
signals:
	void infoUpdated(State state);
protected:
	FullInfoMetaRequest();
	virtual bool handleData(quint16 type, const DataUnit &data);
};

template <typename T>
T ShortInfoMetaRequest::value(MetaFieldKey key, const T &defaultValue) {
	QVariant res = value(key);
	if (!res.isValid())
		return defaultValue;
	return res.value<T>();
}

} // namespace Ireen

#endif // IREEN_INFOMETAREQUEST_H

