using System.Reflection;
using System.Runtime.InteropServices;

namespace IDCGames.Integration;

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate void NotifyCallback(uint command, IntPtr data, int byteLength);
[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate void LogCallback(IntPtr data, int byteLength);

internal interface INativeApi
{
    int Init(string gameId, string clientKey, uint processId, NotifyCallback notify, LogCallback log);
    int Poll();
    int OpenShop(string transactionId, string extra, int userLevel, bool sandbox, string article);
    int ClosePurchase(string transactionId, string paymentId);
    int End();
}

internal sealed class NativeApi : INativeApi
{
    static NativeApi()
    {
        NativeLibrary.SetDllImportResolver(typeof(NativeApi).Assembly, Resolve);
    }
    private static IntPtr Resolve(string name, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (name != "IDCLib2") return IntPtr.Zero;
        // Never resolve a plugin from the caller's current directory or PATH.
        return NativeLibrary.Load(Path.Combine(AppContext.BaseDirectory, "IDCLib2.dll"));
    }
    public int Init(string gameId, string clientKey, uint processId, NotifyCallback notify, LogCallback log)
        => InitNative(gameId, clientKey, processId, notify, log, true);
    public int Poll() => PollNative();
    public int End() => EndNative();
    public int OpenShop(string transactionId, string extra, int userLevel, bool sandbox, string article)
        => OpenNative(transactionId, extra, userLevel, sandbox, article);
    public int ClosePurchase(string transactionId, string paymentId) => CloseNative(transactionId, paymentId);

    [DllImport("IDCLib2", EntryPoint="_InitIDCLib", ExactSpelling=true, CharSet=CharSet.Unicode, CallingConvention=CallingConvention.StdCall)]
    private static extern int InitNative(string gameId, string token, uint processId, NotifyCallback notify, LogCallback log, [MarshalAs(UnmanagedType.I1)] bool useQueue);
    [DllImport("IDCLib2", EntryPoint="_PullMessage", ExactSpelling=true, CallingConvention=CallingConvention.StdCall)]
    private static extern int PollNative();
    [DllImport("IDCLib2", EntryPoint="_EndIDCLib", ExactSpelling=true, CallingConvention=CallingConvention.StdCall)]
    private static extern int EndNative();
    [DllImport("IDCLib2", EntryPoint="_OpenShop", ExactSpelling=true, CharSet=CharSet.Unicode, CallingConvention=CallingConvention.StdCall)]
    private static extern int OpenNative(string tx, string extra, int level, [MarshalAs(UnmanagedType.I1)] bool sandbox, string article);
    [DllImport("IDCLib2", EntryPoint="_ClosePurchase", ExactSpelling=true, CharSet=CharSet.Unicode, CallingConvention=CallingConvention.StdCall)]
    private static extern int CloseNative(string tx, string paymentId);
}
