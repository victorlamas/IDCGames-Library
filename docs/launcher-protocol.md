# Protocolo del Launcher IDC Games

Windows x64. La DLL implementa IPC local; no es una frontera de seguridad frente a otro proceso local.

## ABI

| Export | Parámetros |
|---|---|
| `_InitIDCLib` | wchar_t* gameId, wchar_t* clientKey, uint32 processId, notify, log, bool useQueue |
| `_PullMessage` | ninguno |
| `_OpenShop` | wchar_t* transactionId, wchar_t* extra, int userLevel, bool sandbox, wchar_t* article |
| `_ClosePurchase` | wchar_t* transactionId, wchar_t* idcpaymentId |
| `_EndIDCLib` | ninguno |

Retorno `int`; convención StdCall. `bool` nativo de un byte (C#: `UnmanagedType.I1`). Notificación: `void(uint32 command, void* data, int byteLength)`; log: `void(void* data, int byteLength)`. En notificaciones JSON la codificación es **UTF-16LE y la longitud está en bytes**, no caracteres. Copiar datos dentro del callback; no retener el puntero. Mantener vivos los delegates y finalizar la DLL antes de descargarlos.

El wrapper usa cola, limita cada payload a 1 MiB, procesa eventos desde `Poll`, no reenvía logs nativos y no deja escapar excepciones de decodificación a código C++. Se permite compatibilidad con datos posteriores al primer objeto JSON. Los campos locales no son pruebas criptográficas.

| Comando | Hex |
|---|---|
| Error | FBB65363 |
| UserData | 03275C4A |
| BadUserInitData | ABA83A28 |
| PurchaseNotification | 733F2850 |
| NewPurchaseCode | FA4967C3 |
| PurchaseOk | 8218D08C |
| PurchaseError | BB485D88 |
| BadPurchaseCode | 6A6AC964 |
| PurchaseClosed | 6C1CCF80 |
| VideoAdResult | B1DE0A7D |

Ejemplo de UserData (datos ficticios):

```json
{"status":0,"description":"OK","extraParams":{"userId":123,"nick":"Demo","tokenUserGameId":"REDACTED"}}
```

El Launcher puede añadir email, language, country y currency. El wrapper expone solo lo necesario para el ejemplo. `Start`/init con retorno 0 no significa login completado. Ver [autenticación](authentication-idc.md) para validar la credencial y [compras/vídeo](shop-and-ads.md) para los demás eventos.
