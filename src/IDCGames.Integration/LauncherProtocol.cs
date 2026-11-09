using System.Text;
using System.Text.Json;

namespace IDCGames.Integration;

public enum LauncherCommand : uint
{
    Error=0xFBB65363, UserData=0x03275C4A, BadUserInitData=0xABA83A28,
    PurchaseNotification=0x733F2850, NewPurchaseCode=0xFA4967C3,
    PurchaseOk=0x8218D08C, PurchaseError=0xBB485D88,
    BadPurchaseCode=0x6A6AC964, PurchaseClosed=0x6C1CCF80, VideoAdResult=0xB1DE0A7D
}

public sealed class LoginUser
{
    public required string UserId { get; init; }
    public string Nick { get; init; } = "";
    // Historical field tokenUserGameId may carry different credential formats.
    // Forward only to your trusted backend; never log it or infer its validity locally.
    public string Credential { get; init; } = "";
    public override string ToString() => $"IDC user {UserId} (credential redacted)";
}

public sealed record PurchaseNotice(string TransactionId, string PaymentId);
public sealed record VideoAdResult(string Status, string GameId, string SessionId, string ErrorCode)
{
    public bool IsTerminal => Status is "OK" or "SKIPPED" or "NO_FILL" or "ERROR";
}

public sealed class LauncherNotification
{
    public uint Command { get; init; }
    public LoginUser? User { get; init; }
    public PurchaseNotice? Purchase { get; init; }
    public VideoAdResult? VideoAd { get; init; }
    public string? ErrorCode { get; init; }
    // Local IPC metadata only; never use this as a payment or reward receipt.
    public override string ToString() => $"Launcher event 0x{Command:X8}";
}

public static class LauncherProtocol
{
    public const int MaxPayloadBytes = 1024 * 1024;
    private static readonly Encoding Utf16 = new UnicodeEncoding(false, false, true);

    public static LauncherNotification Decode(uint command, byte[] bytes)
    {
        ArgumentNullException.ThrowIfNull(bytes);
        if (bytes.Length == 0 || bytes.Length > MaxPayloadBytes) throw new FormatException("Invalid callback length.");
        string rawUtf8 = Encoding.UTF8.GetString(bytes).Trim('\0', ' ', '\r', '\n');
        if (rawUtf8.StartsWith("VIDEO_AD|", StringComparison.Ordinal))
        {
            string[] parts = rawUtf8.Split('|');
            if (parts.Length < 4) throw new FormatException("Incomplete VIDEO_AD callback.");
            return new LauncherNotification { Command=(uint)LauncherCommand.VideoAdResult,
                VideoAd=ReadVideo(parts[1], parts[2], parts[3], parts.Length > 4 ? parts[4] : "") };
        }
        if (bytes.Length % 2 != 0) throw new FormatException("UTF-16 callback must have an even byte count.");
        string json = Utf16.GetString(bytes).TrimEnd('\0', ' ', '\r', '\n', '\t');
        // Parse one JSON value. Some legacy DLL builds append bytes beyond the first object.
        // A JSON reader, unlike brace counting, handles braces/escapes inside strings correctly.
        var reader = new Utf8JsonReader(Encoding.UTF8.GetBytes(json));
        using var document = JsonDocument.ParseValue(ref reader);
        JsonElement root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object) throw new FormatException("Expected JSON object.");
        LoginUser? user = null;
        PurchaseNotice? purchase = null;
        VideoAdResult? video = null;
        if (command == (uint)LauncherCommand.UserData)
        {
            if (Number(root,"status") != 0 || !root.TryGetProperty("extraParams", out JsonElement data))
                throw new FormatException("UserData did not contain a successful login.");
            string id = Text(data,"userId");
            if (!long.TryParse(id, out long numericId) || numericId <= 0) throw new FormatException("Invalid IDC user ID.");
            user = new LoginUser { UserId=id, Nick=Text(data,"nick"), Credential=Text(data,"tokenUserGameId") };
        }
        if (command == (uint)LauncherCommand.PurchaseNotification)
        {
            string tx = Text(root,"dllTransaction"), payment = Text(root,"idcpaymentTransaction");
            if (tx.Length == 0 || payment.Length == 0) throw new FormatException("Incomplete purchase notification.");
            purchase = new PurchaseNotice(tx,payment);
        }
        if (command == (uint)LauncherCommand.VideoAdResult)
            video = ReadVideo(Text(root,"status"), Text(root,"gameId"), Text(root,"sessionId"), Text(root,"errorCode"));
        return new LauncherNotification { Command=command, User=user, Purchase=purchase, VideoAd=video,
            ErrorCode=command is (uint)LauncherCommand.Error or (uint)LauncherCommand.BadUserInitData ? "launcher_rejected" : null };
    }
    private static int? Number(JsonElement obj,string field)
        => obj.TryGetProperty(field,out var value) && value.ValueKind == JsonValueKind.Number && value.TryGetInt32(out int number) ? number : null;
    private static string Text(JsonElement obj,string field)
    {
        if (!obj.TryGetProperty(field,out var value)) return "";
        return value.ValueKind == JsonValueKind.String ? value.GetString() ?? "" : value.ValueKind == JsonValueKind.Number ? value.GetRawText() : "";
    }
    private static VideoAdResult ReadVideo(string status,string game,string session,string error)
    {
        if (status is not ("STARTED" or "OK" or "SKIPPED" or "NO_FILL" or "ERROR")) throw new FormatException("Unknown video status.");
        return new VideoAdResult(status,game,session,error);
    }
}
