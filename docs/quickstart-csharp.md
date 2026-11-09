# Ejemplo C# de IDC Games Launcher



Requisitos: Windows x64, .NET SDK 8+, Launcher IDC Games abierto y con sesión iniciada, juego dado de alta en IDC. Solicitar a IDC el `id_game`, la clave cliente del Launcher y la configuración de tienda/anuncios. Ninguno es un token de GamePanel.



```powershell

$env:IDC_GAME_ID = "123" # Sustituir por el ID real del juego

$env:IDC_GAME_CLIENT_KEY = "REPLACE_WITH_LAUNCHER_CLIENT_KEY"

dotnet run --project examples/LauncherDemo -- --check-native

dotnet run --project examples/LauncherDemo

```



La DLL se copia automáticamente al directorio del ejemplo. Para un juego propio: referenciar el proyecto `src/IDCGames.Integration/IDCGames.Integration.csproj`, desplegar `IDCLib2.dll` junto al ejecutable y compilar/ejecutar x64. Si se usa el paquete NuGet generado, publicar para `win-x64` y verificar que la DLL nativa queda junto al ejecutable. No cargar una DLL desde una ruta suministrada por un usuario.



```csharp

using IDCGames.Integration;

using var idc = new LauncherClient(gameId, launcherClientKey);

idc.Notification += message => {

    if (message.User is { } user) {

        // Enviar user.UserId y user.Credential al backend propio por HTTPS.

        // Validar allí antes de crear la sesión del juego. No imprimir Credential.

    }

};

idc.Start();

// En el loop de actualización del juego/UI, cada ~16 ms:

idc.Poll();

// Mantener el cliente vivo; Dispose al cerrar el juego, después de parar Poll.

```



`Start()` sin excepción solo confirma inicialización. Esperar `UserData`; `IsAuthenticated` describe el estado local del Launcher, no una sesión validada por vuestro servidor. Solo una instancia nativa por proceso. Mantener `Poll()` y las llamadas del juego en el mismo hilo; no invocar `Dispose` simultáneamente desde otro hilo.



El ejemplo usa el PID del proceso y se basa en el flujo CSharp_Test del Launcher actual. Para la validación final usar la entrada del juego configurada en el Launcher y comprobar la asociación del proceso. Una app lanzada manualmente puede no corresponder a la configuración del juego instalada.



| Comando de consola | Acción |

|---|---|

| `shop` | Abrir tienda en sandbox |

| `shop-live` | Abrir checkout real; usar solo en pruebas autorizadas |

| `ad` | Abrir vídeo en modo preview/sandbox |

| `ad-live` | Solicitar anuncio real, sujeto a disponibilidad |

| `ack <tx> <payment>` | Confirmar recepción únicamente después de verificar y entregar en backend |

| `quit` | Finalizar la biblioteca |



`ack` es una operación manual de diagnóstico, no una comprobación de pago. El ejemplo no entrega artículos ni recompensas. Para errores: comprobar x64 y DLL, Launcher activo, juego/clave correctos, callback de rechazo y polling activo. No adjuntar tokens ni logs nativos completos a incidencias.



La DLL nativa distribuida es heredada y puede generar sus propios archivos de diagnóstico según la compilación. Aunque el wrapper descarta el callback de log, revisar y proteger también los archivos nativos; no activar ni compartir trazas que contengan credenciales.
