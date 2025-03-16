#include "QXMLRpc.h"
#include <QDomDocument>
#include <QtGlobal>
#include <atomic>
#define DEBUG_PREFIX "QXMLRpcClient[" << clientId << ']'
#define xmlrpcDebug qDebug() << DEBUG_PREFIX
#define xmlrpcInfo  qInfo()  << DEBUG_PREFIX

bool QXMLRpcClient::debugMode = true;

QXMLRpcClient::QXMLRpcClient(QObject* parent) : QNetworkAccessManager(parent)
{
  static std::atomic_ushort idCounter(0);

  clientId = ++idCounter;
  if (debugMode)
    xmlrpcDebug << "Constructed";
}

QXMLRpcClient::~QXMLRpcClient()
{
  if (debugMode)
    xmlrpcDebug << "Destroyed"; 
}

void QXMLRpcClient::setEndpoint(const QUrl& value)
{
  endpoint = value;
  emit endpointChanged();
}

void QXMLRpcClient::call(const QString& methodName, const QVariantList& parameters, std::function<void (QVariant)> callback)
{
  QString body = getXmlForMethodCall(methodName, parameters);
  QNetworkReply* reply;
  QNetworkRequest* request = new QNetworkRequest(endpoint);

  request->setRawHeader("Connection", "close");
  request->setRawHeader("Content-Type", "text/xml");
  request->setRawHeader("User-Agent", "qt-xmlrpc/0.1");
  reply = QNetworkAccessManager::post(*request, body.toUtf8());
  xmlrpcInfo << "Calling" << endpoint << "::" << methodName;
  if (debugMode)
    xmlrpcDebug << "Sending XMLRpc query" << qPrintable(body);
  connect(reply, &QNetworkReply::finished, [=]()
  {
    QVariant returnValue = getReturnValueFromReply(*reply);

    if (QXMLRpcFault::isFault(returnValue))
      emit faultReceived(QXMLRpcFault(returnValue));
    else
      emit responseReceived(returnValue);
    if (callback)
      callback(returnValue);
    reply->deleteLater();
    delete request;
  });
}

QString QXMLRpcClient::getXmlForMethodCall(const QString& methodName, const QVariantList& parameters)
{
  QString result;

  result += "<?xml version=\"1.0\"?>\n";
  result += "<methodCall>\n";
  result += "  <methodName>" + methodName + "</methodName>\n";
  result += "  <params>\n";
  for (const QVariant& value : parameters)
  {
    result += "    <param>\n";
    result += "      <value>\n";
    result += "        " + fromVariantToXmlValue(value) + '\n';
    result += "      </value>\n";
    result += "    </param>\n";
  }
  result += "  </params>\n";
  result += "</methodCall>\n";
  return result;
}

static QString escapeString(QString source)
{
  return source.replace("&", "&amp;").replace("<", "&lt;");
}

QString QXMLRpcClient::fromVariantToXmlValue(const QVariant& variant)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  switch (variant.typeId())
#else
  switch (static_cast<QMetaType::Type>(variant.type()))
#endif
  {
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
      return fromVariantListToXmlValue(variant.toList());
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash:
      return fromVariantHashToXmlValue(variant.toHash());
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
      return "<int>" + variant.toString() + "</int>";
    case QMetaType::Double:
    case QMetaType::Float:
      return "<double>" + variant.toString() + "</double>";
    case QMetaType::Bool:
      return "<boolean>" + (variant.toBool() ? QString("1") : QString("0")) + "</boolean>";
    case QMetaType::QDateTime:
      return "<dateTime.iso8601>" + variant.toDateTime().toString("yyyyMMddThh:mm:ss") + "</dateTime.iso8601>";
    case QMetaType::QByteArray:
      return "<base64>" + variant.toByteArray().toBase64() + "</base64>";
    case QMetaType::Nullptr:
      return "<nil></nil>"; // Non-standard extension
    case QMetaType::QString:
    default:
      break ;
  }
  return "<string>" + escapeString(variant.toString()) + "</string>";
}

QString QXMLRpcClient::fromVariantListToXmlValue(const QVariantList& list)
{
  QString result;

  result += "<array><data>\n";
  for (const QVariant& value : list)
  {
    result += "<value>";
    result += fromVariantToXmlValue(value);
    result += "</value>\n";
  }
  result += "</data></array>";
  return result;
}

QString QXMLRpcClient::fromVariantHashToXmlValue(const QVariantHash& hash)
{
  QString result;

  result += "<struct>\n";
  for (auto it = hash.begin() ; it != hash.end() ; ++it)
  {
    result += "<member>";
    result += "<name>" + it.key() + "</name>";
    result += "<value>";
    result += fromVariantToXmlValue(it.value());
    result += "</value>";
    result += "</member>\n";
  }
  result += "</struct>";
  return result;
}

static QDomElement getElementByPath(QDomElement element, QStringList path)
{
  QDomElement child = element.firstChildElement(path.takeFirst());

  if (path.size() > 0)
    return getElementByPath(child, path);
  return child;
}

static QVariant xmlValueToVariant(QDomElement);

static QPair<QString,QDomElement> loadStructMember(QDomElement member)
{
  QPair<QString, QDomElement> result;

  for (QDomElement n = member.firstChildElement() ; !n.isNull() ; n = n.nextSiblingElement())
  {
    if (n.tagName() == "name")
      result.first = n.text();
    else if (n.tagName() == "value")
      result.second = n;
  }
  return result;
}

static QVariant xmlStructToHash(QDomElement element)
{
  QVariantMap map;

  for (QDomElement n = element.firstChildElement() ; !n.isNull() ; n = n.nextSiblingElement())
  {
    if (n.tagName() == "member")
    {
      auto member = loadStructMember(n);
      map.insert(member.first, xmlValueToVariant(member.second));
    }
  }
  return map;
}

static QVariant xmlArrayToList(QDomElement element)
{
  QVariantList list;
  QDomElement data = element.firstChildElement("data");

  for (QDomElement n = data.firstChildElement() ; !n.isNull() ; n = n.nextSiblingElement())
  {
    if (n.tagName() == "value")
      list.push_back(xmlValueToVariant(n));
  }
  return list;
}

static QVariant xmlValueToVariant(QDomElement value)
{
  QDomElement data = value.firstChildElement();

  if (data.tagName() == "struct")
    return xmlStructToHash(data);
  else if (data.tagName() == "array")
    return xmlArrayToList(data);
  else if (data.tagName() == "i4" || data.tagName() == "int")
    return QVariant(data.text().toInt());
  else if (data.tagName() == "double")
    return QVariant(data.text().toDouble());
  else if (data.tagName() == "dateTime.iso8601")
    return QVariant(QDateTime::fromString(data.text(), Qt::ISODate));
  else if (data.tagName() == "boolean")
    return QVariant(data.text() == "1");
  return data.text();
}

static QVariant raiseXmlRpcFault(unsigned short clientId, QByteArray& body, QDomElement element)
{
  QVariant     value = xmlValueToVariant(element.firstChildElement());
  QXMLRpcFault fault(value);

  xmlrpcInfo << "XMLRpc fault: code = " << fault.code();
  xmlrpcInfo << "XMLRpc fault: string = " << fault.message();
  return value;
}

QVariant QXMLRpcClient::getReturnValueFromReply(QNetworkReply& reply)
{
  int status = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  QDomDocument document;
  QDomElement value, fault;
  QByteArray replyBody = reply.readAll();

  xmlrpcInfo << "Response status=" << status;
  if (debugMode)
    xmlrpcDebug << "Received XMLRpc response" << qPrintable(replyBody);
  document.setContent(replyBody);
  fault = document.documentElement().firstChildElement("fault");
  if (!fault.isNull())
    return raiseXmlRpcFault(clientId, replyBody, fault);
  value = getElementByPath(document.documentElement(), {"params", "param", "value"});
  return xmlValueToVariant(value);
}
