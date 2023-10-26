# QtXMLRpc

This library is a simple XML-RPC client using QtNetwork. It uses [QVariant](https://doc.qt.io/qt-6/qvariant.html) to give you a simple way to read and write XML-RPC method parameters and return values.

## Setup

Make sure you have the Qt development package installed for either Qt6 or Qt5. This library requires the Core, Network and Xml components.

Build the library with the following commands:

```
mkdir build
cd build
cmake ..
make
```

Use the following command to install the library to your system:

```
sudo make install
```

### Add to your CMake project

QtXMLRpc uses pkgconfig: once installed on your system, you can easily add it to your projects using CMake's pkgconfig extension:

```cmake
# add the pkgconfig extension
find_package(PkgConfig)

# import QtXMLRpc
pkg_check_modules(QTXMLRPC REQUIRED Qt${QT_VERSION_MAJOR}XMLRpc>=1.0.0)

# register the library's include directory
include_directories(${QTXMLRPC_INCLUDE_DIRS})

target_link_libraries(target PRIVATE
  # ...
  ${QTXMLRPC_LIBRARIES})
```

## Usage

Usint QtXMLRpc is pretty simple and straightforward. The `QXMLRpcClient` object
provides the `setEndpoint` and `call` method, which respectively allow you to
setup the url and credentials for your XML-RPC server using
[QUrl](https://doc.qt.io/qt-6/qurl.html), and call one of your server's method.

```c++
#include <QSharedPointer>
#include <QXMLRpc.h>

void makeXmlrpcCall()
{
  QSharedPointer<QXMLRpcClient> xmlrpc(new QXMLRpcClient());
  QUrl endpoint("https://domain.com/xmlrpc");
  QVariantMap parameters;

  // Assuming your server uses http authentication,
  // you may set the username and password using QUrl:
  endpoint.setUserName("username");
  endpoint.setPassword("password");

  // Set the URL of the XML-RPC endpoint you're using with a QUrl paramter:
  xmlrpc->setEndpoint(endpoint);

  // If you need to pass parameters, set them up in a QVariantList object:
  parameters << 42 << QString("Hello World");

  // Check the type mapping table at the end of this document to see how
  // variables types will map to XML-RPC types.

  // Use the `call` method to call an XML-RPC method on your remote server.
  // Note how we capture the xmlrpc QSharedPointer within the callback to
  // ensure that it will remain in memory during the operation:
  xmlrpc->call("methodName", parameters, [xmlrpc](QVariant result)
  {
    // Once a response has been received, you should always check whether it
    // is a return value or a faulty response by using QXMLRpcFault::isFault:
    if (QXMLRpcFault::isFault(result))
      qDebug() << "Your request failed:" << QXMLRpcFault(result).message();

    // Otherwise, you can safely interact with the QVariant return value:
    else
      qDebug() << "Request response:" << result;
  });
}
```

You may also use Qt signals to handle responses and failures. In which case,
successfull responses are handled with the `responseReceived` signal, and
faulty responses are handled with the `faultReceived` signal:

```c++
#include <QSharedPointer>
#include <QXMLRpc.h>

QXMLRpcClient* makeXmlrpcCallMachine(QUrl endpoint, QObject* parent)
{
  QXMLRpcClient* xmlrpc = new QXMLRpcClient(parent);

  xmlrpc->setEndpoint(endpoint);
  QObject::connect(xmlrpc, &QXMLRpcClient::responseReceived, parent, [](QVariant response)
  {
    qDebug() << "Received a response from XML-RPC:" << response;
  });
  QObject::connect(xmlrpc, &QXMLRpcClient::faultReceived, parent, [](QXMLRpcFault fault)
  {
    qDebug() << "Received a faulty response from XML-RPC: " << fault.code() << fault.message();
  });
  return xmlrpc;
}
```

### Type mapping table

QtXMLRpc uses [QVariant::typeId](https://doc.qt.io/qt-6/qvariant.html#typeId)
to figure out how to map native types to XML-RPC types:

```
<array>             QVariantList
                    QStringList
<base64>            QByteArray
<boolean>           bool
<datetime.iso8601>  QDateTime
<double>            double, float
<integer>           int, unsigned int, short, unsigned short, long, unsigned long, long long, unsigned long long
<nil>               std::nullptr_t
<string>            QString
<struct>            QVariantMap, QVariantHash
```

Any unsupported type will be passed as `<string>`, using the `QVariant::toString` convertor.
