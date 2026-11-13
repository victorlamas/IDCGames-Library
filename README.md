# Integración de IDC Games, Steam y Epic

Guía independiente de GamePanel para el estudio. Incluye una biblioteca C# y un ejemplo funcional para integrar **IDC Games Launcher (Windows x64)**, más los flujos de autenticación de Steam y Epic para el backend del juego.

En Admin, **Publisher** es la cuenta que publica el juego. **Developer** es quien lo desarrolla y puede ser la misma entidad u otra. Para el alta, las claves MCP y los créditos de la ficha, consulta la [guía oficial de Publisher](https://admin.idcgames.com/publisher/get-started).

Repositorio del estudio/Nvia: https://github.com/victorlamas/IDCGames-Library. La documentación oficial de IDC Games se publica por separado en https://github.com/idcgames/idcgames_integration y trata únicamente de IDC Games.

## Empezar

1. Para compilar o usar `dotnet run`: Windows x64, .NET SDK 8 o posterior y runtime .NET 8 x64 para ejecutar el ejemplo. Si Visual Studio ya incluye esos componentes, no hace falta instalarlos de nuevo. Para ejecutar el [ejemplo compilado](https://github.com/victorlamas/IDCGames-Library/releases/tag/v2.0.0), descomprimir el ZIP completo y ejecutar `LauncherDemo.exe --demo`: basta el runtime .NET 8 x64, sin SDK.
2. Ejecutar `dotnet run --project examples/LauncherDemo -- --demo` para ver eventos de ejemplo sin conexiones ni compras.
3. Ejecutar `dotnet run --project examples/LauncherDemo -- --check-native` para cargar la DLL y comprobar sus cinco exports.
4. Seguir [el ejemplo C#](docs/quickstart-csharp.md) con las credenciales de aplicación facilitadas por IDC Games.

## Documentación

- [Autenticación IDC Games](docs/authentication-idc.md): credenciales, peticiones y límites del contrato actual.
- [Autenticación de los tres proveedores](docs/authentication-idc-steam-epic.md): IDC, Steam y Epic; validación y sesiones del backend.
- [Launcher y ABI nativa](docs/launcher-protocol.md).
- [Tienda, compras y anuncios de vídeo](docs/shop-and-ads.md).
- [DLL, procedencia y compilación nativa](native/README.md).

## Contenido

| Ruta | Uso |
|---|---|
| `src/IDCGames.Integration` | Wrapper administrado .NET 8 para el Launcher IDC |
| `examples/LauncherDemo` | Consola de login, tienda y vídeo; sandbox por defecto |
| `runtimes/win-x64/native` | DLL IDC actual y manifiesto SHA-256 |
| `native/IDCLib2` | Fuentes C++ importadas del Launcher |
| `tests/IDCGames.Integration.Tests` | Pruebas offline sin cuentas ni servicios externos |
| `tools` | Verificación y empaquetado manual; no GitHub Actions |
| `legacy` | Material de 2020 conservado solo como referencia histórica |

La biblioteca implementa IPC de IDC Games; no es un SDK de Steam/Epic. Esos proveedores se integran con sus SDK oficiales según la guía. No requiere GamePanel.

## Verificar y distribuir

Ejecutar `powershell -ExecutionPolicy Bypass -File tools/verify.ps1` y después `powershell -ExecutionPolicy Bypass -File tools/build.ps1`. Los paquetes quedan en `artifacts/`; no se publican automáticamente en NuGet.

No se incluyen credenciales reales. El aviso local de login no demuestra identidad ante el servidor; los avisos de compra o vídeo tampoco son recibos verificables. La prueba completa con Launcher y servicios reales requiere la configuración de cada juego. Licencia existente: [GPL-3.0](LICENSE); confirmar con IDC Games las condiciones de distribución aplicables antes de integrar el binario en otro producto.
