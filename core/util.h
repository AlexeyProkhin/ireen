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

#ifndef IREEN_UTIL_H
#define IREEN_UTIL_H

#include "ireen_global.h"
#include <QByteArray>
#include <QtEndian>
#include <QTextCodec>

#define IMPLEMENT_ME QString("Function %1 at line %2: implement me.").arg(Q_FUNC_INFO).arg(__LINE__)

namespace Ireen {

namespace Util {

template<typename T>
inline QByteArray toBigEndian(T source)
{
	QByteArray data;
	data.resize(sizeof(T));
	qToBigEndian(source, (uchar *) data.data());
	return data;
}
template<typename T>
inline QByteArray toLittleEndian(T source)
{
	QByteArray data;
	data.resize(sizeof(T));
	qToLittleEndian(source, (uchar *) data.data());
	return data;
}

IREEN_EXPORT QTextCodec *asciiCodec();
extern void setAsciiCodec(QTextCodec *codec);
IREEN_EXPORT QTextCodec *utf8Codec();
IREEN_EXPORT QTextCodec *utf16Codec();
IREEN_EXPORT QTextCodec *defaultCodec();
IREEN_EXPORT QTextCodec *detectCodec();

} } // namespace Ireen::Util

#endif // IREEN_UTIL_H

