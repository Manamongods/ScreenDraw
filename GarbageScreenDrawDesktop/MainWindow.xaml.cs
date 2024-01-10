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
        private static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, int dwExtraInfo);

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

            WindowStyle = WindowStyle.None;
            WindowState = WindowState.Maximized;
            Topmost = true;
            Opacity = 0.5f;
            AllowsTransparency = true;
            IsHitTestVisible = false;
            Background = Brushes.Transparent;
            InputBindings.Clear();

            Focusable = false;

            renderTarget = new RenderTargetBitmap(width, height, 96, 96, PixelFormats.Default);
            renderTarget.Clear();

            drawingVisual = new DrawingVisual();
            //using (DrawingContext drawingContext = drawingVisual.RenderOpen())
            //{
            //    SolidColorBrush brush = Brushes.Red;
            //    double centerX = width / 2;
            //    double centerY = height / 2;
            //    drawingContext.DrawEllipse(brush, null, new Point(centerX, centerY), width / 6, height / 6);
            //}
            //renderTarget.Render(drawingVisual);

            Image image = new Image { Source = renderTarget };
            image.Stretch = Stretch.Fill;
            Content = image;
            image.Opacity = 1;
            image.IsHitTestVisible = false;
            image.Focusable = false;
            image.AllowDrop = false;
            image.InputBindings.Clear();
            image.InputScope = null;
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

        private static bool down = false;
        private void Run()
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

                    float prevx = 0, prevy = 0;

                    try
                    {
                        Console.WriteLine("Waiting for Android client...");
                        writer.Flush();

                        int marker = 478934687;
                        byte[] intBytes = BitConverter.GetBytes(marker);
                        byte a = intBytes[0];
                        byte b = intBytes[1];
                        byte c = intBytes[2];
                        byte d = intBytes[3];

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

                                    int inputType = reader.ReadInt32();
                                    float x = reader.ReadSingle();
                                    float y = reader.ReadSingle();
                                    float pressure = reader.ReadSingle();

                                    const float THRESHOLD = 0.25f;

                                    int type = 0;
                                    if (inputType == 0)
                                    {
                                        if (down)
                                        {
                                            down = false;
                                            type = 2;
                                        }
                                    }
                                    else
                                    {
                                        if (pressure > THRESHOLD)
                                        {
                                            if (!down)
                                            {
                                                down = true;
                                                type = 1;
                                            }
                                        }
                                        else
                                        {
                                            if (down)
                                            {
                                                down = false;
                                                type = 2;
                                            }
                                        }
                                    }

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

                                        x *= 65535;
                                        y *= 65535;

                                        int xx = (int)x;
                                        int yy = (int)y;

                                        uint flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
                                        //if (type == 1)
                                        //    flags |= DOWN;
                                        //else if (type == 2)
                                        //    flags |= UP;
                                        mouse_event(flags, xx, yy, 0, 0);

                                        //Thread.Sleep(TimeSpan.FromTicks(10));
                                    }
                                }
                                if (points.Count > MIN_POINTS * 2 || (points.Count > 0 && !stream.DataAvailable))
                                {
                                    List<float> points2 = new(points);

                                    Application.Current.Dispatcher.Invoke(() =>
                                    {
                                        using (DrawingContext drawingContext = drawingVisual.RenderOpen())
                                        {
                                            SolidColorBrush brush = Brushes.Red;
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
