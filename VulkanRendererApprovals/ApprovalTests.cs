using ImageSharp;
using ImageSharp.Formats;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using Xunit;

namespace VulkanRendererApprovals
{
    public class ApprovalTests
    {
        static ApprovalTests()
        {
            Configuration.Default.AddImageFormat(new PngFormat());
        }

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
            using (var approved = new Image(Path.Combine(directoryName, $"Images\\{callerMemberName}.approved.png")))
            using (var received = new Image(Path.Combine(directoryName, $"Images\\{callerMemberName}.received.png")))
            {
                Assert.Equal(approved.Pixels, received.Pixels);
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
