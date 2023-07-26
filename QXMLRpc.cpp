#include "QXMLRpc.h"
#include <QDomDocument>
#include <iostream>

QXMLRpcClient::QXMLRpcClient(QObject* parent) : QNetworkAccessManager(parent)
{
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
  connect(reply, &QNetworkReply::finished, [=]()
  {
    callback(getReturnValueFromReply(*reply));
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
  /*
  qDebug() << "XML for method call:";
  std::cout << result.toStdString() << std::endl;
  */
  return result;
}

QString QXMLRpcClient::fromVariantToXmlValue(const QVariant& variant)
{
  switch (variant.typeId())
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
      //return "<dateTime.iso8601>" + variant.toDateTime().toString(Qt::ISODate) + "</dateTime.iso8601>";
    default:
      break ;
  }
  return "<string>" + variant.toString() + "</string>";
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

static QVariant raiseXmlRpcFault(QByteArray& body, QDomElement element)
{
  QVariant     value = xmlValueToVariant(element.firstChildElement());
  QXMLRpcFault fault(value);

  qDebug() << "XMLRpc fault: code = " << fault.code();
  qDebug() << "XMLRpc fault: string = " << fault.message();
  return value;
}

QVariant QXMLRpcClient::getReturnValueFromReply(QNetworkReply& reply)
{
  int status = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  QDomDocument document;
  QDomElement value, fault;
  QByteArray replyBody = reply.readAll();

  /*
  qDebug() << "QXMLRpcClient:";
  std::cout << replyBody.toStdString() << std::endl;
  */
  document.setContent(replyBody);
  fault = document.documentElement().firstChildElement("fault");
  if (!fault.isNull())
    return raiseXmlRpcFault(replyBody, fault);
  value = getElementByPath(document.documentElement(), {"params", "param", "value"});
  return xmlValueToVariant(value);
}
