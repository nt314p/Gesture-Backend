using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace Gesture
{
    // Adapted from https://stackoverflow.com/a/31612770
    public class AccurateTimer
    {
        private delegate void TimerEventDel(int id, int msg, IntPtr user, int dw1, int dw2);

        [DllImport("winmm.dll")]
        private static extern int timeBeginPeriod(int msec);

        [DllImport("winmm.dll")]
        private static extern int timeEndPeriod(int msec);

        [DllImport("winmm.dll")]
        private static extern int timeSetEvent(int delay, int resolution, TimerEventDel handler, IntPtr user,
            int eventType);

        [DllImport("winmm.dll")]
        private static extern int timeKillEvent(int id);

        private const int TIME_PERIODIC = 1;

        private readonly Action timerAction;
        private readonly int timerId;
        private readonly TimerEventDel handler; // NOTE: declare at class scope so garbage collector doesn't release it!!!

        public AccurateTimer(Action action, int delay)
        {
            timerAction = action;
            timeBeginPeriod(1);
            handler = TimerCallback;
            GC.KeepAlive(handler);
            timerId = timeSetEvent(delay, 0, handler, IntPtr.Zero, TIME_PERIODIC);
        }

        public void Stop()
        {
            var err = timeKillEvent(timerId);
            timeEndPeriod(1);
            Thread.Sleep(100); // Ensure callbacks are drained
        }

        private void TimerCallback(int id, int msg, IntPtr user, int dw1, int dw2)
        {
            if (timerId != 0)
            {
                if (timerAction == null)
                {
                    Console.WriteLine("timerAction is null!");
                }
                else
                {
                    timerAction.Invoke();
                }
            }
        }
    }
}