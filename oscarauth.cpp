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

#include "oscarauth.h"
#include "client_p.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QNetworkProxy>
#if defined(OSCAR_USE_3RDPARTY_HMAC)
#ifdef Q_OS_HAIKU
# define SHA2_TYPES
#endif
# include "../3rdparty/hmac/hmac_sha2.h"
#elif defined(OSCAR_USE_QCA2)
# include <QtCrypto>
#else
# error Oscar authorization module depends on hmac-sha256 support
#endif

#include "k8json/k8json.h"

#define ICQ_LOGIN_URL "https://api.login.icq.net/auth/clientLogin"
#define ICQ_START_SESSION_URL "http://api.icq.net/aim/startOSCARSession"
#define DEBUG() if (!(*isDebug())) {} else debug()

namespace Ireen {

Q_GLOBAL_STATIC_WITH_ARGS(bool, isDebug, (qgetenv("QUTIM_OSCAR_DEBUG").toInt() > 0))

class OscarResponse
{
public:
	enum ResultCode
	{
		Success = 200,
		MoreAuthenticationRequired = 330,
		InvalidRequest = 400,
		AuthorizationRequired  = 401,
		MethodNoAllowed = 405,
		RequestTimeout = 408,
		SourceRateLimitReached = 430,
		InvalidKey = 440,
		KeyUsageLimitReached = 441,
		KeyInvalidIP = 442,
		MissingRequiredParameter = 460,
		SourceRequired = 461,
		ParameterError = 462,
		GenericServerError = 500,
		RateLimitReached = 607
	};

	OscarResponse(const QByteArray &json);
	~OscarResponse();

	ResultCode result() const;
	AbstractConnection::ConnectionError error() const;
	QString resultString() const;
	int detailCode() const;
	QVariantMap rawResult() const;
	QVariantMap data() const;
private:
	QVariantMap m_data;
	ResultCode m_result;
	QString m_resultString;
	QVariantMap m_rawResult;
	int m_detailCode;
};

OscarResponse::OscarResponse(const QByteArray &json)
{
	DEBUG() << json;
	QVariant dataVar;
	int len = json.length();
	K8JSON::parseRecord(dataVar, reinterpret_cast<const uchar*>(json.data()), &len);
	QVariantMap data = dataVar.toMap();
	QVariantMap response = data.value(QLatin1String("response")).toMap();
	m_rawResult = data;
	m_result = static_cast<ResultCode>(response.value(QLatin1String("statusCode")).toInt());
	m_detailCode = response.value(QLatin1String("statusDetailCode")).toInt();
	m_resultString = response.value(QLatin1String("statusText")).toString();
	m_data = response.value(QLatin1String("data")).toMap();
}

OscarResponse::~OscarResponse()
{
}

OscarResponse::ResultCode OscarResponse::result() const
{
	return m_result;
}

int OscarResponse::detailCode() const
{
	return m_detailCode;
}

AbstractConnection::ConnectionError OscarResponse::error() const
{
	switch (m_result) {
	case Success:
		return AbstractConnection::NoError;
	case MoreAuthenticationRequired:
	case InvalidRequest:
		return AbstractConnection::InternalError;
	case AuthorizationRequired:
		return AbstractConnection::MismatchNickOrPassword;
	case MethodNoAllowed:
	case RequestTimeout:
		return AbstractConnection::InternalError;
	case SourceRateLimitReached:
	case RateLimitReached:
		return AbstractConnection::RateLimitExceeded;
	case InvalidKey:
	case KeyUsageLimitReached:
	case KeyInvalidIP:
	case MissingRequiredParameter:
	case SourceRequired:
	case ParameterError:
	case GenericServerError:
		return AbstractConnection::InternalError;
	default:
		return AbstractConnection::SocketError;
	}
}

QString OscarResponse::resultString() const
{
	return m_resultString;
}

QVariantMap OscarResponse::rawResult() const
{
	return m_rawResult;
}

QVariantMap OscarResponse::data() const
{
	return m_data;
}

OscarAuth::OscarAuth(Client *client, const OAuthLoginData &loginData) :
	QObject(client),
	m_client(client),
	m_state(Invalid),
	m_loginData(loginData),
	m_tokenData(loginData.lastToken().toMap())
{
#if defined(OSCAR_USE_QCA2)
	static bool qcaInited = false;
	if (!qcaInited) {
		qcaInited = true;
		QCA::init();
		QCA::setAppName("ireen");
	}
#endif
	m_manager.setProxy(m_client->proxy());
	connect(m_client, SIGNAL(proxyUpdated(QNetworkProxy)),
			SLOT(setProxy(QNetworkProxy)));
}

OscarAuth::~OscarAuth()
{
	m_cleanupHandler.clear();
}

QAbstractSocket::SocketState OscarAuth::socketState()
{
	if (m_state == OscarAuth::Invalid || m_state == OscarAuth::AtError)
		return QAbstractSocket::UnconnectedState;
	return QAbstractSocket::ConnectingState;
}

void OscarAuth::setProxy(const QNetworkProxy &proxy)
{
	m_manager.setProxy(proxy);
}

void OscarAuth::login()
{
#if defined(OSCAR_USE_QCA2)
	if (!QCA::isSupported("hmac(sha256)")) {
		critical(DebugVeryVerbose) << "HMAC-SHA1 feature for QCA is not found. Try to install qca2-plugin-ossl";
		emit error(AbstractConnection::InternalClientError);
		deleteLater();
		return;
	}
#endif
	if (!m_tokenData.isEmpty()) {
		QByteArray a = m_tokenData.value(QLatin1String("a")).toByteArray();
		QDateTime expiresAt = m_tokenData.value(QLatin1String("expiresAt")).toDateTime();
		if (expiresAt > QDateTime::currentDateTime()) {
			startSession(a, m_tokenData.value(QLatin1String("sessionSecret")).toByteArray());
			return;
		}
	}
	clientLogin(true);
}

static QByteArray sha256hmac(const QByteArray &array, const QByteArray &sessionSecret)
{
#if defined(OSCAR_USE_3RDPARTY_HMAC)
	QByteArray mac(SHA256_DIGEST_SIZE, 0);
	hmac_sha256(reinterpret_cast<unsigned char*>(const_cast<char*>(sessionSecret.data())),
				sessionSecret.length(),
				reinterpret_cast<unsigned char*>(const_cast<char*>(array.data())),
				array.length(),
				reinterpret_cast<unsigned char*>(mac.data()),
				mac.size());
	return mac.toBase64();
#elif defined(OSCAR_USE_QCA2)
	QCA::MessageAuthenticationCode hash(QLatin1String("hmac(sha256)"), sessionSecret);
	hash.update(array);
	return hash.final().toByteArray().toBase64();
#else
# error Oscar authorization module depends on hmac-sha256 support
#endif
}

static QByteArray paranoicEscape(const QByteArray &raw)
{
	static char hex[17] = "0123456789ABCDEF";
	QByteArray escaped;
	escaped.reserve(raw.size() * 3);
	for (int i = 0; i < raw.size(); ++i) {
		const quint8 c = static_cast<quint8>(raw[i]);
		escaped += '%';
		escaped += hex[c >> 4];
		escaped += hex[c & 0x0f];
	}
	return escaped;
}

void OscarAuth::clientLogin(bool longTerm)
{
	QUrl url = QUrl::fromEncoded(ICQ_LOGIN_URL);
	url.addQueryItem(QLatin1String("devId"), m_loginData.developerId());
	url.addQueryItem(QLatin1String("f"), QLatin1String("json"));
	url.addQueryItem(QLatin1String("s"), m_client->uin());
	url.addQueryItem(QLatin1String("language"), generateLanguage());
	url.addQueryItem(QLatin1String("tokenType"), QLatin1String(longTerm ? "longterm" : "shortterm"));
	// FIXME: Sometimes passwords are cp-1251 (or any other windows-scpecific local encoding) encoded
	url.addEncodedQueryItem("pwd", paranoicEscape(m_loginData.password().toUtf8()));
	url.addQueryItem(QLatin1String("idType"), QLatin1String("ICQ"));
	url.addQueryItem(QLatin1String("clientName"), m_loginData.password());
	url.addQueryItem(QLatin1String("clientVersion"), QString::number(version()));
	QByteArray query = url.encodedQuery();
	url.setEncodedQuery(QByteArray());
	DEBUG() << Q_FUNC_INFO << url << query;
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	QNetworkReply *reply = m_manager.post(request, query);
	m_cleanupHandler.add(reply);
	connect(reply, SIGNAL(finished()), this, SLOT(onClientLoginFinished()));
}

void OscarAuth::onClientLoginFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	Q_ASSERT(reply);
	reply->deleteLater();
	if (reply->error() != QNetworkReply::NoError) {
		m_errorString = reply->errorString();
		emit error(AbstractConnection::SocketError);
		deleteLater();
		return;
	}
	OscarResponse response(reply->readAll());
	DEBUG() << Q_FUNC_INFO << response.rawResult();
	if (response.result() != OscarResponse::Success) {
		m_errorString = response.resultString();
		emit error(response.error());
		deleteLater();
		return;
	}

	QVariantMap data = response.data().value(QLatin1String("token")).toMap();
	QByteArray token = data.value(QLatin1String("a")).toByteArray();
	int expiresIn = data.value(QLatin1String("expiresIn")).toInt();
	QDateTime expiresAt = QDateTime::currentDateTime().addSecs(expiresIn);
	QByteArray sessionSecret = data.value(QLatin1String("sessionSecret")).toByteArray();
	int hostTime = data.value(QLatin1String("hostTime")).toInt();
	int localTime = QDateTime::currentDateTime().toUTC().toTime_t();
	sessionSecret = sha256hmac(sessionSecret, m_loginData.password().toUtf8());
	{
		m_tokenData.insert(QLatin1String("a"), token);
		m_tokenData.insert(QLatin1String("expiresAt"), expiresAt.toString(Qt::ISODate));
		m_tokenData.insert(QLatin1String("sessionSecret"), sessionSecret);
		m_tokenData.insert(QLatin1String("hostTimeDelta"), hostTime - localTime);
		emit m_client->loginTokenUpdated(m_tokenData);
	}
	startSession(token, sessionSecret);
}

void OscarAuth::startSession(const QByteArray &token, const QByteArray &sessionKey)
{
	QUrl url = QUrl::fromEncoded(ICQ_START_SESSION_URL);
	url.addEncodedQueryItem("a", token.toPercentEncoding());
	url.addQueryItem(QLatin1String("distId"), m_loginData.distributionId());
	url.addQueryItem(QLatin1String("f"), QLatin1String("json"));
	url.addQueryItem(QLatin1String("k"), m_loginData.developerId());
	int localTime = QDateTime::currentDateTime().toUTC().toTime_t();
	{
		localTime += m_tokenData.value(QLatin1String("hostTimeDelta")).toInt();
	}
	url.addQueryItem(QLatin1String("ts"), QString::number(localTime));
//	Some strange error at ICQ servers. I receive "Parameter error" if it is set
//	url.addQueryItem(QLatin1String("language"), generateLanguage());
	url.addQueryItem(QLatin1String("majorVersion"), QString::number(m_loginData.versionMajor()));
	url.addQueryItem(QLatin1String("minorVersion"), QString::number(m_loginData.versionMinor()));
	url.addQueryItem(QLatin1String("pointVersion"), QString::number(m_loginData.versionSecMinor()));
	url.addQueryItem(QLatin1String("clientName"), m_loginData.clientName());
	url.addQueryItem(QLatin1String("clientVersion"), QString::number(version()));
	url.addEncodedQueryItem("useTLS", m_loginData.isSslEnabled() ? "1" : "0");
	url.addEncodedQueryItem("sig_sha256", generateSignature("GET", sessionKey, url));
	DEBUG() << url.toEncoded();
	QNetworkRequest request(url);
	//	Some strange error at ICQ servers. I receive "Parameter error" if it is set no anything else
	request.setRawHeader("Accept-Language", generateLanguage().toLatin1());
	QNetworkReply *reply = m_manager.get(request);
	m_cleanupHandler.add(reply);
	connect(reply, SIGNAL(finished()), SLOT(onStartSessionFinished()));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(onSslErrors(QList<QSslError>)));
}

void OscarAuth::onStartSessionFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	Q_ASSERT(reply);
	reply->deleteLater();
	deleteLater();
	if (reply->error() != QNetworkReply::NoError) {
		m_errorString = reply->errorString();
		emit error(AbstractConnection::SocketError);
		return;
	}
	OscarResponse response(reply->readAll());
	DEBUG() << Q_FUNC_INFO << response.rawResult();
	QVariantMap data = response.data();
	if (response.result() == OscarResponse::InvalidRequest
			&& response.detailCode() == 1015) {
		// Invalid local time
		int hostTime = data.value(QLatin1String("ts")).toInt();
		int localTime = QDateTime::currentDateTime().toUTC().toTime_t();
		m_tokenData.insert(QLatin1String("hostTimeDelta"), hostTime - localTime);
		emit m_client->loginTokenUpdated(m_tokenData);
		login();
		return;
	}
	if (response.result() != OscarResponse::Success) {
		m_errorString = response.resultString();
//		m_account->config(QLatin1String("general")).remove(QLatin1String("token"));
		emit error(response.error());
		return;
	}
	QString host = data.value(QLatin1String("host")).toString();
	int port = data.value(QLatin1String("port"), 443).toInt();
	QByteArray cookie = QByteArray::fromBase64(data.value(QLatin1String("cookie")).toByteArray());
	m_client->d_func()->connectToBOSS(host, port, cookie);
}

void OscarAuth::onSslErrors(const QList<QSslError> &errors)
{
	QString str;
	foreach (const QSslError &error, errors) {
		str += "(" + QString::number(error.error()) + ") ";
		str += error.errorString();
		str += '\n';
	}
	str.chop(1);
	DEBUG() << Q_FUNC_INFO << str;
}

QString OscarAuth::generateLanguage()
{
	QLocale locale;
	if (locale.language() != QLocale::C)
		return locale.name().toLower().replace('_', '-');
	else
		return QLatin1String("en-us");
}

int OscarAuth::version() const
{
	return (m_loginData.versionMajor() << 24) | (m_loginData.versionMinor() << 16) |
			m_loginData.versionSecMinor() << 8 | m_loginData.versionPatch();
}

QByteArray OscarAuth::generateSignature(const QByteArray &method, const QByteArray &sessionSecret, const QUrl &url)
{
	QList<QPair<QString, QString> > items = url.queryItems();
	qSort(items);
	QByteArray array = method;
	array += '&';
	QString str;
	str = url.toString(QUrl::RemoveUserInfo | QUrl::RemovePort
					   | QUrl::RemoveQuery | QUrl::RemoveFragment);
	array += QUrl::toPercentEncoding(str);
	array += '&';
	str.clear();
	for (int i = 0; i < items.size(); ++i) {
		str += items[i].first;
		str += QLatin1String("=");
		str += QLatin1String(QUrl::toPercentEncoding(items[i].second));
		str += QLatin1String("&");
	}
	str.chop(1);
	array += QUrl::toPercentEncoding(str, QByteArray(), "&=");

	return sha256hmac(array, sessionSecret);
}

} // ireen

