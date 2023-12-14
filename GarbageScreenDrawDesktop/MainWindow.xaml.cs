using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
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
        //private static extern void mouse_event(MouseEventFlags dwFlags, int dx, int dy, uint dwData, int dwExtraInfo);
        private static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, int dwExtraInfo);

        //[DllImport("user32.dll")]
        //static extern bool SetCursorPos(int X, int Y);

        //[Flags]
        //private enum MouseEventFlags
        //{
        //    LeftDown = 0x00000002,
        //    LeftUp = 0x00000004,
        //    RightDown = 0x00000008,
        //    RightUp = 0x00000010,
        //    MiddleDown = 0x00000020,
        //    MiddleUp = 0x00000040,
        //    Wheel = 0x00000800,
        //    Absolute = 0x00008000
        //}

        static uint DOWN = 0x0002;
        static uint UP = 0x0004;
        static uint MOUSEEVENTF_ABSOLUTE = 0x8000;
        static uint MOUSEEVENTF_MOVE = 0x0001;


        double radius = 5;
        const int MIN_POINTS = 10;
        int width = 3440;
        int height = 1440;

        RenderTargetBitmap renderTarget = null;

        private const int GWL_EXSTYLE = -20;
        private const int WS_EX_LAYERED = 0x00080000;
        private const int WS_EX_TRANSPARENT = 0x00000020;
        [DllImport("user32.dll")]
        private static extern int GetWindowLong(IntPtr hwnd, int index);
        [DllImport("user32.dll")]
        private static extern int SetWindowLong(IntPtr hwnd, int index, int newStyle);

        public MainWindow()
        {
            InitializeComponent();

            WindowStyle = WindowStyle.None; // Borderless window
            WindowState = WindowState.Maximized; // Maximize to cover the entire screen
            Topmost = true;
            Opacity = 0.5f;
            AllowsTransparency = true;
            IsHitTestVisible = false;
            Background = Brushes.Transparent;
            InputBindings.Clear();
            //IsEnabled = false;

            Focusable = false;

            renderTarget = new RenderTargetBitmap(width, height, 96, 96, PixelFormats.Default);
            renderTarget.Clear();

             drawingVisual = new DrawingVisual();
            using (DrawingContext drawingContext = drawingVisual.RenderOpen())
            {
                SolidColorBrush brush = Brushes.Red; // Choose your desired color
                double centerX = width / 2;
                double centerY = height / 2;
                drawingContext.DrawEllipse(brush, null, new Point(centerX, centerY), width / 6, height / 6);
            }
            renderTarget.Render(drawingVisual);

            // Use the renderTarget (RenderTargetBitmap) as a texture or source
            Image image = new Image { Source = renderTarget };
            image.Stretch = Stretch.Fill;
            //image.Width = width;
            //image.Height = height;
            Content = image;
            image.Opacity = 1;
            image.IsHitTestVisible = false;
            image.Focusable = false;
            image.AllowDrop = false;
            image.InputBindings.Clear();
            image.InputScope = null;

            //Canvas.SetLeft(image, 0);
            //Canvas.SetTop(image, 0);

            //Thread newThread = new Thread(Run);
            //newThread.Priority = ThreadPriority.Highest;
            //newThread.Start();
        }
        DrawingVisual drawingVisual;

        protected override void OnSourceInitialized(EventArgs e)
        {
            IntPtr hWnd = new System.Windows.Interop.WindowInteropHelper(this).Handle;
            int extendedStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
            SetWindowLong(hWnd, GWL_EXSTYLE, extendedStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);

            Task.Run(() =>
            {
            Run();
            });
        }

        private void Run()
        {
            using (var writer = new StreamWriter("output.txt"))
            {
                Console.SetOut(writer);

                while (true)
                {
                    Console.WriteLine("Starting up");
                    writer.Flush();

                    //TcpListener server = new TcpListener(IPAddress.Any, 8987);
                    TcpListener server = new TcpListener(IPAddress.Any, 8987);
                    server.Start();

                    float prevx = 0, prevy = 0;

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

                        List<float> points = new();

                        using (TcpClient client = server.AcceptTcpClient())
                        using (NetworkStream stream = client.GetStream())
                        using (BinaryReader reader = new BinaryReader(stream))
                        {
                            client.ReceiveBufferSize = ((4 + 1) * 4) * 16;

                            Console.WriteLine("???");
                            writer.Flush();
                            while (client.Connected)
                            {
                                if (stream.DataAvailable)
                                {
                                    int counter = 0;
                                    while (reader.ReadByte() != a || reader.ReadByte() != b || reader.ReadByte() != c || reader.ReadByte() != d)
                                    {
                                        Console.WriteLine("Seeking!");
                                        writer.Flush();
                                        if (counter++ > 30)
                                            throw new System.Exception("Too many!");
                                    }

                                    int type = reader.ReadInt32();
                                    if (type < 0)
                                    {
                                        type = -type;
                                    }
                                    float x = reader.ReadSingle();
                                    float y = reader.ReadSingle();
                                    float pressure = reader.ReadSingle();
                                    //Console.WriteLine("Received: " + type + " " + x + " " + y + " " + pressure);
                                    //writer.Flush();

                                    float dx = (x - prevx) * width;
                                    float dy = (y - prevy) * height;
                                    float dist = MathF.Sqrt(dx * dx + dy * dy);

                                    const float minDist = 5.0f;
                                    if (dist > minDist || !stream.DataAvailable || type != 0) // 
                                    {
                                        prevx = x;
                                        prevy = y;

                                        points.Add(x * width);
                                        points.Add(y * height);

                                        //x /= 2200;
                                        //y /= 1650;
                                        x *= 65535;
                                        y *= 65535;

                                        int xx = (int)x;
                                        int yy = (int)y;

                                        //System.Windows.Forms.Cursor.Position = new System.Drawing.Point(xx, yy);
                                        //SetCursorPos(xx, yy);

                                        //if (type == 1)
                                        //    mouse_event(MouseEventFlags.LeftDown, 0, 0, 0, 0);
                                        //else if (type == 2)
                                        //    mouse_event(MouseEventFlags.LeftUp, 0, 0, 0, 0);

                                        uint flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                                        if (type == 1)
                                            flags |= DOWN;
                                        else if (type == 2)
                                            flags |= UP;
                                        mouse_event(flags, xx, yy, 0, 0);

                                        Thread.Sleep(TimeSpan.FromTicks(1000));
                                    }
                                }
                                if (points.Count > MIN_POINTS * 2 || (points.Count > 0 && !stream.DataAvailable))
                                {
                                    List<float> points2 = new(points);

                                    Application.Current.Dispatcher.Invoke(() =>
                                    {
                                        using (DrawingContext drawingContext = drawingVisual.RenderOpen())
                                        {
                                            SolidColorBrush brush = Brushes.Red; // Choose your desired color
                                            for (int i = 0; i < points2.Count; i += 2)
                                            {
                                                //Console.WriteLine("Drawing: " + points2[i + 0] + ", " + points2[i + 1]);
                                                drawingContext.DrawEllipse(brush, null, new Point(points2[i + 0], points2[i + 1]), radius, radius);
                                            }
                                            points2.Clear();
                                        }

                                        renderTarget.Render(drawingVisual);
                                    });
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
