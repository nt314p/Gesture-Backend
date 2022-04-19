using System;
using System.Diagnostics;
using System.Threading;

namespace Gesture
{
    internal static class Program
    {
        private static void Main()
        {
            //AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
            try
            {
                var runner = new Runner();
            }
            catch (Exception exception)
            {
                Console.WriteLine(exception.Message);
                Console.WriteLine(exception.StackTrace);
                Console.WriteLine(exception.ToString());
            }
        }

        private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            var exception = (Exception)e.ExceptionObject;
            Console.WriteLine(exception.Message);
            Console.WriteLine(exception.ToString());
            Console.WriteLine(exception.StackTrace);
            Console.WriteLine(Environment.StackTrace);
        }
    }

    public enum MiddleAction
    {
        None, // middle mouse is not being pressed
        Undetermined, // middle action has not yet been determined
        Scroll,
        Drag,
    }

    internal class Runner
    {
        private Stopwatch watch;
        private Stopwatch lastDataStopwatch;
        private Stopwatch tick;
        private Stopwatch inputTimeDeltaStopwatch;
        private AccurateTimer mainTimer;

        private readonly double nanosecondsPerTick;

        private int packetCount = 1;

        private readonly RemoteDevice gestureDevice;
        private readonly BluetoothSerial bleSerial;
        private Vector3 previousMouseDelta;

        private const string ServiceGuid = "0000ffe0-0000-1000-8000-00805f9b34fb";
        private const string CharacteristicGuid = "0000ffe1-0000-1000-8000-00805f9b34fb";
        private const int DataTimeoutThresholdMs = 400;
        private const int PacketIntervalMs = 10;

        private bool shouldLoop;
        private bool previousMiddlePressed;
        private MiddleAction middleAction;

        public Runner()
        {
            gestureDevice = new RemoteDevice(15, 300, 500);
            previousMouseDelta = new Vector3();
            middleAction = MiddleAction.None;

            bleSerial = new BluetoothSerial("Gesture", ServiceGuid, CharacteristicGuid, 3);
            bleSerial.Connected += OnBLEConnected;
            bleSerial.PacketReceived += OnPacketReceived;
            gestureDevice.DataAligned += OnDataAligned;
            gestureDevice.DataMisaligned += OnDataMisaligned;
            bleSerial.Disconnected += OnBLEDisconnected;

            bleSerial.AttemptConnection();

            nanosecondsPerTick = 1000000000L / Stopwatch.Frequency;

            watch = Stopwatch.StartNew();
            inputTimeDeltaStopwatch = new Stopwatch();
            lastDataStopwatch = new Stopwatch();

            while (true)
            {
                Thread.Sleep(100);
                if (lastDataStopwatch.ElapsedMilliseconds > DataTimeoutThresholdMs)
                {
                    Console.WriteLine($"Have not received data for {lastDataStopwatch.ElapsedMilliseconds} ms...");
                    shouldLoop = false;
                }
                else if (bleSerial.IsConnected && gestureDevice.IsDataAligned)
                {
                    shouldLoop = true;
                }
            }
        }

        private void OnBLEConnected()
        {
            Console.WriteLine("Bluetooth device connected!");
            shouldLoop = true;
            lastDataStopwatch = Stopwatch.StartNew();
            inputTimeDeltaStopwatch = Stopwatch.StartNew();
        }

        private void OnBLEDisconnected()
        {
            Console.WriteLine("Gesture remote was disconnected, attempting to reconnect...");
            shouldLoop = false;
            lastDataStopwatch.Reset();
            lastDataStopwatch.Stop();
            gestureDevice.IsDataAligned = false;
            bleSerial.AttemptConnection();
        }

        private void OnDataAligned()
        {
            Console.WriteLine("Aligned data!");
            Input.Initialize();

            shouldLoop = true;
            mainTimer = new AccurateTimer(Loop, 1);
        }

        private void OnDataMisaligned()
        {
            Console.WriteLine("Data is misaligned! Attempting to realign...");
            shouldLoop = false;
        }

        private void Loop()
        {
            if (!shouldLoop) return;

            //Console.WriteLine($"Tick delta: {tick.ElapsedTicks * nanosecondsPerTick / 1000000f} ms");
            //tick = Stopwatch.StartNew();
            ReadDataAligned();
        }
        
        private void OnPacketReceived()
        {
            lastDataStopwatch.Reset();
            lastDataStopwatch.Start();
            if (gestureDevice.IsDataAligned) return;

            while (bleSerial.BytesToRead > 0)
            {
                if (gestureDevice.AlignData(bleSerial.ReadByte())) break;
            }
        }

        private void ReadDataAligned()
        {
            var bytes = bleSerial.BytesToRead;

            if (bytes >= gestureDevice.PacketSize)
            {
                if (!gestureDevice.TryProcessPacket(bleSerial.ReadBytes(gestureDevice.PacketSize)))
                {
                    return;
                }

                if (tick == null) tick = Stopwatch.StartNew();
                //Console.WriteLine($"ms per packet: {tick.ElapsedTicks * nanosecondsPerTick / 1000000f / packetCount}\t{buffer.Count}");
                //packetCount++;

                ProcessMouse();
                ProcessButtons();
                ProcessScroll();
            }

            var elapsedMs = (float)(inputTimeDeltaStopwatch.ElapsedTicks * nanosecondsPerTick / 1000000L);

            // distance = velocity [distance/tick] * ms / (ms/tick) [tick]
            var predictedMouseDelta = previousMouseDelta * (elapsedMs / PacketIntervalMs);

            // TODO: is scrolling laggy because its not being lerped between packet receives or because the scroll dead zone is too large? 

            // prevent mouse move when scrolling
            if (middleAction != MiddleAction.Scroll) Input.MovePointer(predictedMouseDelta.X, predictedMouseDelta.Y);

            inputTimeDeltaStopwatch = Stopwatch.StartNew();
        }

        private void ProcessMouse()
        {
            const float sensitivity = 10f;
            const float powerFactor = 1.4f;
            const float scrollPowerFactor = 1.5f;
            const float deadZone = 0.3f;
            const float scrollDeadZone = 3f;

            var x = gestureDevice.Gyro.Z;
            var y = gestureDevice.Gyro.X;
            var scroll = gestureDevice.Gyro.Y;

            if (Math.Abs(x) < deadZone) x = 0;
            if (Math.Abs(y) < deadZone) y = 0;
            if (Math.Abs(scroll) < scrollDeadZone) scroll = 0;

            var cookedX = (float)-(Math.Sign(x) * Math.Pow(Math.Abs(x), powerFactor) / sensitivity);
            var cookedY = (float)(Math.Sign(y) * Math.Pow(Math.Abs(y), powerFactor) / sensitivity);
            var cookedScroll = (float)(Math.Sign(scroll) * Math.Pow(Math.Abs(scroll), scrollPowerFactor) / 15f);

            //Console.WriteLine($"{x}\t{y}");
            //Console.WriteLine($"{cookedX}\t{cookedY}");
            //Console.WriteLine(dataPacket.Gyro.Y);

            if (middleAction == MiddleAction.Undetermined)
            {
                const int scrollTolerance = 25;
                const int dragTolerance = 20;

                if (Math.Abs(scroll) > scrollTolerance)
                {
                    middleAction = MiddleAction.Scroll;
                }

                if (Math.Abs(x) > dragTolerance || Math.Abs(y) > dragTolerance)
                {
                    middleAction = MiddleAction.Drag;
                }
            }

            if (middleAction == MiddleAction.Scroll) // TODO: refactor this into scroll processing
            {
                Input.Scroll((int)MathF.Round(cookedScroll));
            }

            previousMouseDelta = new Vector3(cookedX, cookedY, 0);
        }

        private void ProcessButtons()
        {
            Input.RightButton(gestureDevice.RightPressed);
            Input.MiddleButton(gestureDevice.MiddlePressed);
            Input.LeftButton(gestureDevice.LeftPressed);
            //Input.Touch(gestureDevice.LeftPressed);
        }

        private void ProcessScroll()
        {
            if (gestureDevice.MiddlePressed && !previousMiddlePressed)
            {
                //Console.WriteLine(dataPacket.Gyro.Y);f (middlePressed && !previousMiddlePressed)
                //Console.WriteLine(dataPacket.Gyro.Z);
                //Console.WriteLine(dataPacket.Gyro.X);
                previousMiddlePressed = true;
                middleAction = MiddleAction.Undetermined;
            }
            else if (!gestureDevice.MiddlePressed)
            {
                previousMiddlePressed = false;
                middleAction = MiddleAction.None;
            }
        }
    }
}