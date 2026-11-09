# Biblioteca nativa IDC Games

`runtimes/win-x64/native/IDCLib2.dll` se importa del ejemplo CSharp_Test del Launcher IDC Games actual. Arquitectura PE: x64. Tamaño: 174080 bytes. SHA-256:

```text
0f80a973254a21d0a08950b0b0ecf39da24c2cf7b9a24da64ba2c664f266af51
```

El [manifiesto](../runtimes/win-x64/native/manifest.json) registra la procedencia. Las fuentes de `IDCLib2/` se importan del mismo árbol del Launcher. **No se afirma reproducibilidad binaria ni se ha recompilado esta DLL en esta entrega.** La comprobación automática carga el binario y resuelve sus exports sin iniciar comunicación con el Launcher.

Para compilar las fuentes: abrir `IDCLib2/IDCLib2.sln` con Visual Studio y herramientas C++ de Windows; el proyecto original usa toolset v142. Seleccionar Release/x64, instalar el Windows SDK requerido por el proyecto o retargetearlo conscientemente. Revisar y probar cualquier binario recompilado antes de sustituir el distribuido y actualizar el manifiesto.

El código nativo heredado contiene buffers fijos y mecanismos de compatibilidad históricos. El wrapper aplica límites a sus entradas, pero no convierte el IPC local en un protocolo autenticado ni constituye una auditoría completa del código nativo. No cargar claves de servicios internos en esta biblioteca. Las condiciones de redistribución del binario deben confirmarse con IDC Games; se conserva la licencia original del repositorio.
