using IDCGames.Integration;
using System.Runtime.InteropServices;
using System.Text;

Console.OutputEncoding=Encoding.UTF8;
if(args.Contains("--check-native"))
{
    var handle=NativeLibrary.Load(Path.Combine(AppContext.BaseDirectory,"IDCLib2.dll"));
    try { foreach(var name in new[]{"_InitIDCLib","_PullMessage","_OpenShop","_ClosePurchase","_EndIDCLib"}) { NativeLibrary.GetExport(handle,name); Console.WriteLine($"OK {name}"); } }
    finally { NativeLibrary.Free(handle); }
    return;
}
if(args.Contains("--demo"))
{
    Console.WriteLine("OFFLINE DEMO: no Launcher, HTTP calls, ads, purchases or rewards.");
    foreach(var sample in new[]{
        ((uint)LauncherCommand.UserData,"""{"status":0,"extraParams":{"userId":123,"nick":"Demo","tokenUserGameId":"example-only"}}"""),
        ((uint)LauncherCommand.PurchaseNotification,"""{"dllTransaction":"demo-tx","idcpaymentTransaction":"demo-payment"}"""),
        ((uint)LauncherCommand.VideoAdResult,"""{"status":"OK","gameId":"123","sessionId":"demo-ad","errorCode":""}""")})
    {
        var n=LauncherProtocol.Decode(sample.Item1,Encoding.Unicode.GetBytes(sample.Item2));
        Console.WriteLine(n.User!=null ? n.User.ToString() : n.VideoAd!=null ? $"Ad status {n.VideoAd.Status}; no reward granted" : "Purchase notice; no delivery or acknowledgement");
    }
    return;
}
string? gameId=Environment.GetEnvironmentVariable("IDC_GAME_ID"), key=Environment.GetEnvironmentVariable("IDC_GAME_CLIENT_KEY");
if(string.IsNullOrWhiteSpace(gameId)||string.IsNullOrWhiteSpace(key))
{
    Console.WriteLine("Set IDC_GAME_ID and IDC_GAME_CLIENT_KEY, then run with the Launcher logged in. Use --demo for offline validation or --check-native to check the DLL without login.");
    return;
}
using var client=new LauncherClient(gameId,key);
client.Notification+=n=>
{
    if(n.User!=null) Console.WriteLine($"Login received for IDC user {n.User.UserId}. Validate the credential in your backend; it is not printed.");
    else if(n.Purchase!=null) Console.WriteLine($"Purchase notice: tx={n.Purchase.TransactionId}, payment={n.Purchase.PaymentId}. Verify/deliver in backend before ack.");
    else if(n.VideoAd!=null) Console.WriteLine($"Video: {n.VideoAd.Status}, session={n.VideoAd.SessionId}. No reward is granted by this example.");
    else Console.WriteLine($"Event: 0x{n.Command:X8} {n.ErrorCode}");
};
try { client.Start(); }
catch(Exception e) when(e is DllNotFoundException or BadImageFormatException or EntryPointNotFoundException or InvalidOperationException)
{ Console.Error.WriteLine($"Cannot initialize: {e.Message}"); Environment.ExitCode=1; return; }
Console.WriteLine("Waiting for UserData. Commands: shop, shop-live, ad, ad-live, ack <tx> <payment>, quit. shop/ad default to sandbox previews.");
Task<string?> input=Task.Run(Console.ReadLine);
while(true)
{
    client.Poll();
    if(!input.IsCompleted) { await Task.Delay(16); continue; }
    string? line=await input;
    if(line==null || line.Equals("quit",StringComparison.OrdinalIgnoreCase)) break;
    try
    {
        string[] parts=line.Split(' ',StringSplitOptions.RemoveEmptyEntries);
        switch(parts.FirstOrDefault()?.ToLowerInvariant())
        {
            case "shop": case "shop-live": client.OpenShop(Guid.NewGuid().ToString(),sandbox:parts[0]=="shop"); break;
            case "ad": case "ad-live": await client.OpenVideoAdAsync(sandbox:parts[0]=="ad"); Console.WriteLine("Request sent. Wait for the terminal callback before requesting another ad."); break;
            case "ack" when parts.Length==3: client.AcknowledgePurchase(parts[1],parts[2]); Console.WriteLine("Acknowledgement sent. This does not replace backend receipt validation."); break;
            default: Console.WriteLine("Unknown command."); break;
        }
    }
    catch(Exception e) when(e is ArgumentException or InvalidOperationException or IOException or OperationCanceledException) { Console.Error.WriteLine(e.Message); }
    input=Task.Run(Console.ReadLine);
}
