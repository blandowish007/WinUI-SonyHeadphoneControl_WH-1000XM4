using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Imaging;
using SonyHeadphonesClient.API;
using SonyHeadphonesClient.UICommand;
using System;
using System.Resources;
using System.Security.Principal;
using System.Windows.Input;
using Windows.UI.ViewManagement;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace SonyHeadphonesClient.Control
{
    public sealed partial class TaskbarIcon : UserControl
    {
        public ICommand LeftClickCommand_ = new LeftClickCommand();
        public ICommand ExitCommand_ = new ExitCommand();
        public ICommand AutoRuntCommand_;

        private ToggleMenuFlyoutItem AutoStartItem;

        // UI 刷新定时器：2s 读 BatteryState 缓存更新右键菜单文本（不查询，零蓝牙开销）。
        // 查询职责在 SettingPage（连接生命周期内），TaskbarIcon 只负责显示，避免重复查询和未连接时空转。
        private readonly DispatcherTimer _uiRefreshTimer = new DispatcherTimer();

        public TaskbarIcon()
        {
            this.InitializeComponent();
            var Theme = Application.Current.RequestedTheme;
            Console.WriteLine(System.AppDomain.CurrentDomain.SetupInformation.ApplicationBase);
            if (Theme == ApplicationTheme.Light)
                TaskbarIconView.IconSource = new BitmapImage(new Uri(System.AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "Image/LightIcon.ico"));
            else
                TaskbarIconView.IconSource = new BitmapImage(new Uri(System.AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "Image/DarkIcon.ico"));
            AutoStartItem = FindName("AutoStart") as ToggleMenuFlyoutItem;
            AutoRuntCommand_ = new AutoRuntCommand(CheckAutoStart);
            CheckAutoStart();

            // 启动 UI 刷新：2s 读缓存更新右键菜单文本（不查询，零开销）
            _uiRefreshTimer.Interval = TimeSpan.FromSeconds(2);
            _uiRefreshTimer.Tick += (_, _) => RefreshMenuText();
            _uiRefreshTimer.Start();
        }

        // 只读 BatteryState 缓存刷新 tooltip（不发起蓝牙查询，零开销）
        private void RefreshMenuText()
        {
            try
            {
                TaskbarIconView.ToolTipText = BatteryState.FormatDisplay();
            }
            catch { }
        }

        public void CheckAutoStart()
        {
            if (AutoRun.IsAutoStart())
                AutoStartItem.IsChecked = true;
            else
                AutoStartItem.IsChecked = false;
        }
        public static bool IsAdministrator()
        {
            WindowsIdentity current = WindowsIdentity.GetCurrent();
            WindowsPrincipal windowsPrincipal = new WindowsPrincipal(current);
            return windowsPrincipal.IsInRole(WindowsBuiltInRole.Administrator);
        }
    }
}
