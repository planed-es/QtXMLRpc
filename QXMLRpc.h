#ifndef  QXMLRPC_H
# define QXMLRPC_H

# include <QtNetwork/QNetworkAccessManager>
# include <QtNetwork/QNetworkReply>
# include <QString>
# include <QUrl>

class QXMLRpcClient : public QNetworkAccessManager
{
  Q_OBJECT
public:
  QXMLRpcClient(QObject* parent = nullptr);

  void setEndpoint(const QUrl& value) { endpoint = value; emit endpointChanged(); }
  void call(const QString& methodName, const QVariantList& parameters, std::function<void (QVariant)> callback);

signals:
  void endpointChanged();

private:
  QUrl endpoint;
  static QString getXmlForMethodCall(const QString& methodName, const QVariantList& parameters);
  static QString fromVariantToXmlValue(const QVariant& variant);
  static QString fromVariantListToXmlValue(const QVariantList& list);
  static QString fromVariantHashToXmlValue(const QVariantHash& hash);
  static QVariant getReturnValueFromReply(QNetworkReply& reply);
};

#endif
