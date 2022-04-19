using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.Advertisement;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Devices.Enumeration;

namespace Gesture
{
   public class BluetoothSerial
   {
      public Action PacketReceived;
      public Action Connected;
      public Action Disconnected;

      public int BytesToRead => buffer.Count;

      public bool IsConnected
      {
         get
         {
            if (bleDevice == null) return false;
            return bleDevice.ConnectionStatus == BluetoothConnectionStatus.Connected;
         }
      }

      private BluetoothLEDevice bleDevice;
      private readonly Guid serviceGuid;
      private readonly Guid characteristicGuid;
      private readonly string targetName;
      private readonly int maxConnectionRetries;
      private GattDeviceService customService;
      private GattCharacteristic customCharacteristic;

      private Stopwatch tick;
      private readonly BluetoothLEAdvertisementWatcher watcher;
      private readonly BlockingCollection<byte> buffer;
      private readonly Dictionary<ulong, bool> deviceAdvertisementDict;

      public BluetoothSerial(string targetName, string serviceGuid, string characteristicGuid, int maxConnectionRetries)
      {
         this.targetName = targetName;
         this.serviceGuid = Guid.Parse(serviceGuid);
         this.characteristicGuid = Guid.Parse(characteristicGuid);
         this.maxConnectionRetries = maxConnectionRetries;

         buffer = new BlockingCollection<byte>(new ConcurrentQueue<byte>());
         deviceAdvertisementDict = new Dictionary<ulong, bool>();
         tick = Stopwatch.StartNew();

         watcher = new BluetoothLEAdvertisementWatcher
         {
            ScanningMode = BluetoothLEScanningMode.Active,
            SignalStrengthFilter =
            {
               InRangeThresholdInDBm = -80,
               OutOfRangeThresholdInDBm = -90,
               OutOfRangeTimeout = TimeSpan.FromMilliseconds(5000),
               SamplingInterval = TimeSpan.FromMilliseconds(1000)
            }
         };
         
         watcher.Received += OnAdvertisementReceived;
      }

      public void AttemptConnection()
      {
         watcher.Stop();
         deviceAdvertisementDict.Clear();
         watcher.Start();
         Console.WriteLine("Watching for BLE device advertisements...");
      }

      public void Disconnect()
      {
         bleDevice.ConnectionStatusChanged -= OnConnectionStatusChanged;
         customService?.Dispose();
         bleDevice?.Dispose();
      }

      public byte ReadByte()
      {
         return buffer.Take();
      }

      public byte[] ReadBytes(int count)
      {
         if (count > buffer.Count) throw new InvalidOperationException("Not enough bytes left to read");
         var bytes = new byte[count];
         for (var i = 0; i < count; i++)
         {
            bytes[i] = buffer.Take();
         }

         return bytes;
      }
      
      private async void OnAdvertisementReceived(BluetoothLEAdvertisementWatcher sender, BluetoothLEAdvertisementReceivedEventArgs args)
      {
         var hasName = args.Advertisement.LocalName != string.Empty;
         var name = hasName ? args.Advertisement.LocalName : "(unnamed device)";
         var printNewAdvertisement = true;
         
         if (deviceAdvertisementDict.TryGetValue(args.BluetoothAddress, out var previouslyHadName))
         {
            printNewAdvertisement = !previouslyHadName && hasName; // only print new advertisement if name was received
            deviceAdvertisementDict[args.BluetoothAddress] = previouslyHadName || hasName;
         }
         else
         {
            deviceAdvertisementDict.Add(args.BluetoothAddress, hasName);
         }
         
         var bytes = BitConverter.GetBytes(args.BluetoothAddress).Reverse().ToArray();
         var address = Convert.ToHexString(bytes);
         
         if (printNewAdvertisement) Console.WriteLine($"Received advertisement: {address} - {name}");
         
         if (args.Advertisement.LocalName != targetName) return;
         Console.WriteLine($"\nFound device '{targetName}'! Establishing connection...");
         sender.Stop();
         
         bleDevice = await BluetoothLEDevice.FromBluetoothAddressAsync(args.BluetoothAddress);

         var connectionTries = 0;
         while (true)
         {
            var initializeResult = await Initialize(bleDevice);
            if (initializeResult) break;
            Console.WriteLine("Failed to initialize device, retrying...");
            connectionTries++;
            if (connectionTries != maxConnectionRetries) continue;
            Console.WriteLine($"Reached maximum tries ({maxConnectionRetries}) to establish connection, exiting...");
            Environment.Exit(0);
         }
         
         Connected?.Invoke();
         
         bleDevice.ConnectionStatusChanged += OnConnectionStatusChanged;
         Console.WriteLine("Listening for connection changes");

         /*device.DeviceInformation.Pairing.Custom.PairingRequested += PairRequested;
         
         var pairingResult = await device.DeviceInformation.Pairing.Custom.PairAsync(DevicePairingKinds.ProvidePin,
            DevicePairingProtectionLevel.None);
         Console.WriteLine("Pair status: {0}", pairingResult.Status);
         
         device.DeviceInformation.Pairing.Custom.PairingRequested -= PairRequested;*/
      }

      private void OnConnectionStatusChanged(BluetoothLEDevice sender, object args)
      {
         if (sender.ConnectionStatus == BluetoothConnectionStatus.Disconnected)
         {
            Disconnected?.Invoke();
         }
      }

      private async Task<bool> Initialize(BluetoothLEDevice device)
      {
         var servicesResult = await device.GetGattServicesForUuidAsync(serviceGuid, BluetoothCacheMode.Uncached);

         if (servicesResult.Status != GattCommunicationStatus.Success)
         {
            Console.WriteLine("Unable to fetch service");
            return false;
         }
         
         Console.WriteLine("Service status: {0}", servicesResult.Status); // Success with uncached mode, not paired
         customService = servicesResult.Services[0];
         
         var characteristicsResult =
            await customService.GetCharacteristicsForUuidAsync(characteristicGuid, BluetoothCacheMode.Uncached);
            
         Console.WriteLine("Characteristic status: {0}", characteristicsResult.Status); // Success with uncached mode, not paired

         if (characteristicsResult.Status == GattCommunicationStatus.AccessDenied) return false;
         
         var characteristics = characteristicsResult.Characteristics.ToArray();
         customCharacteristic = characteristics[0];

         var descriptorResult = await customCharacteristic.GetDescriptorsAsync(BluetoothCacheMode.Uncached);
         Console.WriteLine("Descriptor status: {0}", descriptorResult.Status); // Success with uncached mode, not paired
         
         var writeConfig =
            await customCharacteristic.WriteClientCharacteristicConfigurationDescriptorWithResultAsync(
               GattClientCharacteristicConfigurationDescriptorValue.Notify);
         
         Console.WriteLine("Write config status: {0}", writeConfig.Status); // Success, not paired!

         var characteristicConfig =
            await customCharacteristic.ReadClientCharacteristicConfigurationDescriptorAsync();
         Console.WriteLine("Characteristic config status: {0}", characteristicConfig.Status);

         if (characteristicConfig.ClientCharacteristicConfigurationDescriptor ==
             GattClientCharacteristicConfigurationDescriptorValue.Notify)
         {
            Console.WriteLine("Notifications enabled!");
         }
         
         customCharacteristic.ValueChanged += CharacteristicOnValueChanged;
      
         return true;
      }

      private void CharacteristicOnValueChanged(GattCharacteristic sender, GattValueChangedEventArgs args)
      {
         // Console.WriteLine($"{DateTime.Now}\tCharacteristic value changed!");
         var byteBuffer = args.CharacteristicValue;

         for (uint index = 0; index < byteBuffer.Length; index++)
         {
            buffer.Add(byteBuffer.GetByte(index));
         }

         var nanosecondsPerTick = 1000000000L / Stopwatch.Frequency;
         //Console.WriteLine($"Tick delta: {tick.ElapsedTicks * nanosecondsPerTick / 1000000f} ms\t {args.Timestamp.Millisecond}");
         //Console.WriteLine(Convert.ToHexString(bytes));

         tick = Stopwatch.StartNew();
         PacketReceived?.Invoke();
      }

      private static void PairRequested(DeviceInformationCustomPairing sender, DevicePairingRequestedEventArgs args)
      {
         Console.WriteLine("Received pairing request!");
         args.Accept("605011");
      }
   } 
}