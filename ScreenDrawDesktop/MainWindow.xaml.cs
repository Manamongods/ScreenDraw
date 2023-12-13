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

using LibUsbDotNet;
using LibUsbDotNet.LibUsb;
using LibUsbDotNet.Info;
using LibUsbDotNet.Descriptors;
using LibUsbDotNet.Main;

namespace ScreenDrawDesktop
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        static UsbDeviceFinder MyUsbFinder = new UsbDeviceFinder(0x18D1, 0x4EE2); // Replace with your device's vendor and product IDs
        //static UsbDeviceFinder MyUsbFinder = new UsbDeviceFinder(0x18D1, 0x4EE2, 0x0404); // Replace with your device's vendor and product IDs

        public MainWindow()
        {

            using (var writer = new StreamWriter("output.txt"))
            {
                Console.SetOut(writer);

                // Get a list of all connected USB devices
                UsbRegDeviceList devices = UsbDevice.AllDevices.FindAll(MyUsbFinder);
                //UsbRegDeviceList devices = UsbDevice.AllDevices.FindAll((e) => { return true; });

                UsbDevice usbDevice = null;

                foreach (UsbRegistry usbRegistry in devices)
                {
                    Console.WriteLine("Device Description: " + usbRegistry.FullName);

                    usbDevice = usbRegistry.Device;

                    // Get the USB device info to extract Vendor and Product IDs
                    //UsbDevice usbDevice2 = usbRegistry.Device;

                    // Get the Vendor and Product IDs (VID and PID)
                    //Console.WriteLine("ManufacturerString: " + usbDevice2?.Info?.ManufacturerString);
                    //Console.WriteLine("Product ID (PID): " + usbDevice2?.Info?.ProductString);
                }

                InitializeComponent();

                ErrorCode ec = ErrorCode.None;

                Console.WriteLine("Starting");

                // Find the USB device
                if(usbDevice == null)
                usbDevice = UsbDevice.OpenUsbDevice(MyUsbFinder);
                if (usbDevice == null)
                {
                    Console.WriteLine("Device not found.");
                    return;
                }

                // Open endpoint for communication
                UsbEndpointReader reader = usbDevice.OpenEndpointReader(ReadEndpointID.Ep01);
                byte[] readBuffer = new byte[1024];

                while (true)
                {
                    int bytesRead;
                    ec = reader.Read(readBuffer, 5000, out bytesRead);

                    if (ec != ErrorCode.None)
                    {
                        Console.WriteLine("Error reading from device: " + ec);
                        break;
                    }

                    if (bytesRead > 0)
                    {
                        string receivedData = Encoding.ASCII.GetString(readBuffer, 0, bytesRead);
                        Console.WriteLine("Received data: " + receivedData);
                        // Process received data here
                    }
                }

                usbDevice.Close();
            }
        }
    }
}
