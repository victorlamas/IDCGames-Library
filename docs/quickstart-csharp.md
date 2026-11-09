# Ejemplo C# de IDC Games Launcher



Para compilar: Windows x64 y .NET SDK 8 o posterior. Para ejecutar la consola compilada: runtime .NET 8 x64; no hace falta el SDK. Un SDK posterior no implica tener instalado el runtime 8. Para conectar con servicios reales: Launcher IDC Games abierto y con sesión iniciada, juego dado de alta en IDC. Solicitar a IDC el `id_game`, la clave cliente del Launcher y la configuración de tienda/anuncios. Ninguno es un token de GamePanel.



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

## Qué requiere .NET y qué no

El ejemplo original `CSharp_Test/IDCTest.csproj` del Launcher es una consola C# con `TargetFramework=net8.0-windows` y `PlatformTarget=x64`. Su `run.bat` usa `dotnet run --configuration Release`, que compila y ejecuta; por eso necesita SDK. Su `IDCTest.runtimeconfig.json` pide `Microsoft.NETCore.App` 8.0.

La consola publicada en este repositorio también usa .NET 8. El ZIP actual es framework-dependent: requiere el runtime .NET 8 x64, pero no el SDK. Descomprimir todos los archivos juntos; no copiar solo el EXE. Ejecutar `LauncherDemo.exe --demo` para una prueba offline o configurar las variables y ejecutar `LauncherDemo.exe` para el flujo real.

Para comprobar una instalación existente:

```powershell
dotnet --list-sdks
dotnet --list-runtimes
```

Para entregar una consola que incluya el runtime y no exija instalar .NET en el equipo de destino, el desarrollador puede publicar una variante self-contained:

```powershell
dotnet publish examples/LauncherDemo -c Release -r win-x64 --self-contained true -o artifacts/LauncherDemo-selfcontained
```

Distribuir la carpeta completa resultante, incluida `IDCLib2.dll`. Esta variante no es el ZIP framework-dependent de la release actual y ocupa más espacio.

IDCLib2.dll es una biblioteca nativa de Windows x64. La dependencia .NET corresponde a esta consola y su wrapper C#; no obliga a usar .NET en un juego integrado desde otro lenguaje o motor. Este ejemplo .NET 8 tampoco es un proyecto .NET Framework 4.x.
