using System;
using System.Runtime.InteropServices;

namespace Gesture
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Vector3Short
    {
        private short X;
        private short Y;
        private short Z;

        public Vector3 ToVector3(float range)
        {
            return new Vector3
            {
                X = range * X / short.MaxValue,
                Y = range * Y / short.MaxValue,
                Z = range * Z / short.MaxValue
            };
        }
    }
    
    public struct Vector3
    {
        public float X;
        public float Y;
        public float Z;

        public Vector3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public static Vector3 operator +(Vector3 a) => a;
        public static Vector3 operator -(Vector3 a) => new(-a.X, -a.Y, -a.Z);
        public static Vector3 operator +(Vector3 a, Vector3 b) => new(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        public static Vector3 operator -(Vector3 a, Vector3 b) => a + -b;
        public static Vector3 operator *(Vector3 a, float k) => new(a.X * k, a.Y * k, a.Z * k);
        public static Vector3 operator *(float k, Vector3 a) => a * k;
        public static Vector3 operator /(Vector3 a, float k) => new(a.X / k, a.Y / k, a.Z / k);
        public static Vector3 operator /(float k, Vector3 a) => a / k;

        public override string ToString()
        {
            return $"{X}\t{Y}\t{Z}";
        }
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Packet
    {
        //public Vector3 Acceleration;
        public Vector3Short Gyro;
        public sbyte ScrollData;
        public byte ButtonData;
    }
    
    public class RemoteDevice
    {
        private const byte Signature = 0b10101000;
        
        private readonly int alignmentAttemptsThreshold; // the maximum attempts at aligning packets
        private readonly int degreesPerSecondRange;
        private readonly int sequentialValidPacketsToAlign; // number of valid sequential packets before alignment is accepted

        public readonly int PacketSize;
        private readonly IntPtr packetPtr;

        private int byteIndex;
        private int triedPackets;
        private readonly byte[] byteValidCount; // byteValidCount[i] holds how many consecutive times the ith byte in the packet has been valid

        public bool IsDataAligned;
        public Action DataAligned;
        public Action DataMisaligned;
        
        public Vector3 Gyro { get; private set; }
        public bool LeftPressed { get; private set; }
        public bool MiddlePressed { get; private set; }
        public bool RightPressed { get; private set; }

        public RemoteDevice(int sequentialValidPacketsToAlign, int alignmentAttemptsThreshold, int degreesPerSecondRange)
        {
            this.sequentialValidPacketsToAlign = sequentialValidPacketsToAlign;
            this.alignmentAttemptsThreshold = alignmentAttemptsThreshold;
            this.degreesPerSecondRange = degreesPerSecondRange;
            PacketSize = Marshal.SizeOf<Packet>();
            
            packetPtr = Marshal.AllocHGlobal(PacketSize);
            byteValidCount = new byte[PacketSize];
        }

        ~RemoteDevice() => Marshal.FreeHGlobal(packetPtr);
        
        public bool AlignData(byte b)
        {
            if (triedPackets > alignmentAttemptsThreshold)
            {
                Console.WriteLine($"Tried to align {alignmentAttemptsThreshold} packets with no success");
                Console.WriteLine("Exiting");
                Environment.Exit(0);
            }
            
            if ((b & 0b11111000) == Signature) // check upper 5 bits for signature
            {
                byteValidCount[byteIndex]++;
                if (byteValidCount[byteIndex] >= sequentialValidPacketsToAlign)
                {
                    IsDataAligned = true;
                    DataAligned?.Invoke();
                    return true;
                }
            }
            else
            {
                byteValidCount[byteIndex] = 0;
            }

            byteIndex++;

            if (byteIndex >= byteValidCount.Length)
            {
                byteIndex = 0;
                triedPackets++;
            }
            return false;
        }
        
        public bool TryProcessPacket(byte[] bytes)
        {
            if (!IsDataAligned) throw new InvalidOperationException("Data is not aligned!");
            if (bytes.Length != PacketSize) throw new ArgumentException("Byte array argument is incorrectly sized!");
           
            Marshal.Copy(bytes, 0, packetPtr, PacketSize);
            var rawPacket = Marshal.PtrToStructure<Packet>(packetPtr);

            Gyro = rawPacket.Gyro.ToVector3(degreesPerSecondRange);
            
            var buttonData = rawPacket.ButtonData;
            RightPressed = buttonData % 2 == 1;
            LeftPressed = (buttonData >> 1) % 2 == 1;
            MiddlePressed = (buttonData >> 2) % 2 == 1;

            if ((buttonData & 0b11111000) == Signature) return true;

            IsDataAligned = false;
            triedPackets = 0;
            DataMisaligned?.Invoke();
            return false;
        }
    }
}