using System;
using System.Security.Cryptography;
class Program {
    static int Main() {
        try {
            var version = Environment.Version;
            Console.WriteLine("AILINUX_CLR_VERSION=" + version);
            decimal rounded = decimal.Round(1.235m, 2, MidpointRounding.AwayFromZero);
            Console.WriteLine("ROUND_RESULT=" + rounded.ToString(System.Globalization.CultureInfo.InvariantCulture));
            byte[] digest;
            using (SHA256 sha = SHA256.Create()) digest = sha.ComputeHash(new byte[] {1, 2, 3});
            Console.WriteLine("SHA256_LEN=" + digest.Length);
            Console.WriteLine("CLR_PASS=" + (version.Major == 4 && rounded == 1.24m && digest.Length == 32));
            return version.Major == 4 && rounded == 1.24m && digest.Length == 32 ? 0 : 5;
        }
        catch (Exception ex) {
            Console.Error.WriteLine(ex.ToString());
            return 7;
        }
    }
}
