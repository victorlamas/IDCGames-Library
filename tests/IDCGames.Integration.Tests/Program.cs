using IDCGames.Integration;
using System.Runtime.InteropServices;
using System.Text;

int passed=0;
void Test(string name,Action action) { action(); passed++; Console.WriteLine($"PASS {name}"); }
void Check(bool condition) { if(!condition) throw new Exception("Assertion failed"); }
void Reject<T>(Action action) where T:Exception { try { action(); } catch(T) { return; } throw new Exception($"Expected {typeof(T).Name}"); }
LauncherNotification Decode(LauncherCommand c,string json)=>LauncherProtocol.Decode((uint)c,Encoding.Unicode.GetBytes(json));
const string Login="""{"status":0,"extraParams":{"userId":123,"nick":"A } B","tokenUserGameId":"test-only-credential"}}""";
Test("UTF16 bytes, braces and trailing native data",()=>{var n=Decode(LauncherCommand.UserData,Login+"\0ignored");Check(n.User?.UserId=="123" && n.User.Nick=="A } B");});
Test("credentials excluded from object logging",()=>Check(!Decode(LauncherCommand.UserData,Login).User!.ToString().Contains("test-only-credential")));
Test("unsuccessful login rejected",()=>Reject<FormatException>(()=>Decode(LauncherCommand.UserData,"""{"status":1,"extraParams":{"userId":123}}""")));
Test("string status rejected",()=>Reject<FormatException>(()=>Decode(LauncherCommand.UserData,"""{"status":"0","extraParams":{"userId":123}}""")));
Test("invalid user id rejected",()=>Reject<FormatException>(()=>Decode(LauncherCommand.UserData,"""{"status":0,"extraParams":{"userId":0}}""")));
Test("odd and oversized buffers rejected",()=>{Reject<FormatException>(()=>LauncherProtocol.Decode(0,new byte[1]));Reject<FormatException>(()=>LauncherProtocol.Decode(0,new byte[LauncherProtocol.MaxPayloadBytes+1]));});
Test("purchase field names and completeness",()=>{Check(Decode(LauncherCommand.PurchaseNotification,"""{"dllTransaction":"tx","idcpaymentTransaction":"pay"}""").Purchase==new PurchaseNotice("tx","pay"));Reject<FormatException>(()=>Decode(LauncherCommand.PurchaseNotification,"""{"transactionId":"tx"}"""));});
Test("video terminal states and compatibility",()=>{foreach(var status in new[]{"STARTED","OK","SKIPPED","NO_FILL","ERROR"}){var n=LauncherProtocol.Decode((uint)LauncherCommand.VideoAdResult,Encoding.UTF8.GetBytes($"VIDEO_AD|{status}|123|session|"));Check(n.VideoAd!.IsTerminal==(status!="STARTED"));}Reject<FormatException>(()=>Decode(LauncherCommand.VideoAdResult,"""{"status":"SUCCESS"}"""));});
Test("client input validation",()=>{foreach(var id in new[]{"0","-1"," 1","+1","1\""})Reject<ArgumentException>(()=>new LauncherClient(id,"example"));Reject<ArgumentException>(()=>new LauncherClient("1","bad\"key"));});
Test("init is not authentication; handlers outside native callback",()=>{
 var api=new Fake();using var c=new LauncherClient("123","example",api);c.Start();Check(!c.IsAuthenticated);Reject<InvalidOperationException>(()=>c.OpenShop("tx"));int events=0;c.Notification+=n=>{Check(!api.InCallback);events++;};api.Send(LauncherCommand.UserData,Login);Check(events==0);c.Poll();Check(c.IsAuthenticated && events==1);c.OpenShop("tx");Check(api.Shops==1);});
Test("one process owner and idempotent shutdown",()=>{var api=new Fake();var c=new LauncherClient("123","example",api);c.Start();using var other=new LauncherClient("123","example",new Fake());Reject<InvalidOperationException>(other.Start);c.Dispose();c.Dispose();Check(api.Ends==1);other.Start();});
Test("failed initialization releases process owner",()=>{using var bad=new LauncherClient("123","example",new Fake{InitResult=1});Reject<InvalidOperationException>(bad.Start);using var good=new LauncherClient("123","example",new Fake());good.Start();});
Test("malformed callback cannot escape to native code",()=>{var api=new Fake();using var c=new LauncherClient("123","example",api);c.Start();api.Send(LauncherCommand.UserData,"bad json");c.Poll();Check(!c.IsAuthenticated);});
Test("purchase notices never auto-acknowledge",()=>{var api=new Fake();using var c=new LauncherClient("123","example",api);c.Start();api.Send(LauncherCommand.UserData,Login);api.Send(LauncherCommand.PurchaseNotification,"""{"dllTransaction":"tx","idcpaymentTransaction":"pay"}""");c.Poll();Check(api.Acks==0);c.AcknowledgePurchase("tx","pay");Check(api.Acks==1);});
Test("native JSON injection and overflow blocked before PInvoke",()=>{var api=new Fake();using var c=new LauncherClient("123","example",api);c.Start();api.Send(LauncherCommand.UserData,Login);c.Poll();foreach(var value in new[]{"quote\"","slash\\","control\n",new string('x',65)})Reject<ArgumentException>(()=>c.OpenShop(value));Reject<ArgumentException>(()=>c.OpenShop("tx",article:new string('é',49)));Check(api.Shops==0);});
Test("rejection revokes local authenticated state",()=>{var api=new Fake();using var c=new LauncherClient("123","example",api);c.Start();api.Send(LauncherCommand.UserData,Login);c.Poll();api.Send(LauncherCommand.BadUserInitData,"{}");c.Poll();Check(!c.IsAuthenticated);Reject<InvalidOperationException>(()=>c.AcknowledgePurchase("tx","pay"));});
Console.WriteLine($"{passed} offline tests passed. No live provider calls.");

sealed class Fake:INativeApi {
 NotifyCallback? notify;public int InitResult,Shops,Acks,Ends;public bool InCallback;
 public int Init(string g,string k,uint p,NotifyCallback n,LogCallback l){notify=n;return InitResult;}
 public int Poll()=>0;
 public int OpenShop(string t,string e,int l,bool s,string a){Shops++;return 0;}
 public int ClosePurchase(string t,string p){Acks++;return 0;}
 public int End(){Ends++;return 0;}
 public void Send(LauncherCommand command,string value){byte[] bytes=Encoding.Unicode.GetBytes(value);IntPtr ptr=Marshal.AllocHGlobal(bytes.Length);try{Marshal.Copy(bytes,0,ptr,bytes.Length);InCallback=true;notify!((uint)command,ptr,bytes.Length);}finally{InCallback=false;Marshal.FreeHGlobal(ptr);}}
}
