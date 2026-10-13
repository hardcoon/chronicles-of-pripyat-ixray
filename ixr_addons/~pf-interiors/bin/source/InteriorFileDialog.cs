using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

[assembly: AssemblyTitle("Interior Layout File Dialog")]
[assembly: AssemblyCompany("Hardcoon")]
[assembly: AssemblyVersion("0.1.0.0")]

internal static class InteriorFileDialog
{
    private static readonly Encoding TextEncoding = Encoding.GetEncoding(1251);
    private const int MaxBytes = 262144;
    [DllImport("shell32.dll")]
    private static extern int SHGetKnownFolderPath(ref Guid id, uint flags, IntPtr token, out IntPtr path);
    [DllImport("user32.dll")]
    private static extern IntPtr GetActiveWindow();
    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll")]
    private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    private static void CancelDialog()
    {
        IntPtr window = GetActiveWindow();
        uint process;
        GetWindowThreadProcessId(window, out process);
        if (process == (uint)Process.GetCurrentProcess().Id)
            PostMessage(window, 0x0010, IntPtr.Zero, IntPtr.Zero);
    }
    private sealed class Owner : IWin32Window
    {
        public IntPtr Handle { get; private set; }
        public Owner(IntPtr handle) { Handle = handle; }
    }
    private static string Decode(string value)
    {
        List<byte> bytes = new List<byte>();
        byte[] input = TextEncoding.GetBytes(value);
        for (int i = 0; i < input.Length; i++)
        {
            if (input[i] == (byte)'%')
            {
                if (i + 2 >= input.Length) throw new InvalidDataException();
                bytes.Add(Convert.ToByte(Encoding.ASCII.GetString(input, i + 1, 2), 16));
                i += 2;
            }
            else bytes.Add(input[i]);
        }
        return TextEncoding.GetString(bytes.ToArray());
    }
    private static Dictionary<string, string> Read(string path)
    {
        Dictionary<string, string> result = new Dictionary<string, string>();
        if (!File.Exists(path)) return result;
        if (new FileInfo(path).Length > MaxBytes) throw new InvalidDataException();
        foreach (string line in File.ReadAllLines(path, TextEncoding))
        {
            int separator = line.IndexOf('=');
            if (separator > 0) result[line.Substring(0, separator)] = line.Substring(separator + 1);
        }
        return result;
    }
    private static void AtomicText(string path, string value)
    {
        string temporary = path + ".tmp";
        File.WriteAllText(temporary, value, TextEncoding);
        if (File.Exists(path)) File.Replace(temporary, path, null);
        else File.Move(temporary, path);
    }
    private static void CheckLayout(string path)
    {
        if (new FileInfo(path).Length > MaxBytes) throw new InvalidDataException();
        using (StreamReader reader = new StreamReader(path, TextEncoding, false))
            if (reader.ReadLine() != "PF_INTERIOR\t1") throw new InvalidDataException();
    }
    private static string Downloads()
    {
        Guid id = new Guid("374DE290-123F-4565-9164-39C4925E467B");
        IntPtr path;
        if (SHGetKnownFolderPath(ref id, 0, IntPtr.Zero, out path) == 0)
        {
            try { return Marshal.PtrToStringUni(path); }
            finally { Marshal.FreeCoTaskMem(path); }
        }
        return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");
    }
    private static string Folder(Dictionary<string, string> settings, string key)
    {
        string encoded;
        if (settings.TryGetValue(key, out encoded))
        {
            try
            {
                string decoded = Encoding.Unicode.GetString(Convert.FromBase64String(encoded));
                if (Directory.Exists(decoded)) return decoded;
            }
            catch (FormatException) { }
        }
        return Downloads();
    }
    private static string SafeName(string name)
    {
        foreach (char invalid in Path.GetInvalidFileNameChars()) name = name.Replace(invalid, '_');
        name = name.Trim().TrimEnd('.');
        return name.Length == 0 ? "interior.pfint" : name + ".pfint";
    }
    private static void SaveFolder(string path, Dictionary<string, string> settings, string key, string selected)
    {
        settings[key] = Convert.ToBase64String(Encoding.Unicode.GetBytes(Path.GetDirectoryName(selected)));
        StringBuilder text = new StringBuilder();
        foreach (KeyValuePair<string, string> pair in settings)
            text.Append(pair.Key).Append('=').Append(pair.Value).Append("\r\n");
        AtomicText(path, text.ToString());
    }
    [STAThread]
    private static int Main(string[] args)
    {
        if (args.Length == 1 && args[0] == "--self-test")
        {
            // A non-interactive check, also used by the addon build harness.
            if (Decode("%D2%E5%F1%F2") != "\u0422\u0435\u0441\u0442") return 1;
            if (!SafeName("a/b").Equals("a_b.pfint")) return 2;
            if (String.IsNullOrEmpty(Downloads())) return 3;
            return 0;
        }
        if (args.Length != 1) return 2;
        string job = Path.GetFullPath(args[0]);
        string directory = Path.GetDirectoryName(job);
        if (!job.EndsWith(".job", StringComparison.OrdinalIgnoreCase) ||
            !directory.EndsWith("\\pf_interiors\\io", StringComparison.OrdinalIgnoreCase)) return 2;
        string status = "error";
        try
        {
            Dictionary<string, string> request = Read(job);
            bool exporting = request["mode"] == "export";
            if (!exporting && request["mode"] != "import") throw new InvalidDataException();
            int parentId = Int32.Parse(request["parent"]);
            Process parent = Process.GetProcessById(parentId);
            if (parent.HasExited) return 2;
            string settingsPath = Path.Combine(Directory.GetParent(directory).FullName, "dialogs.ltx");
            Dictionary<string, string> settings = Read(settingsPath);
            string key = exporting ? "last_export" : "last_import";
            Application.EnableVisualStyles();
            using (FileDialog dialog = exporting ? (FileDialog)new SaveFileDialog() : new OpenFileDialog())
            using (System.Windows.Forms.Timer timer = new System.Windows.Forms.Timer())
            {
                dialog.Title = Decode(request["title"]);
                dialog.Filter = Decode(request["filter"]) + " (*.pfint)|*.pfint";
                dialog.DefaultExt = "pfint";
                dialog.AddExtension = true;
                dialog.RestoreDirectory = true;
                dialog.InitialDirectory = Folder(settings, key);
                if (exporting)
                {
                    dialog.FileName = SafeName(Decode(request["name"]));
                    ((SaveFileDialog)dialog).OverwritePrompt = true;
                }
                else ((OpenFileDialog)dialog).CheckFileExists = true;
                timer.Interval = 500;
                timer.Tick += delegate {
                    try { if (parent.HasExited || File.Exists(job + ".cancel")) CancelDialog(); }
                    catch (InvalidOperationException) { CancelDialog(); }
                };
                timer.Start();
                DialogResult choice = parent.MainWindowHandle == IntPtr.Zero ? dialog.ShowDialog() : dialog.ShowDialog(new Owner(parent.MainWindowHandle));
                timer.Stop();
                status = "cancel";
                if (choice == DialogResult.OK && !File.Exists(job + ".cancel"))
                {
                    string transfer = job + ".pfint";
                    if (exporting)
                    {
                        CheckLayout(transfer);
                        string temporary = dialog.FileName + ".partial." + Guid.NewGuid().ToString("N");
                        try
                        {
                            File.Copy(transfer, temporary, false);
                            if (File.Exists(dialog.FileName)) File.Replace(temporary, dialog.FileName, null);
                            else File.Move(temporary, dialog.FileName);
                        }
                        finally { if (File.Exists(temporary)) File.Delete(temporary); }
                    }
                    else
                    {
                        CheckLayout(dialog.FileName);
                        File.Copy(dialog.FileName, transfer, true);
                    }
                    // UTF-16 paths are stored as ASCII Base64 inside CP1251
                    // preferences, retaining Unicode file-dialog directories.
                    try { SaveFolder(settingsPath, settings, key, dialog.FileName); }
                    catch (IOException) { }
                    catch (UnauthorizedAccessException) { }
                    status = "ok";
                }
            }
        }
        catch (Exception) { status = "error"; }
        try { AtomicText(job + ".result", "status=" + status + "\r\n"); }
        catch (Exception) { return 1; }
        return status == "error" ? 1 : 0;
    }
}
