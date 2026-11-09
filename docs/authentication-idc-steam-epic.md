# Autenticación independiente: IDC Games, Steam y Epic

Para el estudio/Nvia. Ningún flujo necesita GamePanel. La biblioteca C# de este repositorio implementa únicamente el Launcher de IDC; para Steam y Epic usar los SDK oficiales del proveedor.

## Credenciales y responsabilidades

| Proveedor | Cliente del juego | Backend / configuración privada |
|---|---|---|
| IDC Games | id_game, clave cliente Launcher y credencial del jugador recibida por IPC | Canal de validación acordado con IDC; no compartir su clave interna global |
| Steam | AppID, Steamworks SDK, ticket del jugador | Publisher Web API key autorizada para el AppID; nunca enviarla al juego |
| Epic | Configuración EOS de producto/sandbox/deployment, ClientID y política GameClient apropiada; token del jugador | Configuración esperada y, si se necesita, credenciales separadas de TrustedServer |

Los identificadores públicos no son secretos. En EOS un ClientSecret de cliente con una política limitada puede formar parte de la configuración del juego según el diseño de EOS; no confundirlo con credenciales privilegiadas de servidor. Usar políticas de mínimo privilegio y no distribuir un cliente TrustedServer. [Políticas oficiales de EOS](https://dev.epicgames.com/docs/epic-online-services/eos-fundamentals/client-and-client-policy/client-policy-guide).

## IDC Games

Seguir [la guía de IDC](authentication-idc.md): Launcher → callback UserData → HTTPS al backend propio → validación con IDC → sesión del juego. Incluye peticiones reales, respuesta `status`/`valid` y las restricciones del verificador legado y de la clave interna.

## Steam

1. Configurar Steamworks SDK con el AppID del juego y exigir sesión Steam válida.
2. Solicitar `ISteamUser::GetAuthTicketForWebApi(identity)` y esperar `GetTicketForWebApiResponse_t` satisfactorio. Mantener un `identity` acordado con vuestro backend.
3. Convertir los bytes del ticket a hexadecimal y enviarlos por HTTPS al backend propio. No imprimirlos. Gestionar/cancelar el ticket según su ciclo de vida en Steamworks.
4. El backend llama a AuthenticateUserTicket; fijar AppID e identity desde configuración del servidor, no desde datos arbitrarios del cliente.

```bash
curl --get 'https://partner.steam-api.com/ISteamUserAuth/AuthenticateUserTicket/v1/' \
  --data-urlencode 'key=REPLACE_WITH_PUBLISHER_WEB_API_KEY' \
  --data-urlencode 'appid=REPLACE_WITH_STEAM_APP_ID' \
  --data-urlencode 'ticket=REPLACE_WITH_HEX_TICKET' \
  --data-urlencode 'identity=REPLACE_WITH_EXPECTED_IDENTITY'
```

Comprobar éxito del servicio y `response.params.result == "OK"`; usar `steamid` como cadena de 64 bits para evitar pérdida de precisión. No aceptar un SteamID declarado por el cliente en lugar del devuelto por Steam. Proteger las URLs de esta llamada frente a logs: contienen API key/ticket. La petición de ejemplo muestra parámetros, no credenciales utilizables.

La identidad y la propiedad del juego son comprobaciones distintas: aplicar `CheckAppOwnership` y la política de licencias/Family Sharing requerida por el producto. Al rechazar, no crear sesión. Fuentes: [autenticación Steamworks](https://partner.steamgames.com/doc/features/auth), [ISteamUserAuth](https://partner.steamgames.com/doc/webapi/ISteamUserAuth), [GetAuthTicketForWebApi](https://partner.steamgames.com/doc/api/ISteamUser#GetAuthTicketForWebApi).

## Epic Games

1. Registrar el producto y configurar EOS, cliente y políticas en el Developer Portal.
2. Para Epic Account Services iniciar sesión con EOS Auth. Si el juego arranca desde Epic Games Launcher, consumir el exchange code con el flujo soportado del SDK; es efímero y no debe registrarse ni reutilizarse.
3. Obtener el ID token mediante `EOS_Auth_CopyIdToken` y enviarlo al backend propio por HTTPS. No sustituirlo por un ID de usuario declarado ni por un access token de formato distinto.
4. Verificar mediante `EOS_Auth_VerifyIdToken` o validación JWT completa conforme al contrato de Epic: firma y algoritmo permitido, emisor esperado, audience del ClientID configurado, tiempos/expiración y subject esperado. Rechazar `alg:none`, claves ajenas, expirados y audience de otra aplicación.

Descubrimiento de claves públicas para la verificación compatible con el contrato actual de Epic:

```bash
curl 'https://api.epicgames.dev/epic/oauth/v2/.well-known/jwks.json'
```

Usar una biblioteca JWT mantenida, seleccionar por `kid`, cachear claves con renovación acotada y fijar emisor/audience a configuración confiable; nunca descargar `jku`/`x5u` de un token no confiable. La guía no implementa un verificador criptográfico propio.

`EOS Auth` identifica un **EpicAccountID**; `EOS Connect` usa un **ProductUserID**. No intercambiarlos ni aplicar el verificador de Auth a un token Connect. Validar también licencia/entitlement mediante los servicios Ecom pertinentes si se necesita acceso al producto; un ID token válido solo prueba identidad.

Fuentes: [ID tokens de Epic Account Services](https://dev.epicgames.com/docs/epic-online-services/accounts-and-social/eos-epic-account-services/auth-interface/retrieve-and-validate-id-tokens?lang=en-US), [autenticación Web API](https://dev.epicgames.com/docs/web-api-ref/authentication), [políticas de cliente](https://dev.epicgames.com/docs/epic-online-services/eos-fundamentals/client-and-client-policy/client-policy-guide).

## Sesión propia del juego

El estudio implementa su endpoint HTTPS de login; no hay una ruta universal compartida por estos proveedores. Identificar cuentas por `(proveedor, subject verificado)`, aplicar rate limiting y timeouts, no registrar credenciales y emitir sesiones propias con expiración/revocación. Para vincular proveedores exigir sesión y prueba de control de ambas cuentas; no fusionar automáticamente por nombre o email.

Verificar, con las cuentas de prueba del estudio, tokens correctos, falsificados, expirados, otra aplicación y usuarios sin licencia. Este repositorio no incluye credenciales Steam/Epic ni ejecuta pruebas contra sus servicios.
