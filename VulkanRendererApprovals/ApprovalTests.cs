using SixLabors.ImageSharp;
using SixLabors.ImageSharp.Formats;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using Xunit;

namespace VulkanRendererApprovals
{
    public class ApprovalTests
    {
        static void RenderModel(string modelPath, [CallerMemberName] string callerMemberName = null, [CallerFilePath] string callerFilePath = null)
        {
            var directoryName = Path.GetDirectoryName(callerFilePath);
            Process.Start(new ProcessStartInfo
            {
                FileName = Path.Combine(directoryName, "../x64/Release/VulkanRenderer.exe"),
                WorkingDirectory = directoryName,
                Arguments = $"--model \"{modelPath}\" --image \"Images\\{callerMemberName}.received.png\""
            });
        }

        static void VerifyImage([CallerMemberName] string callerMemberName = null, [CallerFilePath] string callerFilePath = null)
        {
            var directoryName = Path.GetDirectoryName(callerFilePath);
            using (var approved = Image.Load(Path.Combine(directoryName, $"Images\\{callerMemberName}.approved.png")))
            using (var received = Image.Load(Path.Combine(directoryName, $"Images\\{callerMemberName}.received.png")))
            {
                Assert.Equal(approved.SavePixelData(), received.SavePixelData());
            }
        }

        [Fact]
        public void Bunny()
        {
            RenderModel("Models\\bun_zipper.ply");
            VerifyImage();
        }
    }
}
