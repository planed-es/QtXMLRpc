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

  int code() const;
  QString message() const;

  static bool isFault(QVariant value);

private:
  QVariantMap value;
};

class QXMLRpcClient : public QNetworkAccessManager
{
  Q_OBJECT
public:
  explicit QXMLRpcClient(QObject* parent = nullptr);
  virtual ~QXMLRpcClient();

  static bool debugMode;

  void setEndpoint(const QUrl& value);
  void call(const QString& methodName, const QVariantList& parameters) { call(methodName, parameters, std::function<void(QVariant)>()); }
  void call(const QString& methodName, const QVariantList& parameters, std::function<void (QVariant)> callback);

signals:
  void endpointChanged();
  void responseReceived(QVariant);
  void faultReceived(QXMLRpcFault);

private:
  QUrl endpoint;
  unsigned short clientId;
  static QString getXmlForMethodCall(const QString& methodName, const QVariantList& parameters);
  static QString fromVariantToXmlValue(const QVariant& variant);
  static QString fromVariantListToXmlValue(const QVariantList& list);
  static QString fromVariantHashToXmlValue(const QVariantHash& hash);
  QVariant getReturnValueFromReply(QNetworkReply& reply);
};

#endif
