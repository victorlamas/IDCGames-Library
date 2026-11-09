# Tienda y anuncios de IDC Games

Requiere login local recibido y configuración del juego en IDC Games. La biblioteca abre las experiencias del Launcher; el backend del juego sigue siendo responsable de la entrega y de la autorización.

## Tienda

```csharp
idc.OpenShop(Guid.NewGuid().ToString(), article: "", sandbox: true);
```

`article` vacío abre la tienda; para abrir un artículo usar el identificador configurado con IDC. `extra` y `userLevel` son metadatos del cliente, no autorización. Los parámetros nativos tienen límites; el wrapper rechaza cadenas demasiado largas, comillas, barras invertidas y controles porque la DLL histórica construye JSON en buffers fijos.

Notificación de compra:

```json
{"dllTransaction":"game-order-123","idcpaymentTransaction":"idc-payment-456"}
```

1. Crear orden del juego y asociarla a usuario/juego/precio/producto en backend.
2. Abrir checkout con un ID de correlación único. Sandbox prueba el flujo y no demuestra un pago de producción.
3. Recibir notificación local y enviar IDs al backend.
4. El backend verifica el pago con el mecanismo comercial acordado con IDC, comprueba usuario, juego, producto, importe/moneda y estado; entrega de forma duradera e idempotente con unicidad del ID de pago.
5. Después de confirmar la entrega, llamar `idc.AcknowledgePurchase(transactionId, paymentId)`.

`_ClosePurchase` confirma recepción; no verifica un pago. Nunca entregar ni confirmar automáticamente al recibir el callback. Este repositorio no incluye una API de liquidación/consulta nueva: el endpoint y las credenciales del servicio comercial se deben acordar con IDC. Reintentos, cierres y notificaciones duplicadas no deben duplicar entregas.

## Vídeo Ads

No existe un export `_OpenVideoAd` en la DLL incluida. El wrapper escribe JSON UTF-8 al named pipe local `IDCGamesPipe` y recibe el resultado por el callback nativo:

```csharp
string requestId = await idc.OpenVideoAdAsync(sandbox: true);
```

```json
{"videoAd":true,"gameId":"123","sandbox":true,"requestId":"example-uuid","layout":{"mode":"fullscreen_monitor"}}
```

Modos del wrapper: `fullscreen_monitor`, `fullscreen_game`, `windowed`. El Launcher también tiene opciones avanzadas de rectángulo; no se exponen en este ejemplo. Timeout de conexión al pipe: 3 segundos. Solo pedir un anuncio a la vez; el Launcher puede devolver `ALREADY_ACTIVE`.

Callback JSON (`VideoAdResult`):

```json
{"status":"OK","gameId":"123","sessionId":"example-session","errorCode":""}
```

Compatibilidad: `VIDEO_AD|STATUS|gameId|sessionId|errorCode` en UTF-8. Estados: `STARTED` (no terminal), `OK` (completado), `SKIPPED`, `NO_FILL`, `ERROR`. El evento web de completado se traduce a `OK`. No asumir disponibilidad de inventario ni repetir sin límite ante `NO_FILL`.

**El resultado actual no devuelve `requestId`.** No usarlo para afirmar correlación garantizada. Comprobar el juego esperado, mantener una sola solicitud activa y tratar resultados tardíos/duplicados de forma conservadora.

El Launcher gestiona la página `https://auth.idcgames.com/pages/video/{gameId}/{gameSlug}`, sesión y bridge con el proveedor. No abrir esa URL arbitrariamente como sustituto del flujo nativo. Preview/sandbox no acredita monetización. Un `OK` local tampoco es un recibo de recompensa: cualquier premio con valor debe autorizarse en backend mediante el mecanismo verificado acordado con IDC y entregarse una sola vez por sesión. El ejemplo no concede premios ni simula éxito cuando falta anuncio.

## Prueba real antes de publicar un juego

Probar login correcto/rechazado, apertura/cancelación de tienda, compra sandbox, repetición del mismo pago, cierre/reinicio durante entrega, anuncio completado/saltado/sin inventario y ausencia del Launcher. Las pruebas automáticas incluidas son offline y no sustituyen esta aceptación con las credenciales y configuración del juego.
