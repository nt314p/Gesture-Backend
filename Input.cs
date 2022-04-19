using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Gesture
{
    [StructLayout(LayoutKind.Sequential)]
    public struct INPUT
    {
        public uint type;
        public MOUSEINPUT mi;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-mouseinput
    [StructLayout(LayoutKind.Sequential)]
    public struct MOUSEINPUT
    {
        public int dx;
        public int dy;
        public int mouseData;
        public uint dwFlags;
        public uint time;
        public IntPtr dwExtraInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct CURSORINFO
    {
        public uint cbSize;
        public uint flags;
        public IntPtr hCursor;
        public POINT ptScreenPos;
    }

    public static class Input
    {
        [DllImport("user32.dll")]
        private static extern bool GetCursorPos(out POINT lpPoint);

        [DllImport("user32.dll")]
        private static extern bool GetCursorInfo(ref CURSORINFO pci);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern uint SendInput(uint cInputs, ref INPUT pInputs, int cbSize);
        
        [DllImport("user32.dll")]
        private static extern IntPtr GetDC(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);

        [DllImport("gdi32.dll")]
        private static extern int GetDeviceCaps(IntPtr hWnd, int index);

        private const uint MOUSEEVENTF_MOVE = 0x0001;
        private const uint MOUSEEVENTF_LEFTDOWN = 0x0002;
        private const uint MOUSEEVENTF_LEFTUP = 0x0004;
        private const uint MOUSEEVENTF_MIDDLEDOWN = 0x0020;
        private const uint MOUSEEVENTF_MIDDLEUP = 0x0040;
        private const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
        private const uint MOUSEEVENTF_RIGHTUP = 0x0010;
        private const uint MOUSEEVENTF_WHEEL = 0x0800;
        private const uint MOUSEEVENTF_ABSOLUTE = 0x8000;

        private const uint INPUT_MOUSE = 0;

        private const int HORZRES = 8;
        private const int VERTRES = 10;
        private const int DESKTOPVERTRES = 117;
        private const int DESKTOPHORZRES = 118;

        private static bool left;
        private static bool middle;
        private static bool right;
        private static bool touch;
        private static float mouseX;
        private static float mouseY;
        private static int screenWidth; // takes into account scaling
        private static int screenHeight;

        private static Stopwatch touchDragStopwatch;
        private const int TouchDragIntervalMs = 20;
        
        private static INPUT input;
        private static int inputStructSize;
        
        public static void Initialize()
        {
            touchDragStopwatch = Stopwatch.StartNew();
            
            GetCursorPos(out var point);
            mouseX = point.X;
            mouseY = point.Y;
            
            PointerInput.Initialize(point);

            var primary = GetDC(IntPtr.Zero); // get dc for entire screen
            screenWidth = GetDeviceCaps(primary, HORZRES) - 1;
            screenHeight = GetDeviceCaps(primary, VERTRES) - 1;

            ReleaseDC(IntPtr.Zero, primary);

            input = new INPUT
            {
                type = INPUT_MOUSE,
                mi =
                {
                    // mouse input struct
                    dx = 0,
                    dy = 0,
                    time = 0,
                    mouseData = 0,
                    dwExtraInfo = IntPtr.Zero
                }
            };

            inputStructSize = Marshal.SizeOf<INPUT>();
        }

        public static void PrintCursorStatus()
        {
            var cursorInfo = new CURSORINFO();
            cursorInfo.cbSize = (uint)Marshal.SizeOf<CURSORINFO>();
            if (!GetCursorInfo(ref cursorInfo))
            {
                Console.WriteLine("GetCursorInfo failed!");
            }
            else
            {
                Console.WriteLine($"Pos: ({cursorInfo.ptScreenPos.X}, {cursorInfo.ptScreenPos.Y})");
                Console.WriteLine($"Flags: {cursorInfo.flags}");
            }
        }
        
        public static void MovePointer(float x, float y)
        {
            //GetCursorPos(out POINT prevCursorPoint);
            //SetCursorPos(prevCursorPoint.X + x, prevCursorPoint.Y + y);
            mouseX += x;
            mouseY += y;
            mouseX = Math.Clamp(mouseX, 0, screenWidth);
            mouseY = Math.Clamp(mouseY, 0, screenHeight);

            if (x == 0 && y == 0)
            {
                GetCursorPos(out var point);
                mouseX = point.X;
                mouseY = point.Y;
                return;
            }

            var intMouseX = (int)Math.Round(mouseX * 1.25f);
            var intMouseY = (int)Math.Round(mouseY * 1.25f);

            if (!touch)
            {
                SetCursorPosition(mouseX, mouseY);
            }

            PointerInput.SetTouchPosition(intMouseX, intMouseY);

            var touchInput = true; // TODO: make global input mode variable
            
            if (!touch && touchInput) return;
            if (touchDragStopwatch.ElapsedMilliseconds < TouchDragIntervalMs) return;

            // drag only if we are currently touching and enough time has elapsed since the last drag
            PointerInput.TouchDrag(touch);
            touchDragStopwatch.Reset();
            touchDragStopwatch.Start();
        }

        public static void Touch(bool press)
        {
            if (touch == press) return;
            Console.WriteLine($"Touch {press}");
            PointerInput.Touch(press);
            touch = press;
        }

        public static void LeftButton(bool press)
        {
            if (left == press) return;
            MouseClick(press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
            left = press;
        }

        public static void MiddleButton(bool press)
        {
            if (middle == press) return;
            MouseClick(press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP);
            middle = press;
        }

        public static void RightButton(bool press)
        {
            if (right == press) return;
            MouseClick(press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
            right = press;
        }

        private static void SetCursorPosition(float x, float y)
        {
            input.mi.dx = (int)(x * ushort.MaxValue / screenWidth);
            input.mi.dy = (int)(y * ushort.MaxValue / screenHeight);
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            input.mi.mouseData = 0;

            if (SendInputMouse() == 0)
            {
                Console.WriteLine($"SendInput (move) failed! Error: {Marshal.GetLastWin32Error()}");
            }
        }

        public static void Scroll(int scrollAmount)
        {
            input.mi.dx = 0;
            input.mi.dy = 0;
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = scrollAmount;

            if (SendInputMouse() == 0)
            {
                Console.WriteLine($"SendInput (scroll) failed! Error: {Marshal.GetLastWin32Error()}");
            }
        }

        private static void MouseClick(uint flags)
        {
            input.mi.dx = 0;
            input.mi.dy = 0;
            input.mi.dwFlags = flags;
            input.mi.mouseData = 0;

            if (SendInputMouse() == 0)
            {
                Console.WriteLine($"SendInput (click) failed! Error: {Marshal.GetLastWin32Error()}");
            }
        }

        private static uint SendInputMouse()
        {
            return SendInput(1, ref input, inputStructSize);
        }
    }
}