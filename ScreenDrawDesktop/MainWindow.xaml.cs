using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace ScreenDrawDesktop
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        [DllImport("user32.dll")]
        private static extern void mouse_event(MouseEventFlags dwFlags, int dx, int dy, uint dwData, int dwExtraInfo);

        [Flags]
        private enum MouseEventFlags
        {
            LeftDown = 0x00000002,
            LeftUp = 0x00000004,
            RightDown = 0x00000008,
            RightUp = 0x00000010,
            MiddleDown = 0x00000020,
            MiddleUp = 0x00000040,
            Wheel = 0x00000800,
            Absolute = 0x00008000
        }

        public MainWindow()
        {
            using (var writer = new StreamWriter("output.txt"))
            {
                Console.SetOut(writer);

                while (true)
                {
                    Console.WriteLine("Starting up");
                    writer.Flush();

                    TcpListener server = new TcpListener(IPAddress.Any, 8987);
                    server.Start();

                    try
                    {
                        Console.WriteLine("Waiting for Android client...");
                        writer.Flush();

                        int marker = 478934687;
                        byte[] intBytes = BitConverter.GetBytes(marker);
                        byte a = intBytes[3];
                        byte b = intBytes[2];
                        byte c = intBytes[1];
                        byte d = intBytes[0];

                        using (TcpClient client = server.AcceptTcpClient())
                        using (NetworkStream stream = client.GetStream())
                        using (BinaryReader reader = new BinaryReader(stream))
                        {
                            Console.WriteLine("???");
                            writer.Flush();
                            while (client.Connected)
                            {
                                if (stream.DataAvailable)
                                {
                                    int counter = 0;
                                    while (reader.ReadByte() != a || reader.ReadByte() != b || reader.ReadByte() != c || reader.ReadByte() != d)
                                    {
                                        //Console.WriteLine("Seeking!");
                                        //writer.Flush();
                                        if (counter++ > 30)
                                            throw new System.Exception("Too many!");
                                    }

                                    //writer.Flush();

                                    int type = reader.ReadInt32();
                                    float x = reader.ReadSingle();
                                    float y = reader.ReadSingle();
                                    float pressure = reader.ReadSingle();
                                    //Console.WriteLine("Received: " + type + " " + x + " " + y + " " + pressure);

                                    int xx = (int)x;
                                    int yy = (int)y;

                                    System.Windows.Forms.Cursor.Position = new System.Drawing.Point(xx, yy);

                                    if (type == 1)
                                        mouse_event(MouseEventFlags.LeftDown, 0, 0, 0, 0);
                                    else if (type == 2)
                                        mouse_event(MouseEventFlags.LeftUp, 0, 0, 0, 0);
                                    System.Threading.Thread.Sleep(TimeSpan.FromTicks(1000));
                                }
                            }
                        }
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine("Failed: " + e);
                    }
                    finally
                    {
                        server.Stop();
                    }
                }
            }
        }
    }
}
