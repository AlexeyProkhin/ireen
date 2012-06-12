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

#ifndef IREEN_TLV_H
#define IREEN_TLV_H

#include <QByteArray>
#include <QString>
#include <QMap>
#include <QtEndian>
#include "ireen_global.h"
#include "util.h"
#include "dataunit.h"

class QDataStream;

namespace Ireen {

class TLV;
class TLVMap;

class IREEN_EXPORT TLV: public DataUnit
{
public:
	inline TLV(quint16 type = 0x0000);
	template<typename T>
	inline TLV(quint16 type, const T &data);
	inline quint16 type() const { return m_type; }
	inline void setType(quint16 type) { m_type = type; }
	inline QByteArray toByteArray(ByteOrder bo = BigEndian) const;
	inline operator QByteArray() const { return toByteArray(BigEndian); }
	static inline TLV fromByteArray(const QByteArray &data, ByteOrder bo = BigEndian);
private:
	quint16 m_type;
};

class TLVMap: public QMap<quint16, TLV>
{
public:
	inline TLVMap();
	inline TLVMap(const QMap<quint16, TLV> &other);
	inline TLV value(int key) const;
	template<typename T>
	T value(quint16 type, const T &def = T()) const;
	template<typename T>
	TLVMap::iterator insert(quint16 type, const T &data);
	inline TLVMap::iterator insert(quint16 type);
	inline TLVMap::iterator insert(const TLV &tlv);
	quint32 valuesSize() const;
	operator QByteArray() const;
	inline static TLVMap fromByteArray(const QByteArray &data, ByteOrder bo = BigEndian);
private:
	TLVMap::iterator insert(quint16 type, const TLV &data);
};


TLV::TLV(quint16 type)
{
	m_type = type;
	setMaxSize(0xffff);
}

template<typename T>
Q_INLINE_TEMPLATE TLV::TLV(quint16 type, const T &data):
	m_type(type)
{
	setMaxSize(0xffff);
	append(data);
}

QByteArray TLV::toByteArray(ByteOrder bo) const
{
	DataUnit data;
	data.append<quint16>(m_type, bo);
	data.append<quint16>(m_data, bo);
	return data.data();
}

TLV TLV::fromByteArray(const QByteArray &data, ByteOrder bo)
{
   return DataUnit(data).read<TLV>(bo);
}

TLVMap::TLVMap()
{
}

TLVMap::TLVMap(const QMap<quint16, TLV> &other) :
	QMap<quint16, TLV>(other)
{
}

TLV TLVMap::value(int key) const
{
	return QMap<quint16, TLV>::value(key);
}

template<typename T>
Q_INLINE_TEMPLATE T TLVMap::value(quint16 type, const T &def) const
{
	TLVMap::const_iterator it = find(type);
	if (it != constEnd()) {
		it->resetState();
		return it->read<T>();
	} else {
		return def;
	}
}

template<typename T>
Q_INLINE_TEMPLATE TLVMap::iterator TLVMap::insert(quint16 type, const T &data)
{
	return QMap<quint16, TLV>::insert(type, TLV(type, data));
}

TLVMap::iterator TLVMap::insert(quint16 type)
{
	return QMap<quint16, TLV>::insert(type, TLV(type));
}

TLVMap::iterator TLVMap::insert(const TLV &tlv)
{
	return QMap<quint16, TLV>::insert(tlv.type(), tlv);
}

TLVMap TLVMap::fromByteArray(const QByteArray &data, ByteOrder bo)
{
	return DataUnit(data).read<TLVMap>(bo);
}

void DataUnit::appendTLV(quint16 type, ByteOrder bo)
{
	append(TLV(type), bo);
}

template<typename T>
Q_INLINE_TEMPLATE void DataUnit::appendTLV(quint16 type, const T &data, ByteOrder bo)
{
	append(TLV(type, data), bo);
}

template<>
Q_INLINE_TEMPLATE void DataUnit::appendTLV(quint16 type, const DataUnit &data, ByteOrder bo)
{
	append(TLV(type, data.data()), bo);
}

template<>
struct fromDataUnitHelper<TLV>
{
	static inline TLV fromByteArray(const DataUnit &d, ByteOrder bo = BigEndian)
	{
		TLV tlv(0xffff);
		if (d.dataSize() < 4)
			return tlv;
		tlv.setType(d.read<quint16>(bo));
		if (d.dataSize() < 2)
			tlv.setType(0xffff);
		else
			tlv.append(d.read<QByteArray, quint16>(bo));
		return tlv;
	}
};

template<>
struct fromDataUnitHelper<TLVMap>
{
	static inline TLVMap fromByteArray(const DataUnit &d, ByteOrder bo = BigEndian)
	{
		TLVMap tlvs;
		forever {
			TLV tlv = fromDataUnitHelper<TLV>::fromByteArray(d, bo);
			if (tlv.type() == 0xffff)
				return tlvs;
			tlvs.insert(tlv);
		}
		return tlvs;
	}
	template<class L>
	static inline TLVMap fromByteArray(const DataUnit &d, L count, ByteOrder bo = BigEndian)
	{
		TLVMap tlvs;
		for (L i = 0; i < count; i++) {
			TLV tlv = fromDataUnitHelper<TLV>::fromByteArray(d, bo);
			if (tlv.type() == 0xffff)
				return tlvs;
			tlvs.insert(tlv);
		}
		return tlvs;
	}
};

template<>
struct toDataUnitHelper<TLV>
{
	static inline QByteArray toByteArray(TLV data, ByteOrder bo = BigEndian)
	{
		return data.toByteArray(bo);
	}
};

inline QDebug &operator<<(QDebug &stream, const TLV &data)
{
	stream.nospace() << hex << "(" << data.type() << ", " << data.data().toHex() << ")";
	return stream;
}

} // namespace Ireen

#endif // IREEN_TLV_H

