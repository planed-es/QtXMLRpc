#ifndef  QXMLRPC_H
# define QXMLRPC_H

# include <QtNetwork/QNetworkAccessManager>
# include <QtNetwork/QNetworkReply>
# include <QString>
# include <QUrl>

class QXMLRpcFault
{
public:
  QXMLRpcFault(QVariant value) : value(value.toMap()) {}

  int code() const { return value["faultCode"].toInt(); }
  QString message() const { return value["faultString"].toString(); }

  static bool isFault(QVariant value);

private:
  QVariantMap value;
};

class QXMLRpcClient : public QNetworkAccessManager
{
  Q_OBJECT
public:
  QXMLRpcClient(QObject* parent = nullptr);

  void setEndpoint(const QUrl& value) { endpoint = value; emit endpointChanged(); }
  void call(const QString& methodName, const QVariantList& parameters) { call(methodName, parameters, std::function<void(QVariant)>()); }
  void call(const QString& methodName, const QVariantList& parameters, std::function<void (QVariant)> callback);

signals:
  void endpointChanged();
  void responseReceived(QVariant);
  void faultReceived(QXMLRpcFault);

private:
  QUrl endpoint;
  static QString getXmlForMethodCall(const QString& methodName, const QVariantList& parameters);
  static QString fromVariantToXmlValue(const QVariant& variant);
  static QString fromVariantListToXmlValue(const QVariantList& list);
  static QString fromVariantHashToXmlValue(const QVariantHash& hash);
  static QVariant getReturnValueFromReply(QNetworkReply& reply);
};

#endif
