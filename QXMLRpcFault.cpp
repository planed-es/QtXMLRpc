#include "QXMLRpc.h"
#include <QtGlobal>

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
