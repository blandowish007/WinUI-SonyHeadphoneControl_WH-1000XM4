using System.Threading;

namespace SonyHeadphonesClient.API
{
    /// <summary>
    /// 全局电量状态：单一数据源。查询逻辑集中在此，UI（SettingPage 电量行）和
    /// 托盘（右键菜单）都读同一份数据，避免两个循环各自采样导致显示不一致。
    /// </summary>
    internal static class BatteryState
    {
        // Current: 最新一次查询结果。-2=未连接, -1=已连接但查询失败, 0-100=电量
        // LastValid: 上一条有效电量（0-100），-1=从未查到过有效值
        private static int _current = -2;
        private static int _lastValid = -1;
        // 当前环境声音控制模式（"降噪"/"环境声音"/"语音"/"关闭"），空=未设置
        private static string _mode = "";
        private static readonly object _lock = new object();

        public static int Current
        {
            get { lock (_lock) return _current; }
            private set { lock (_lock) _current = value; }
        }

        public static int LastValid
        {
            get { lock (_lock) return _lastValid; }
        }

        /// <summary>设置当前模式（由 SettingPage 切换模式时调用）</summary>
        public static void SetMode(string mode)
        {
            lock (_lock) _mode = mode ?? "";
        }

        /// <summary>
        /// 根据当前查询结果格式化托盘显示文本（tooltip 共用）。
        /// 查询瞬断（cur=-2/-1）时保留上一条有效电量，避免闪烁；真断开由 MarkDisconnected 清空 lastValid。
        /// </summary>
        public static string FormatDisplay()
        {
            int cur;
            int valid;
            string mode;
            lock (_lock) { cur = _current; valid = _lastValid; mode = _mode; }
            string first;
            if (cur >= 0 && cur <= 100)
                first = $"WH-1000XM4 · {cur}%";
            else if (valid >= 0)
                first = $"WH-1000XM4 · {valid}%";
            else if (cur == -1)
                first = "WH-1000XM4 · 已连接（电量未知）";
            else
                first = "WH-1000XM4 · 未连接";
            // 已连接且有模式时追加模式行
            bool connected = (cur >= 0 && cur <= 100) || valid >= 0 || cur == -1;
            if (connected && !string.IsNullOrEmpty(mode))
                return first + "\n模式：" + mode;
            return first;
        }

        /// <summary>
        /// 根据当前查询结果格式化控制界面电量行文本。
        /// </summary>
        public static string FormatSettingLine()
        {
            int cur;
            int valid;
            lock (_lock) { cur = _current; valid = _lastValid; }
            if (cur >= 0 && cur <= 100)
                return $"电量：{cur}%";
            if (valid >= 0)
                return $"电量：{valid}%";
            if (cur == -1)
                return "电量：未知";
            return "电量：未连接";
        }

        /// <summary>
        /// 断开连接时调用：清空状态，UI 自动显示「未连接」。
        /// </summary>
        public static void MarkDisconnected()
        {
            lock (_lock) { _current = -2; _lastValid = -1; _mode = ""; }
        }


        /// <summary>
        /// 执行一次电量查询（含重试），更新 Current/LastValid 并返回 Current。
        /// 上游 SetChanges 每秒 refresh() 断开重连 socket，单次查询可能踩在断开窗口，
        /// 故按 retries 重试（间隔 500ms）踩中连上窗口。
        /// </summary>
        public static int Query(int retries)
        {
            int result = -2;
            for (int attempt = 0; attempt < retries; attempt++)
            {
                try
                {
                    if (Headphone.IsConnected())
                    {
                        int v = Headphone.GetBatteryLevel();
                        if (v >= 0 && v <= 100)
                        {
                            result = v;
                            break;
                        }
                        // 查询返回 -1（已连接但失败），标记为已连接查询失败
                        result = -1;
                    }
                    else
                    {
                        result = -2;
                    }
                }
                catch
                {
                    result = -1;
                }
                if (attempt < retries - 1)
                    Thread.Sleep(500);
            }

            Current = result;
            if (result >= 0 && result <= 100)
            {
                lock (_lock) _lastValid = result;
            }
            return result;
        }
    }
}
