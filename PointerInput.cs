using System;
using System.Runtime.InteropServices;

namespace Gesture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT
    {
        public int X;
        public int Y;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/windef/ns-windef-rect
    [StructLayout(LayoutKind.Explicit)]
    public struct RECT
    {
        [FieldOffset(0)] public long left;
        [FieldOffset(4)] public long top;
        [FieldOffset(8)] public long right;
        [FieldOffset(12)] public long bottom;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-pointer_touch_info
    [StructLayout(LayoutKind.Sequential)]
    public struct POINTER_TOUCH_INFO
    {
        public POINTER_INFO pointerInfo;
        public TOUCH_FLAGS touchFlags;
        public TOUCH_MASK touchMask;
        public RECT rcContact;
        public RECT rcContactRaw;
        public uint orientation;
        public uint pressure;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-pointer_info
    [StructLayout(LayoutKind.Sequential)]
    public struct POINTER_INFO
    {
        public POINTER_INPUT_TYPE pointerType;
        public uint pointerId;
        public uint frameId;
        public POINTER_FLAGS pointerFlags;
        public IntPtr sourceDevice;
        public IntPtr hwndTarget;
        public POINT ptPixelLocation;
        public POINT ptHimetricLocation;
        public POINT ptPixelLocationRaw;
        public POINT ptHimetricLocationRaw;
        public uint dwTime;
        public uint historyCount;
        public int InputData;
        public uint dwKeyStates;
        public ulong PerformanceCount;
        public POINTER_BUTTON_CHANGE_TYPE ButtonChangeType;
    }
    
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-pointer_type_info
    [StructLayout(LayoutKind.Sequential)]
    struct POINTER_TYPE_INFO {
        public POINTER_INPUT_TYPE type; 
        public POINTER_PEN_INFO penInfo;
    }
    
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-pointer_pen_info
    [StructLayout(LayoutKind.Sequential)]
    struct POINTER_PEN_INFO
    {
        public POINTER_INFO pointerInfo;
        public PEN_FLAGS penFlags;
        public PEN_MASK penMask;
        public uint pressure;
        public uint rotation;
        public int tiltX;
        public int tiltY;
    }
    
    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/inputmsg/pen-flags-constants
    [Flags]
    public enum PEN_FLAGS
    {
        NONE = 0,
        BARREL = 1,
        INVERTED = 2,
        ERASER = 4
    }
    
    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/inputmsg/pen-mask-constants
    [Flags]
    public enum PEN_MASK
    {
        NONE = 0,
        PRESSURE = 1,
        ROTATION = 2,
        TILT_X = 4,
        TILT_Y = 8
    }

    public enum POINTER_INPUT_TYPE
    {
        POINTER = 1,
        TOUCH = 2,
        PEN = 3,
        MOUSE = 4,
        TOUCHPAD = 5
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ne-winuser-pointer_feedback_mode
    public enum POINTER_FEEDBACK_MODE
    {
        DEFAULT,
        INDIRECT,
        NONE
    }

    public enum POINTER_BUTTON_CHANGE_TYPE
    {
        NONE,
        FIRSTBUTTON_DOWN,
        FIRSTBUTTON_UP,
        SECONDBUTTON_DOWN,
        SECONDBUTTON_UP,
        THIRDBUTTON_DOWN,
        THIRDBUTTON_UP,
        FOURTHBUTTON_DOWN,
        FOURTHBUTTON_UP,
        FIFTHBUTTON_DOWN,
        FIFTHBUTTON_UP
    }

    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/inputmsg/pointer-flags-contants
    [Flags]
    public enum POINTER_FLAGS : uint
    {
        NONE = 0x00000000,
        NEW = 0x00000001,
        INRANGE = 0x00000002,
        INCONTACT = 0x00000004,
        FIRSTBUTTON = 0x00000010,
        SECONDBUTTON = 0x00000020,
        THIRDBUTTON = 0x00000040,
        FOURTHBUTTON = 0x00000080,
        FIFTHBUTTON = 0x00000100,
        PRIMARY = 0x00002000,
        CONFIDENCE = 0x000004000,
        CANCELED = 0x000008000,
        DOWN = 0x00010000,
        UPDATE = 0x00020000,
        UP = 0x00040000,
        WHEEL = 0x00080000,
        HWHEEL = 0x00100000,
        CAPTURECHANGED = 0x00200000,
        HASTRANSFORM = 0x0040000
    }

    public enum TOUCH_FLAGS
    {
        NONE
    }

    [Flags]
    public enum TOUCH_MASK : uint
    {
        NONE = 0x00000000,
        CONTACTAREA = 0x00000001,
        ORIENTATION = 0x00000002,
        PRESSURE = 0x00000004
    }
    
    public static class PointerInput
    {
        [DllImport("user32.dll")]
        private static extern bool InitializeTouchInjection(uint maxCount, uint dwMode);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool InjectTouchInput(uint count, ref POINTER_TOUCH_INFO contacts);
        
        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr CreateSyntheticPointerDevice(POINTER_INPUT_TYPE pointerType, ulong maxCount, POINTER_FEEDBACK_MODE mode);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool InjectSyntheticPointerInput(IntPtr device, ref POINTER_TYPE_INFO pointerInfo, int count);
        
        private const POINTER_FLAGS PointerFlagTouchDown = POINTER_FLAGS.INRANGE | POINTER_FLAGS.INCONTACT | POINTER_FLAGS.DOWN;
        private const POINTER_FLAGS PointerFlagTouchDrag = POINTER_FLAGS.INRANGE | POINTER_FLAGS.INCONTACT | POINTER_FLAGS.UPDATE;
        private const POINTER_FLAGS PointerFlagHover = POINTER_FLAGS.INRANGE | POINTER_FLAGS.UPDATE;
        private const POINTER_FLAGS PointerFlagTouchUp = POINTER_FLAGS.INRANGE | POINTER_FLAGS.UP;
        
        // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/input_touchinjection/constants
        private const uint TOUCH_FEEDBACK_DEFAULT = 0x1;
        private const uint TOUCH_FEEDBACK_INDIRECT = 0x2;
        private const uint TOUCH_FEEDBACK_NONE = 0x3;
        
        private static IntPtr penDevice;
        private static POINTER_TOUCH_INFO touchInfo;
        private static POINTER_TYPE_INFO penInfo;
        private static POINTER_INFO pointerInfo;

        public static void Initialize(POINT point)
        {
            pointerInfo = new POINTER_INFO
            {
                pointerType = POINTER_INPUT_TYPE.TOUCH,
                pointerId = 0,
                frameId = 0,
                pointerFlags = PointerFlagTouchDown,
                ptPixelLocation = point
            };
            
            penDevice = CreateSyntheticPointerDevice(POINTER_INPUT_TYPE.PEN, 1, POINTER_FEEDBACK_MODE.INDIRECT);
            
            var pointerPenInfo = new POINTER_PEN_INFO
            {
                pointerInfo = pointerInfo,
                penFlags = PEN_FLAGS.NONE,
                penMask = PEN_MASK.PRESSURE,
                pressure = 500
            };

            penInfo = new POINTER_TYPE_INFO
            {
                type = POINTER_INPUT_TYPE.PEN,
                penInfo = pointerPenInfo
            };
            
            touchInfo = new POINTER_TOUCH_INFO
            {
                pointerInfo = pointerInfo,
                touchFlags = TOUCH_FLAGS.NONE,
                touchMask = TOUCH_MASK.CONTACTAREA | TOUCH_MASK.PRESSURE | TOUCH_MASK.ORIENTATION,
                pressure = 500,
                orientation = 90,
                rcContact = new RECT(),
            };
        }

        public static void SetTouchPosition(int x, int y)
        {
            pointerInfo.ptPixelLocation.X = x;
            pointerInfo.ptPixelLocation.Y = y;
        }

        public static void Touch(bool press)
        {
            if (press)
            {
                var initResult = InitializeTouchInjection(1, TOUCH_FEEDBACK_DEFAULT);
                if (!initResult) Console.WriteLine("InitializeTouchInjection failed!");

                InjectPointerInputLocal(POINTER_INPUT_TYPE.PEN, PointerFlagTouchDown);
            }
            else
            {
                // ensure that last update injection has same coords as the up injection
                InjectPointerInputLocal(POINTER_INPUT_TYPE.PEN, PointerFlagTouchDrag); 
                InjectPointerInputLocal(POINTER_INPUT_TYPE.PEN, PointerFlagTouchUp);
            }
        }
        
        public static void TouchDrag(bool contact)
        {
            InjectPointerInputLocal(POINTER_INPUT_TYPE.PEN, contact ? PointerFlagTouchDrag : PointerFlagHover);
        }

        private static void InjectPointerInputLocal(POINTER_INPUT_TYPE pointerInputType, POINTER_FLAGS pointerFlags)
        {
            pointerInfo.pointerType = pointerInputType;
            pointerInfo.pointerFlags = pointerFlags;
            int error;

            if (pointerInputType == POINTER_INPUT_TYPE.PEN)
            {
                penInfo.penInfo.pointerInfo = pointerInfo;
                if (pointerFlags == PointerFlagTouchDrag) Console.WriteLine("dragging!");
                if (InjectSyntheticPointerInput(penDevice, ref penInfo, 1)) return;
                error = Marshal.GetLastWin32Error();
            } else if (pointerInputType == POINTER_INPUT_TYPE.TOUCH)
            {
                touchInfo.pointerInfo = pointerInfo;
                if (InjectTouchInput(1, ref touchInfo)) return;
                error = Marshal.GetLastWin32Error();
            }
            else
            {
                throw new NotImplementedException($"{pointerInputType.ToString()} input type is not supported");
            }

            var methodName = pointerInputType == POINTER_INPUT_TYPE.PEN
                ? "InjectSyntheticPointerInput"
                : "InjectTouchInput";
            
            Console.Write($"{methodName} failed! Error: {error}");
            switch (pointerFlags)
            {
                case PointerFlagTouchDown:
                    Console.WriteLine("; failed while touching down");
                    break;
                case PointerFlagTouchDrag:
                    Console.WriteLine("; failed while dragging");
                    break;
                case PointerFlagTouchUp:
                    Console.WriteLine("; failed while touching up");
                    break;
                default:
                    Console.WriteLine($"; unknown pointer flag value: {pointerFlags}");
                    break;
            }
        }
    }
}