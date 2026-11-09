using System.Collections.Concurrent;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

namespace IDCGames.Integration;

public sealed class LauncherClient : IDisposable
{
    private static int processOwner;
    private readonly INativeApi native;
    private readonly NotifyCallback notify;
    private readonly LogCallback log;
    private readonly ConcurrentQueue<LauncherNotification> pending = new();
    private readonly object gate = new();
    private bool started, disposed;
    public string GameId { get; }
    private readonly string clientKey;
    public bool IsAuthenticated { get; private set; }
    public event Action<LauncherNotification>? Notification;

    public LauncherClient(string gameId,string clientKey) : this(gameId,clientKey,new NativeApi()) { }
    internal LauncherClient(string gameId,string clientKey,INativeApi native)
    {
        if (string.IsNullOrEmpty(gameId) || gameId.Any(c => c < '0' || c > '9') || !long.TryParse(gameId,out long game) || game <= 0 || gameId.Length > 20) throw new ArgumentException("A positive IDC game ID is required.",nameof(gameId));
        if (string.IsNullOrWhiteSpace(clientKey) || clientKey.Length >= 64 || clientKey.Any(c => c < 33 || c > 126 || c == '"' || c == '\\'))
            throw new ArgumentException("The Launcher client key must be 1 to 63 printable ASCII characters.",nameof(clientKey));
        GameId=gameId; this.clientKey=clientKey; this.native=native;
        notify=OnNativeNotification;
        // Native debug output can contain credentials. Do not forward it to game logs.
        log=(_,_) => { };
    }
    public void Start()
    {
        lock(gate)
        {
            ObjectDisposedException.ThrowIf(disposed,this);
            if(started) throw new InvalidOperationException("Client already started.");
            if(Interlocked.CompareExchange(ref processOwner,1,0)!=0) throw new InvalidOperationException("Only one IDCLib2 client may run per process.");
            try
            {
                int result=native.Init(GameId,clientKey,(uint)Environment.ProcessId,notify,log);
                if(result!=0) throw new InvalidOperationException($"IDCLib2 initialization failed ({result}). Check the running Launcher and game configuration.");
                started=true;
            }
            catch { Interlocked.Exchange(ref processOwner,0); throw; }
        }
    }
    public void Poll(int maxMessages=64)
    {
        if(maxMessages is < 1 or > 1024) throw new ArgumentOutOfRangeException(nameof(maxMessages));
        lock(gate)
        {
            RequireStarted();
            for(int i=0;i<maxMessages;i++) { if(native.Poll()<=0) break; }
        }
        // User handlers execute outside native callbacks and native locks.
        for(int i=0;i<maxMessages && !disposed && pending.TryDequeue(out var item);i++)
        {
            if(item.User!=null) IsAuthenticated=true;
            if(item.Command is (uint)LauncherCommand.Error or (uint)LauncherCommand.BadUserInitData) IsAuthenticated=false;
            Notification?.Invoke(item);
        }
    }
    public void OpenShop(string transactionId,string article="",bool sandbox=true,int userLevel=0,string extra="")
    {
        CheckNativeString(transactionId,64,false); CheckNativeString(article,96,true); CheckNativeString(extra,128,true);
        if(userLevel<0) throw new ArgumentOutOfRangeException(nameof(userLevel));
        lock(gate) { RequireAuthenticated(); CheckResult(native.OpenShop(transactionId,extra,userLevel,sandbox,article),"open shop"); }
    }
    public void AcknowledgePurchase(string transactionId,string paymentId)
    {
        CheckNativeString(transactionId,64,false); CheckNativeString(paymentId,64,false);
        // Call only after the game's backend verifies and durably delivers this purchase.
        lock(gate) { RequireAuthenticated(); CheckResult(native.ClosePurchase(transactionId,paymentId),"acknowledge purchase"); }
    }
    public async Task<string> OpenVideoAdAsync(bool sandbox=true,string layout="fullscreen_monitor",CancellationToken cancellationToken=default)
    {
        if(layout is not ("fullscreen_monitor" or "fullscreen_game" or "windowed")) throw new ArgumentException("Unsupported layout.",nameof(layout));
        lock(gate) RequireAuthenticated();
        string requestId=Guid.NewGuid().ToString();
        byte[] json=JsonSerializer.SerializeToUtf8Bytes(new { videoAd=true,gameId=GameId,sandbox,requestId,layout=new { mode=layout } });
        using var timeout=CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeout.CancelAfter(TimeSpan.FromSeconds(3));
        using var pipe=new NamedPipeClientStream(".","IDCGamesPipe",PipeDirection.Out,PipeOptions.Asynchronous);
        await pipe.ConnectAsync(timeout.Token).ConfigureAwait(false);
        await pipe.WriteAsync(json,timeout.Token).ConfigureAwait(false);
        await pipe.FlushAsync(timeout.Token).ConfigureAwait(false);
        return requestId;
    }
    private void OnNativeNotification(uint command,IntPtr pointer,int length)
    {
        try
        {
            if(pointer==IntPtr.Zero || length<=0 || length>LauncherProtocol.MaxPayloadBytes) throw new FormatException("Invalid callback buffer.");
            var bytes=new byte[length]; Marshal.Copy(pointer,bytes,0,length);
            pending.Enqueue(LauncherProtocol.Decode(command,bytes));
        }
        catch(Exception) { pending.Enqueue(new LauncherNotification { Command=(uint)LauncherCommand.Error, ErrorCode="invalid_launcher_payload" }); }
    }
    private static void CheckNativeString(string value,int bytes,bool empty)
    {
        ArgumentNullException.ThrowIfNull(value);
        if((!empty && value.Length==0) || Encoding.UTF8.GetByteCount(value)>bytes || value.Any(c=>c<' ' || c=='"' || c=='\\'))
            throw new ArgumentException("Native parameter is empty, too long or contains unsupported JSON characters.");
    }
    private static void CheckResult(int result,string operation) { if(result!=0) throw new InvalidOperationException($"IDCLib2 could not {operation} ({result})."); }
    private void RequireStarted() { ObjectDisposedException.ThrowIf(disposed,this); if(!started) throw new InvalidOperationException("Start the integration first."); }
    private void RequireAuthenticated() { RequireStarted(); if(!IsAuthenticated) throw new InvalidOperationException("Wait for a successful UserData callback before opening shop or ads."); }
    public void Dispose()
    {
        lock(gate)
        {
            if(disposed) return;
            disposed=true; IsAuthenticated=false;
            if(started) { try { native.End(); } finally { started=false; Interlocked.Exchange(ref processOwner,0); } }
        }
        GC.KeepAlive(notify); GC.KeepAlive(log);
    }
}
