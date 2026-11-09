# Autenticación de jugadores IDC Games

Integración independiente de GamePanel. El cliente obtiene datos del Launcher; el backend del estudio debe validar la credencial y crear su propia sesión. No confiar en `userId`, nick, email ni en un callback local sin validación del servidor.

## Credenciales que solicitar

| Dato | Emisor / destino |
|---|---|
| IDC `id_game` | IDC Games; identifica el juego, no es secreto |
| Clave cliente Launcher, llamada `gameSecret` en IPC histórico | IDC Games; compatibilidad del cliente nativo, recuperable de un juego distribuido |
| Credencial del jugador | Launcher o login IDC; transmitir solo por HTTPS al backend propio, sin logs |
| Canal de validación para el backend del estudio | Acordar con IDC Games; no facilitar al estudio la clave interna global de Auth |

La clave cliente no autoriza operaciones administrativas ni demuestra la identidad de un jugador. No confundirla con claves de servicios internos.

## Flujo con Launcher

1. El Launcher IDC inicia sesión y lanza el juego.
2. El juego inicializa IDCLib2 y procesa `UserData` con `userId` y `tokenUserGameId`.
3. El cliente envía esos datos a su backend por HTTPS.
4. El backend valida mediante el contrato habilitado por IDC, comprueba el acceso al juego cuando corresponda y emite una sesión propia de corta duración.

`tokenUserGameId` es un nombre histórico: distintos caminos/versiones del Launcher pueden entregar un token corto legado, un token de cuenta legado o un token Auth. No asumir que siempre es JWT. Coordinar con IDC el formato habilitado antes de elegir el verificador.

## Comprobación histórica de token de juego

Contrato actual de compatibilidad, comprobado en el código de Auth:

```bash
curl --request POST 'https://auth.idcgames.com/api/launcher/check-user-game-token' \
  --header 'Content-Type: application/json' \
  --data '{"userId":123,"gameId":456,"tokenUserGameId":"REDACTED","include_user_profile":1}'
```

Respuesta de éxito: `{"status":"OK"}` (puede incluir perfil). Rechazo: `{"status":"KO",...}`. También puede responder HTTP 200 en un rechazo; comprobar siempre el JSON y fallar de forma cerrada. Los IDs son IDC, no IDs internos del juego.

El token corto usa un mecanismo legado basado en MD5 y tiempo; **no debe presentarse como prueba moderna suficiente de identidad para un backend expuesto**. Solicitar a IDC una vía de validación de identidad apropiada antes de abrir servicios sensibles a estudios externos.

## Validación de token Auth: infraestructura controlada por IDC

El contrato interno existente es:

```bash
curl --request POST 'https://auth.idcgames.com/api/web/verify-token' \
  --header 'X-Internal-Key: REPLACE_WITH_INTERNAL_AUTH_KEY' \
  --header 'Content-Type: application/json' \
  --data '{"token":"REDACTED_AUTH_TOKEN","useridc":"123"}'
```

Éxito HTTP 200 con `valid:true`, `useridc`, `nick`, `email`, `id_user`, `source`; un token inválido puede dar HTTP 200 con `valid:false`. Clave interna ausente/incorrecta: HTTP 401. Requerir `valid:true` y coincidencia con el jugador esperado. No fiarse de un JWT solo porque se puede decodificar.

**`X-Internal-Key` es una clave global interna: no incluirla en el cliente, repositorios ni entregarla al estudio externo.** Este ejemplo documenta el contrato de IDC; no implica que el estudio tenga acceso autorizado. IDC debe acordar un servicio de validación controlado o el mecanismo específico de acceso. No hay en esta entrega un endpoint nuevo ni una clave por estudio ya desplegados.

Esta comprobación valida identidad de cuenta, **no comprueba licencia/pertenencia al juego ni devuelve `game_id`**. Esa autorización debe resolverse separadamente.

`/api/launcher/login-game-token` valida una aplicación mediante `gameId`/`gameSecret`; sin token de cuenta puede emitir un token de aplicación. No es un login de jugador por usuario/contraseña ni puede usarse ese token de aplicación como identidad de un jugador.

## Backend propio

Definir un endpoint propio para recibir la credencial, limitar intentos, validar con el proveedor y emitir sesión. Ese endpoint pertenece al estudio; esta guía no inventa una ruta pública de IDC. Mantener los tokens fuera de URLs, métricas, trazas y archivos. No vincular cuentas automáticamente por email. Una identidad autenticada no implica autorización para todas las compras, juegos o recursos.
