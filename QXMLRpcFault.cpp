#include "QXMLRpc.h"
#include <QtGlobal>

int QXMLRpcFault::code() const
{
  return value["faultCode"].toInt();
}

QString QXMLRpcFault::message() const
{
  return value["faultString"].toString();
}

bool QXMLRpcFault::isFault(QVariant value)
{
  int typeId;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  typeId = value.typeId();
#else
  typeId = static_cast<int>(value.type());
#endif
  return typeId == QMetaType::QVariantMap && value.toMap().contains("faultCode");
}
